/*
 * Copyright (c) 2018 Inria
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
 * Definitions of the tagged set associative indexing policy, including the
 * optional context-keyed skewed index.
 */

#include "mem/cache/tags/tagged_entry.hh"

#include <algorithm>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "params/TaggedSetAssociative.hh"

namespace gem5
{

std::vector<TaggedSetAssociative *> TaggedSetAssociative::liveInstances_;

TaggedSetAssociative::TaggedSetAssociative(const Params &p)
    : TaggedIndexingPolicy(p, p.size / p.entry_size,
                           floorLog2(p.entry_size)),
      contextKeyed(p.context_keyed),
      keyTable_(kKeyTableSlots, 0),
      fixedKeys_(p.context_keys.begin(), p.context_keys.end()),
      rng_(p.key_seed >= 0 ? Random::genRandom((uint32_t)p.key_seed)
                          : Random::genRandom())
{
    if (contextKeyed) {
        liveInstances_.push_back(this);
    }
}

TaggedSetAssociative::~TaggedSetAssociative()
{
    liveInstances_.erase(
        std::remove(liveInstances_.begin(), liveInstances_.end(), this),
        liveInstances_.end());
}

uint64_t
TaggedSetAssociative::keyForContext(ContextID context) const
{
    const std::size_t slot = slotFor(context);
    if (keyTable_[slot] != 0) {
        // Already resident: reuse the key currently occupying this slot.
        return keyTable_[slot];
    }

    uint64_t key;
    if (context >= 0 && (std::size_t)context < fixedKeys_.size() &&
        fixedKeys_[context] != 0) {
        // Reproducible, experiment-supplied key for this context.
        key = fixedKeys_[context];
    } else {
        // Fresh high-entropy key; never zero (a zero key would collapse the
        // keyed mapping into a plain, predictable one).
        do {
            key = rng_->random<uint64_t>();
        } while (key == 0);
    }

    keyTable_[slot] = key;
    return key;
}

Addr
TaggedSetAssociative::extractTag(const Addr addr) const
{
    if (!contextKeyed) {
        return TaggedIndexingPolicy::extractTag(addr);
    }
    return addr >> setShift;
}

std::vector<ReplaceableEntry*>
TaggedSetAssociative::getPossibleEntries(const KeyType &key) const
{
    if (!contextKeyed) {
        return sets[extractSet(key)];
    }

    std::vector<ReplaceableEntry*> entries;
    entries.reserve(assoc);

    // One candidate per way, each at that way's keyed set index.
    for (uint32_t way = 0; way < assoc; ++way) {
        entries.push_back(sets[computeWaySet(key, way)][way]);
    }

    return entries;
}

Addr
TaggedSetAssociative::regenerateAddr(const KeyType &key,
                                     const ReplaceableEntry *entry) const
{
    if (!contextKeyed) {
        return (key.address << tagShift) | (entry->getSet() << setShift);
    }
    // The tag is the full line address (see extractTag), so the block
    // address is just the tag shifted back up by the block-offset width,
    // independent of the keyed set mapping.
    return key.address << setShift;
}

std::vector<uint32_t>
TaggedSetAssociative::setIndexVector(Addr addr, ContextID context) const
{
    const uint64_t line_addr = addr >> setShift;
    const uint64_t ckey = keyForContext(context);

    std::vector<uint32_t> indices;
    indices.reserve(assoc);
    for (uint32_t way = 0; way < assoc; ++way) {
        indices.push_back(wayIndexHash(line_addr, way, ckey) & setMask);
    }
    return indices;
}

void
TaggedSetAssociative::setActiveContextAll(ContextID context)
{
    for (auto *policy : liveInstances_) {
        policy->overrideActive_ = true;
        policy->overrideContext_ = context;
    }
}

void
TaggedSetAssociative::clearActiveContextAll()
{
    for (auto *policy : liveInstances_) {
        policy->overrideActive_ = false;
    }
}

} // namespace gem5
