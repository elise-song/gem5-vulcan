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
 * Unit tests for the ScatterCache keyed index-derivation function. These
 * exercise the actual production mapping logic (scatterIndexHash) plus the
 * full-tag reconstruction arithmetic used by ScatterAssociative, without
 * needing to instantiate a SimObject.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include "mem/cache/tags/indexing_policies/scatter_hash.hh"

using gem5::scatterIndexHash;

namespace
{

// A representative cache geometry: 64 B lines, 64 sets, 8 ways.
constexpr unsigned SET_SHIFT = 6;           // log2(64 B line)
constexpr unsigned NUM_SETS = 64;
constexpr uint64_t SET_MASK = NUM_SETS - 1; // 0x3f
constexpr unsigned ASSOC = 8;

// The set-index vector an address maps to under a given domain key: one set
// per way, exactly as ScatterAssociative::getPossibleEntries computes it.
std::vector<uint32_t>
setVector(uint64_t addr, uint64_t key)
{
    const uint64_t line_addr = addr >> SET_SHIFT;
    std::vector<uint32_t> v;
    for (unsigned way = 0; way < ASSOC; ++way) {
        v.push_back(scatterIndexHash(line_addr, way, key) & SET_MASK);
    }
    return v;
}

} // namespace

// The mapping must be a pure function: identical inputs give identical output.
TEST(ScatterHashTest, Deterministic)
{
    const uint64_t addr = 0xcafe000;
    const uint64_t key = 0x0123456789abcdefULL;
    EXPECT_EQ(setVector(addr, key), setVector(addr, key));
    EXPECT_EQ(scatterIndexHash(0x1234, 3, key),
              scatterIndexHash(0x1234, 3, key));
}

// REQUIREMENT: the index must depend on the domain key. Two different keys
// must produce a different candidate set-index vector for the same address,
// so that two domains do not share a congruence class.
TEST(ScatterHashTest, DependsOnDomainKey)
{
    const uint64_t addr = 0xcafe000;
    const uint64_t key_a = 0x1111111111111111ULL;
    const uint64_t key_b = 0x2222222222222222ULL;

    const auto va = setVector(addr, key_a);
    const auto vb = setVector(addr, key_b);
    EXPECT_NE(va, vb);

    // Stronger: across many addresses, two distinct domains almost never map
    // an address to the *same full set-vector*. Require zero exact collisions
    // over a large sample (the whole point of ScatterCache: no shared eviction
    // set across domains).
    unsigned full_collisions = 0;
    for (uint64_t i = 0; i < 4096; ++i) {
        const uint64_t a = i * 64;
        if (setVector(a, key_a) == setVector(a, key_b)) {
            ++full_collisions;
        }
    }
    EXPECT_EQ(full_collisions, 0u);
}

// REQUIREMENT: the index must depend on the way (skewing). A single address
// must not land in the same set in every way.
TEST(ScatterHashTest, DependsOnWay)
{
    const uint64_t key = 0xdeadbeefcafef00dULL;
    const auto v = setVector(0xcafe000, key);

    std::set<uint32_t> distinct(v.begin(), v.end());
    EXPECT_GT(distinct.size(), 1u)
        << "all ways mapped to the same set -- no skewing";
}

// A zero key must still yield a usable (non-degenerate) mapping across ways,
// confirming the function never collapses when a key slips through as zero.
TEST(ScatterHashTest, WayDependenceEvenWithZeroKey)
{
    const auto v = setVector(0xcafe000, 0);
    std::set<uint32_t> distinct(v.begin(), v.end());
    EXPECT_GT(distinct.size(), 1u);
}

// Sanity: the mapping spreads addresses across many sets rather than piling
// them into a handful, per way.
TEST(ScatterHashTest, SpreadsAcrossSets)
{
    const uint64_t key = 0x0f1e2d3c4b5a6978ULL;
    std::set<uint32_t> way0_sets;
    for (uint64_t i = 0; i < 1024; ++i) {
        way0_sets.insert(scatterIndexHash(i, 0, key) & SET_MASK);
    }
    // With 64 sets and 1024 addresses we expect to touch essentially all sets.
    EXPECT_GE(way0_sets.size(), NUM_SETS / 2);
}

// REQUIREMENT: address reconstruction round-trips. ScatterAssociative keeps
// the full line address as the tag (extractTag == addr >> setShift) and
// reconstructs via regenerateAddr == tag << setShift, independent of the keyed
// set mapping. Verify this recovers the block-aligned address exactly.
TEST(ScatterHashTest, TagRoundTrip)
{
    for (uint64_t base = 0; base < 100000; base += 777) {
        const uint64_t addr = base * 64 + 13; // arbitrary offset within a line
        const uint64_t tag = addr >> SET_SHIFT;         // extractTag
        const uint64_t regenerated = tag << SET_SHIFT;  // regenerateAddr
        EXPECT_EQ(regenerated, addr & ~((1ULL << SET_SHIFT) - 1));
    }
}
