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

#include "mem/cache/tags/partition_locked.hh"

#include "base/trace.hh"
#include "debug/CacheRepl.hh"
#include "mem/request.hh"

namespace gem5
{

PartitionLockedTags::PartitionLockedTags(const Params &p)
    : BaseSetAssoc(p), protectedRanges(p.protected_ranges), plStats(stats)
{
}

CacheBlk*
PartitionLockedTags::findVictim(const CacheBlk::KeyType &key,
                                const std::size_t size,
                                std::vector<CacheBlk*> &evict_blks,
                                const uint64_t partition_id,
                                const ContextID context_id)
{
    // Get possible entries to be victimized
    std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(key);

    // Filter entries based on PartitionID
    if (partitionManager) {
        partitionManager->filterByPartition(entries, partition_id);
    }

    // Whether the incoming line will itself be locked, had it been
    // inserted (see insertBlock)
    const bool incoming_locked = isProtectedAddr(key.address, protectedRanges);

    // PL enforcement: remove candidates the incoming request is not
    // allowed to evict
    filterEvictable(entries, incoming_locked, context_id);

    CacheBlk *victim = nullptr;
    if (entries.empty()) {
        // No eligible victim remains: service this request without
        // caching it, rather than evicting protected data.
        plStats.plBypassOnLock++;
        DPRINTF(CacheRepl, "PL cache: no evictable victim for addr %#llx, "
                "bypassing allocation\n", key.address);
    } else {
        // Choose replacement victim from the remaining candidates
        victim = static_cast<CacheBlk*>(replacementPolicy->getVictim(entries));
    }

    // There is only one eviction for this replacement
    evict_blks.push_back(victim);

    return victim;
}

void
PartitionLockedTags::insertBlock(const PacketPtr pkt, CacheBlk *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);

    if (isProtectedAddr(pkt->getAddr(), protectedRanges)) {
        const ContextID owner = pkt->req->hasContextId()
                                    ? pkt->req->contextId()
                                    : InvalidContextID;
        blk->setPlLocked(owner);
        plStats.plLockedBlocks++;
        DPRINTF(CacheRepl, "PL cache: locked block %s to owner %d\n",
                blk->print(), owner);
    }
}

void
PartitionLockedTags::notifyFlushSuppressed(CacheBlk *blk)
{
    plStats.plFlushSuppressed++;
    DPRINTF(CacheRepl, "PL cache: suppressed invalidation of locked block "
            "%s (owner %d)\n", blk->print(), blk->getPlOwner());
}

PartitionLockedTags::PartitionLockedTagsStats::PartitionLockedTagsStats(
    BaseTagStats &base_group)
    : statistics::Group(&base_group),
    ADD_STAT(plLockedBlocks, statistics::units::Count::get(),
             "Number of blocks locked by the PL cache defense"),
    ADD_STAT(plBypassOnLock, statistics::units::Count::get(),
             "Number of allocations that bypassed the cache because no "
             "evictable victim existed among the replacement candidates"),
    ADD_STAT(plFlushSuppressed, statistics::units::Count::get(),
             "Number of cache-maintenance/coherence invalidations "
             "suppressed on PL-locked blocks")
{
}

} // namespace gem5
