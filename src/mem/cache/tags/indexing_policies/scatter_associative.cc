/*
 * Copyright (c) 2024 The Regents of the gem5-vulcan study.
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
 * Definitions of the ScatterCache randomized skewed-associative indexing
 * policy.
 */

#include "mem/cache/tags/indexing_policies/scatter_associative.hh"

#include <algorithm>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

namespace gem5
{

std::vector<ScatterAssociative *> ScatterAssociative::allPolicies;

ScatterAssociative::ScatterAssociative(const Params &p)
    : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size)),
      fixedKeys(p.keys.begin(), p.keys.end()),
      rng(p.seed >= 0 ? Random::genRandom((uint32_t)p.seed)
                      : Random::genRandom())
{
    allPolicies.push_back(this);
}

ScatterAssociative::~ScatterAssociative()
{
    allPolicies.erase(
        std::remove(allPolicies.begin(), allPolicies.end(), this),
        allPolicies.end());
}

uint64_t
ScatterAssociative::getDomainKey(ContextID domain) const
{
    // try_emplace() finds-or-inserts in a single hash lookup: on a hit it
    // just returns the existing entry; on a miss it inserts a placeholder
    // and hands back an iterator to fill in below. This avoids the separate
    // find() + operator[] pair (two lookups) the miss path used before.
    auto [it, inserted] = domainKeys.try_emplace(domain, 0);
    if (!inserted) {
        return it->second;
    }

    uint64_t key;
    if (domain >= 0 && (std::size_t)domain < fixedKeys.size() &&
        fixedKeys[domain] != 0) {
        // Reproducible, experiment-supplied key for this domain.
        key = fixedKeys[domain];
    } else {
        // Fresh high-entropy key; never zero (a zero key would collapse the
        // keyed mapping into a plain, predictable one).
        do {
            key = rng->random<uint64_t>();
        } while (key == 0);
    }

    it->second = key;
    return key;
}

std::vector<ReplaceableEntry *>
ScatterAssociative::getPossibleEntries(const KeyType &key) const
{
    std::vector<ReplaceableEntry *> entries;
    entries.reserve(assoc);

    // One candidate per way, each at that way's keyed set index.
    for (uint32_t way = 0; way < assoc; ++way) {
        entries.push_back(sets[extractSet(key, way)][way]);
    }

    return entries;
}

Addr
ScatterAssociative::regenerateAddr(const KeyType &key,
                                   const ReplaceableEntry *entry) const
{
    // The tag is the full line address (see extractTag), so the block address
    // is just the tag shifted back up by the block-offset width. This is
    // independent of the keyed set mapping, so writebacks and snoops always
    // reconstruct the correct physical address regardless of which domain
    // inserted the block.
    return key.address << setShift;
}

std::vector<uint32_t>
ScatterAssociative::setIndexVector(Addr addr, ContextID domain) const
{
    const uint64_t line_addr = addr >> setShift;
    const uint64_t dkey = getDomainKey(domain);

    std::vector<uint32_t> indices;
    indices.reserve(assoc);
    for (uint32_t way = 0; way < assoc; ++way) {
        indices.push_back(scatterIndexHash(line_addr, way, dkey) & setMask);
    }
    return indices;
}

void
ScatterAssociative::setActiveDomainAll(ContextID domain)
{
    for (auto *policy : allPolicies) {
        policy->activeDomainSet = true;
        policy->activeDomain = domain;
    }
}

void
ScatterAssociative::clearActiveDomainAll()
{
    for (auto *policy : allPolicies) {
        policy->activeDomainSet = false;
    }
}

} // namespace gem5
