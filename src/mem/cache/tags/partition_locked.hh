/**
 * Copyright (c) 2026 Aaryan Patel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Declaration of a Partition-Locked (PL) cache tag store, implementing the
 * secure-cache defense described in Deng, Xiong and Szefer, "Analysis of
 * Secure Caches". Security-critical lines are preloaded and locked to an
 * owning context. A locked line may only be evicted by an incoming line
 * that is itself locked and owned by the same context; when no eligible
 * victim exists in the candidate set, the incoming request bypasses the
 * cache instead of evicting protected data. Locked lines are also immune
 * to invalidation, so cache-maintenance operations (e.g. clflush) cannot
 * be used to evict them either.
 */

#ifndef __MEM_CACHE_TAGS_PARTITION_LOCKED_HH__
#define __MEM_CACHE_TAGS_PARTITION_LOCKED_HH__

#include <algorithm>
#include <vector>

#include "base/addr_range.hh"
#include "base/types.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/base_set_assoc.hh"
#include "mem/packet.hh"
#include "params/PartitionLockedTags.hh"

namespace gem5
{

/**
 * A set-associative tag store that implements the Partition-Locked (PL)
 * cache defense on top of BaseSetAssoc. All PL-specific behavior is
 * self-contained in this class: labeling of protected lines on insertion,
 * eviction-eligibility enforcement on replacement, and flush suppression
 * on invalidation.
 */
class PartitionLockedTags : public BaseSetAssoc
{
  protected:
    /** Address ranges holding security-critical data. Any line that maps
     * to one of these ranges is locked to its inserting context. */
    const std::vector<AddrRange> protectedRanges;

    struct PartitionLockedTagsStats : public statistics::Group
    {
        PartitionLockedTagsStats(BaseTagStats &base_group);

        /** Number of blocks locked by the PL defense. */
        statistics::Scalar plLockedBlocks;

        /** Number of allocations that bypassed the cache because every
         * replacement candidate was a locked line the incoming request
         * was not allowed to evict. */
        statistics::Scalar plBypassOnLock;

        /** Number of invalidation/flush attempts suppressed because the
         * targeted block was PL-locked. */
        statistics::Scalar plFlushSuppressed;
    } plStats;

  public:
    typedef PartitionLockedTagsParams Params;

    PartitionLockedTags(const Params &p);

    virtual ~PartitionLockedTags() {};

    /**
     * Determine whether an address falls within one of the configured
     * protected ranges. Defined inline so that the core PL eligibility
     * logic (this function, canEvict, and filterEvictable) can be
     * exercised directly by unit tests without needing to instantiate a
     * full PartitionLockedTags SimObject (which would require a system,
     * clock domain, indexing policy, and replacement policy).
     *
     * @param addr The address to check.
     * @param ranges The set of protected ranges.
     * @return True if addr is protected.
     */
    static bool
    isProtectedAddr(Addr addr, const std::vector<AddrRange> &ranges)
    {
        for (const auto &range : ranges) {
            if (range.contains(addr)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Determine whether a candidate victim may be evicted by an incoming
     * line, per the PL defense rules: a locked candidate can only be
     * evicted by an incoming line that is itself locked and owned by the
     * same context. Unlocked candidates are always evictable.
     *
     * @param candidate The candidate victim block.
     * @param incoming_locked Whether the incoming line will be locked.
     * @param incoming_owner Owning context of the incoming line, if
     *        locked. Ignored if incoming_locked is false.
     * @return True if candidate may be evicted.
     */
    static bool
    canEvict(const CacheBlk *candidate, bool incoming_locked,
             ContextID incoming_owner)
    {
        if (!candidate->isPlLocked()) {
            // Unlocked lines are always evictable.
            return true;
        }

        // A locked candidate can only be evicted by an incoming line that
        // is itself locked and owned by the same context.
        return incoming_locked && (candidate->getPlOwner() == incoming_owner);
    }

    /**
     * Remove from entries every candidate that the incoming line is not
     * allowed to evict, per canEvict().
     *
     * @param entries Candidate victims; filtered in place.
     * @param incoming_locked Whether the incoming line will be locked.
     * @param incoming_owner Owning context of the incoming line, if
     *        locked.
     */
    static void
    filterEvictable(std::vector<ReplaceableEntry *> &entries,
                     bool incoming_locked, ContextID incoming_owner)
    {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [incoming_locked, incoming_owner](ReplaceableEntry *entry) {
                const CacheBlk *blk = static_cast<const CacheBlk *>(entry);
                return !canEvict(blk, incoming_locked, incoming_owner);
            }), entries.end());
    }

    /**
     * Find replacement victim, honoring PL lock enforcement. Locked
     * candidates that the incoming request is not allowed to evict are
     * removed before consulting the replacement policy. If no candidate
     * remains, nullptr is returned so the caller bypasses the cache
     * instead of evicting protected data.
     */
    CacheBlk* findVictim(const CacheBlk::KeyType &key,
                         const std::size_t size,
                         std::vector<CacheBlk*>& evict_blks,
                         const uint64_t partition_id=0,
                         const ContextID context_id=InvalidContextID)
                         override;

    /**
     * Insert the new block into the cache. If the block's address falls
     * within a protected range, it is locked to the requesting context.
     */
    void insertBlock(const PacketPtr pkt, CacheBlk *blk) override;

    /**
     * Notified by BaseCache when a cache-maintenance or coherence
     * invalidation targeting a PL-locked block was suppressed (the block
     * itself is left untouched by the caller). Bumps plFlushSuppressed.
     *
     * @param blk The PL-locked block whose invalidation was suppressed.
     */
    void notifyFlushSuppressed(CacheBlk *blk) override;
};

} // namespace gem5

#endif //__MEM_CACHE_TAGS_PARTITION_LOCKED_HH__
