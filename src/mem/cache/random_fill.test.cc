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
 * reimplementation here), verifying the security-critical invariants:
 *   - a protected address is recognised as protected;
 *   - the random-fill draw is always a legal, block-aligned, in-region,
 *     in-window line -- and, per Liu & Lee Eq. 6, may legitimately equal the
 *     demanded line itself, since that is what is required for the draw to
 *     be equally likely to land on the demanded line ("collision") or on any
 *     other in-window line ("no collision"); and
 *   - that equal-likelihood property actually holds: CollisionTimingSignal-
 *     VanishesAtFullWindow empirically measures the gap between those two
 *     probabilities and would fail if the demanded line were ever excluded
 *     from the draw (as an earlier version of this code did).
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

// The random fill is always block-aligned, inside the region, and within the
// window. It may legitimately equal the demanded line (see the file
// comment and CollisionTimingSignalVanishesAtFullWindow below). Swept across
// every demand line and many RNG draws so window-edge clamping is exercised
// too.
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
            // within +/- window lines (0 offset, i.e. the demanded line
            // itself, is an allowed outcome)
            const long long delta =
                ((long long)fill - (long long)demand) / (long long)BlkSize;
            EXPECT_LE(std::llabs(delta), (long long)window);
        }
    }
}

// Over enough draws the fill actually varies (it is not a fixed offset), it
// can land on either side of the demanded line when the window allows, and
// it does occasionally coincide with the demanded line itself.
TEST(RandomFillTest, NeighborIsRandomised)
{
    Random::RandomPtr rng = Random::genRandom(0x1234);
    const unsigned window = 8;
    // Pick a demand line with room on both sides.
    const Addr demand = RegionBase + 20 * BlkSize;

    std::set<Addr> seen;
    bool below = false, above = false, self_seen = false;
    for (int trial = 0; trial < 500; trial++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, RegionBase, RegionEnd, window, *rng);
        seen.insert(fill);
        below |= (fill < demand);
        above |= (fill > demand);
        self_seen |= (fill == demand);
    }
    // Many distinct neighbours, on both sides, including the demand itself.
    EXPECT_GT(seen.size(), 4u);
    EXPECT_TRUE(below);
    EXPECT_TRUE(above);
    EXPECT_TRUE(self_seen);
}

// A degenerate zero window has exactly one candidate -- the demanded line
// itself -- matching the paper's RR=0 case, where random fill degenerates to
// plain demand fetch (Section IV.B.3).
TEST(RandomFillTest, ZeroWindowReturnsDemandLine)
{
    Random::RandomPtr rng = Random::genRandom(0x99);
    const Addr demand = RegionBase + 5 * BlkSize;
    const Addr fill = random_fill::pickNeighbor(
        demand, BlkSize, RegionBase, RegionEnd, /*window=*/0, *rng);
    EXPECT_EQ(fill, demand);
}

// The security-critical property (Liu & Lee, Eq. 6): once the window covers
// the whole protected region, the draw must be *equally* likely to land on
// the demanded line itself (a "collision", P1) as on any other specific
// in-window line (a "no collision", P2). The gap P1-P2 is exactly the
// extractable cache-collision timing signal (Eq. 4). An earlier version of
// pickNeighbor() excluded the demanded line from the draw entirely, which
// made P1 a hard 0 while P2 stayed positive -- a large, easily
// distinguishable gap (~1/(num_lines-1), independent of window size) that
// defeated the defense's core security claim for this attack. This test
// would have failed against that version.
TEST(RandomFillTest, CollisionTimingSignalVanishesAtFullWindow)
{
    Random::RandomPtr rng = Random::genRandom(0xBADC0FFE);
    // A 16-line region, matching the paper's own AES-table case study
    // (Section V.A): a 1 KiB table with 64-byte lines.
    constexpr unsigned num_lines = 16;
    const Addr region_lo = RegionBase;
    const Addr region_hi = RegionBase + (Addr)num_lines * BlkSize;
    const unsigned window = num_lines; // covers the whole region from any i
    constexpr int trials = 200000;

    const unsigned demand_line = 3; // i
    const unsigned other_line = 11; // j != i, arbitrary
    const Addr demand = region_lo + (Addr)demand_line * BlkSize;
    const Addr other = region_lo + (Addr)other_line * BlkSize;
    ASSERT_NE(demand, other);

    int self_hits = 0, other_hits = 0;
    for (int t = 0; t < trials; t++) {
        const Addr fill = random_fill::pickNeighbor(demand, BlkSize, region_lo,
                                                    region_hi, window, *rng);
        self_hits += (fill == demand);
        other_hits += (fill == other);
    }

    const double p1 = (double)self_hits / trials;  // P(collision hit)
    const double p2 = (double)other_hits / trials; // P(no-collision hit)
    const double expected = 1.0 / num_lines;

    // With 200k trials at p~1/16, the binomial standard error is ~0.0005, so
    // a tolerance of 0.01 (20 SEs) comfortably absorbs sampling noise while
    // still rejecting the ~0.067 gap the exclusion bug produced.
    constexpr double tol = 0.01;
    EXPECT_NEAR(p1, expected, tol) << "P1 (collision) = " << p1;
    EXPECT_NEAR(p2, expected, tol) << "P2 (no collision) = " << p2;
    EXPECT_NEAR(p1, p2, tol)
        << "collision-timing signal P1-P2 = " << (p1 - p2)
        << " (want ~0 once the window covers the region -- "
        << "the demanded line must be a valid draw outcome, not excluded)";
}
