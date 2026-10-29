/*
 * Copyright (c) 2012-2014, 2017, 2023-2024 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
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
 * Declaration of a base set associative tag store.
 */

#ifndef __MEM_CACHE_TAGS_BASE_SET_ASSOC_HH__
#define __MEM_CACHE_TAGS_BASE_SET_ASSOC_HH__

#include <cstdint>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/base.hh"
#include "base/addr_range.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/cache/tags/partitioning_policies/partition_manager.hh"
#include "mem/packet.hh"
#include "params/BaseSetAssoc.hh"

namespace gem5
{

/**
 * A basic cache tag store.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 *
 * The BaseSetAssoc placement policy divides the cache into s sets of w
 * cache lines (ways).
 */
class BaseSetAssoc : public BaseTags
{
  protected:
    /** The allocatable associativity of the cache (alloc mask). */
    unsigned allocAssoc;

    /** The cache blocks. */
    std::vector<CacheBlk> blks;

    /** Whether tags and data are accessed sequentially. */
    const bool sequentialAccess;

    /** Replacement policy */
    replacement_policy::Base *replacementPolicy;

    /** Address ranges whose lines are pinned, on insertion, to the
     * context that brought them in. */
    const std::vector<AddrRange> pinnedRanges;

    struct PinStats : public statistics::Group
    {
        PinStats(BaseTagStats &base_group);

        /** Number of blocks pinned on insertion. */
        statistics::Scalar pinnedBlocks;

        /** Number of allocations that bypassed the cache because every
         * replacement candidate was a pinned line the incoming request
         * was not allowed to evict. */
        statistics::Scalar bypassOnPinnedVictim;

        /** Number of invalidation/flush attempts suppressed because the
         * targeted block was pinned. */
        statistics::Scalar flushSuppressedPinned;
    } pinStats;

  public:
    /** Convenience typedef. */
     typedef BaseSetAssocParams Params;

    /**
     * Construct and initialize this tag store.
     */
    BaseSetAssoc(const Params &p);

    /**
     * Destructor
     */
    virtual ~BaseSetAssoc() {};

    /**
     * Initialize blocks as CacheBlk instances.
     */
    void tagsInit() override;

    /**
     * This function updates the tags when a block is invalidated. It also
     * updates the replacement data.
     *
     * @param blk The block to invalidate.
     */
    void invalidate(CacheBlk *blk) override;

    /**
     * Access block and update replacement data. May not succeed, in which case
     * nullptr is returned. This has all the implications of a cache access and
     * should only be used as such. Returns the tag lookup latency as a side
     * effect.
     *
     * @param pkt The packet holding the address to find.
     * @param lat The latency of the tag lookup.
     * @return Pointer to the cache block if found.
     */
    CacheBlk* accessBlock(const PacketPtr pkt, Cycles &lat) override
    {
        CacheBlk *blk = findBlock({pkt->getAddr(), pkt->isSecure()});

        // Access all tags in parallel, hence one in each way.  The data side
        // either accesses all blocks in parallel, or one block sequentially on
        // a hit.  Sequential access with a miss doesn't access data.
        stats.tagAccesses += allocAssoc;
        if (sequentialAccess) {
            if (blk != nullptr) {
                stats.dataAccesses += 1;
            }
        } else {
            stats.dataAccesses += allocAssoc;
        }

        // If a cache hit
        if (blk != nullptr) {
            // Update number of references to accessed block
            blk->increaseRefCount();

            // Update replacement data of accessed block
            replacementPolicy->touch(blk->replacementData, pkt);
        }

        // The tag lookup latency is the same for a hit or a miss
        lat = lookupLatency;

        return blk;
    }

    /**
     * Determine whether an address falls within one of the configured
     * pinned ranges.
     *
     * @param addr The address to check.
     * @param ranges The set of pinned ranges.
     * @return True if addr is in a pinned range.
     */
    static bool
    inPinnedRange(Addr addr, const std::vector<AddrRange> &ranges)
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
     * line: a pinned candidate can only be evicted by an incoming line
     * that is itself pinned and owned by the same context. Unpinned
     * candidates are always evictable.
     *
     * @param candidate The candidate victim block.
     * @param incoming_pinned Whether the incoming line will be pinned.
     * @param incoming_owner Owning context of the incoming line, if
     *        pinned. Ignored if incoming_pinned is false.
     * @return True if candidate may be evicted.
     */
    static bool
    canEvict(const CacheBlk *candidate, bool incoming_pinned,
             ContextID incoming_owner)
    {
        if (!candidate->isPinned()) {
            // Unpinned lines are always evictable.
            return true;
        }

        // A pinned candidate can only be evicted by an incoming line that
        // is itself pinned and owned by the same context.
        return incoming_pinned && (candidate->getPinOwner() == incoming_owner);
    }

