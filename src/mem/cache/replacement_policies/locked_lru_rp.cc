/**
 * Copyright (c) 2018-2020 Inria
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

#include "mem/cache/replacement_policies/locked_lru_rp.hh"

#include <cassert>
#include <memory>

#include "params/LockedLRURP.hh"
#include "sim/cur_tick.hh"

namespace gem5
{

namespace replacement_policy
{

LockedLRU::LockedLRU(const Params &p)
  : Base(p)
{
}

void
LockedLRU::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
{
    // Reset last touch timestamp
    std::static_pointer_cast<PartitionData>(
        replacement_data)->lastTouchTick = Tick(0);
}

void
LockedLRU::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Update last touch timestamp
    std::static_pointer_cast<PartitionData>(
        replacement_data)->lastTouchTick = curTick();
}

void
LockedLRU::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Set last touch timestamp
    std::static_pointer_cast<PartitionData>(
        replacement_data)->lastTouchTick = curTick();
}

ReplaceableEntry*
LockedLRU::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);
    ReplaceableEntry* victim = nullptr;
    Tick victimTick = 0;
    // Visit all candidates to find victim
    for (const auto& candidate : candidates) {
        const auto *data =
            static_cast<PartitionData *>(candidate->replacementData.get());
        if (data->locked) {
            continue;
        }
        if (victim == nullptr || data->lastTouchTick < victimTick) {
            victim = candidate;
            victimTick = data->lastTouchTick;
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
LockedLRU::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new PartitionData());
}

void
LockedLRU::lock(const std::shared_ptr<ReplacementData>& replacement_data,
                const ReplacementCandidates& candidates)
{
    auto data = std::static_pointer_cast<PartitionData>(replacement_data);
    if (data -> locked)
        return;
    int count = 0;
    for (const auto& candidate: candidates) {
        auto candidateData = std::static_pointer_cast<PartitionData>(candidate->replacementData);
        if(!candidateData ->locked)
            count++;
    }
    if (count < 2){
        warn("not enough unlocked ways did not lock");
        return;
    }
    data->locked = true;
}

void
LockedLRU::unlock(const std::shared_ptr<ReplacementData>& replacement_data)
{
    auto data = std::static_pointer_cast<PartitionData>(replacement_data);
    data->locked = 0;
    data->lastTouchTick = curTick();
}

} // namespace replacement_policy
} // namespace gem5
