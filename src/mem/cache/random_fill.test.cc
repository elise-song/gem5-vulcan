/*
 * Copyright (c) 2026 The Regents of The University of Michigan
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
 * Directed tests for the Random Fill secure cache helper logic. These
 * exercise the exact production functions used by BaseCache (there is no
 * reimplementation here), verifying the two security-critical invariants:
 *   - a protected address is recognised as protected, and
 *   - the injected random fill is always a legal, in-region, in-window line
 *     that is never the demanded line itself (so the demanded line is never
 *     the one that ends up cached).
 */

#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "base/addr_range.hh"
#include "base/random.hh"
#include "mem/cache/random_fill.hh"

using namespace gem5;

namespace {

constexpr unsigned BlkSize = 64;
// A 4 KiB protected region (64 lines) based at 64 KiB.
constexpr Addr RegionBase = 0x10000;
constexpr Addr RegionEnd = RegionBase + 4096;

} // namespace

// isProtected covers exactly the half-open interval [start, end).
TEST(RandomFillTest, IsProtectedBoundaries)
{
    std::vector<AddrRange> ranges = {AddrRange(RegionBase, RegionEnd)};

    EXPECT_FALSE(random_fill::isProtected(ranges, RegionBase - BlkSize));
    EXPECT_TRUE(random_fill::isProtected(ranges, RegionBase));
    EXPECT_TRUE(random_fill::isProtected(ranges, RegionEnd - BlkSize));
    // end is exclusive
    EXPECT_FALSE(random_fill::isProtected(ranges, RegionEnd));
    EXPECT_FALSE(random_fill::isProtected(ranges, RegionEnd + BlkSize));

    // No ranges => never protected (defense disabled).
    std::vector<AddrRange> none;
    EXPECT_FALSE(random_fill::isProtected(none, RegionBase));
}

// The random fill is always block-aligned, inside the region, within the
// window, and never the demanded line. Swept across every demand line and
// many RNG draws so window-edge clamping is exercised too.
TEST(RandomFillTest, NeighborInvariants)
{
    Random::RandomPtr rng = Random::genRandom(0xC0FFEE);
    const unsigned window = 8;
    const unsigned num_lines = (RegionEnd - RegionBase) / BlkSize;

    for (unsigned dl = 0; dl < num_lines; dl++) {
        const Addr demand = RegionBase + (Addr)dl * BlkSize;
        for (int trial = 0; trial < 200; trial++) {
            const Addr fill = random_fill::pickNeighbor(
                demand, BlkSize, RegionBase, RegionEnd, window, *rng);

            // block-aligned
            EXPECT_EQ(fill % BlkSize, 0u);
            // in region
            EXPECT_GE(fill, RegionBase);
            EXPECT_LT(fill, RegionEnd);
            // never the demanded line (the whole point of the defense)
            EXPECT_NE(fill, demand);
            // within +/- window lines
            const long long delta =
                ((long long)fill - (long long)demand) / (long long)BlkSize;
            EXPECT_LE(std::llabs(delta), (long long)window);
        }
    }
}

// Over enough draws the fill actually varies (it is not a fixed offset), and
// it can land on either side of the demanded line when the window allows.
TEST(RandomFillTest, NeighborIsRandomised)
{
    Random::RandomPtr rng = Random::genRandom(0x1234);
    const unsigned window = 8;
    // Pick a demand line with room on both sides.
    const Addr demand = RegionBase + 20 * BlkSize;

    std::set<Addr> seen;
    bool below = false, above = false;
    for (int trial = 0; trial < 500; trial++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, RegionBase, RegionEnd, window, *rng);
        seen.insert(fill);
        below |= (fill < demand);
        above |= (fill > demand);
    }
    // Many distinct neighbours, on both sides.
    EXPECT_GT(seen.size(), 4u);
    EXPECT_TRUE(below);
    EXPECT_TRUE(above);
}

// A degenerate zero window still yields a distinct, in-region neighbour so
// the demanded line is never the one cached.
TEST(RandomFillTest, ZeroWindowStillDistinct)
{
    Random::RandomPtr rng = Random::genRandom(0x99);
    const Addr demand = RegionBase + 5 * BlkSize;
    const Addr fill = random_fill::pickNeighbor(
        demand, BlkSize, RegionBase, RegionEnd, /*window=*/0, *rng);
    EXPECT_NE(fill, demand);
    EXPECT_GE(fill, RegionBase);
    EXPECT_LT(fill, RegionEnd);
    EXPECT_EQ(fill % BlkSize, 0u);
}