    /**
     * Remove from entries every candidate that the incoming line is not
     * allowed to evict, per canEvict().
     *
     * @param entries Candidate victims; filtered in place.
     * @param incoming_pinned Whether the incoming line will be pinned.
     * @param incoming_owner Owning context of the incoming line, if
     *        pinned.
     */
    static void
    filterEvictable(std::vector<ReplaceableEntry *> &entries,
                    bool incoming_pinned, ContextID incoming_owner)
    {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [incoming_pinned, incoming_owner](ReplaceableEntry *entry) {
                const CacheBlk *blk = static_cast<const CacheBlk *>(entry);
                return !canEvict(blk, incoming_pinned, incoming_owner);
            }), entries.end());
    }

    /**
     * Find replacement victim based on address. The list of evicted blocks
     * only contains the victim. Pinned candidates that the incoming
     * request is not allowed to evict are removed before consulting the
     * replacement policy; if no candidate remains, nullptr is returned so
     * the caller bypasses the cache instead of evicting a pinned line.
     *
     * @param key Address (and secure bit) to find a victim for.
     * @param size Size, in bits, of new block to allocate.
     * @param evict_blks Cache blocks to be evicted.
     * @param partition_id Partition ID for resource management.
     * @param context_id Context ID of the requestor.
     * @return Cache block to be replaced.
     */
    CacheBlk* findVictim(const CacheBlk::KeyType& key,
                         const std::size_t size,
                         std::vector<CacheBlk*>& evict_blks,
                         const uint64_t partition_id=0,
                         const ContextID context_id=InvalidContextID)
                         override;

    /**
     * Insert the new block into the cache and update replacement data.
     *
     * @param pkt Packet holding the address to update
     * @param blk The block to update.
     */
    void insertBlock(const PacketPtr pkt, CacheBlk *blk) override
    {
        // Insert block
        BaseTags::insertBlock(pkt, blk);

        // Increment tag counter
        stats.tagsInUse++;

        if (partitionManager) {
            auto partition_id = partitionManager->readPacketPartitionID(pkt);
            partitionManager->notifyAcquire(partition_id);
        }

        // Update replacement policy
        replacementPolicy->reset(blk->replacementData, pkt);

        // Pin lines from the configured ranges to the inserting context
        if (inPinnedRange(pkt->getAddr(), pinnedRanges)) {
            const ContextID owner = pkt->req->hasContextId()
                                        ? pkt->req->contextId()
                                        : InvalidContextID;
            blk->setPinned(owner);
            pinStats.pinnedBlocks++;
        }
    }

    void moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk) override;

    /**
     * Notified by BaseCache when a cache-maintenance or coherence
     * invalidation targeting a pinned block was suppressed (the block
     * itself is left untouched by the caller).
     *
     * @param blk The pinned block whose invalidation was suppressed.
     */
    void notifyFlushSuppressed(CacheBlk *blk) override;

    /**
     * Limit the allocation for the cache ways.
     * @param ways The maximum number of ways available for replacement.
     */
    virtual void setWayAllocationMax(int ways) override
    {
        fatal_if(ways < 1, "Allocation limit must be greater than zero");
        allocAssoc = ways;
    }

    /**
     * Get the way allocation mask limit.
     * @return The maximum number of ways available for replacement.
     */
    virtual int getWayAllocationMax() const override
    {
        return allocAssoc;
    }

    /**
     * Regenerate the block address from the tag and indexing location.
     *
     * @param block The block.
     * @return the block address.
     */
    Addr regenerateBlkAddr(const CacheBlk* blk) const override
    {
        return indexingPolicy->regenerateAddr({blk->getTag(), false}, blk);
    }

    bool anyBlk(std::function<bool(CacheBlk &)> visitor) override {
        for (CacheBlk& blk : blks) {
            if (visitor(blk)) {
                return true;
            }
        }
        return false;
    }
};

} // namespace gem5

#endif //__MEM_CACHE_TAGS_BASE_SET_ASSOC_HH__
