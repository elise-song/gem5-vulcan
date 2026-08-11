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
 *   - the random-fill draw is always a legal, block-aligned, in-window line
 *     -- and, per Liu & Lee Eq. 6, may legitimately equal the demanded line
 *     itself, since that is what is required for the draw to be equally
 *     likely to land on the demanded line ("collision") or on any other
 *     in-window line ("no collision");
 *   - that equal-likelihood property actually holds: CollisionTimingSignal-
 *     VanishesAtFullWindow empirically measures the gap between those two
 *     probabilities and would fail if the demanded line were ever excluded
 *     from the draw (as an earlier version of this code did);
 *   - the window's two radii (before/after the demand, matching RR1/RR2 in
 *     the paper's Fig. 4) are honoured independently even when unequal; and
 *   - the draw is never clamped to any protected range: real RR1/RR2
 *     hardware has no notion of where a table ends, so a demand near a
 *     table edge legally draws outside it (WindowSpillsPastRegionEdge) --
 *     the paper's own "boundary effect" (Section V-B). There is no
 *     clamped/unclamped choice here; this is the only behavior, to keep the
 *     model synthesizable (no software-only step a real range-register pair
 *     could not implement).
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

// The random fill is always block-aligned and within the window of the
// demand line. It may legitimately equal the demand itself (see the file
// comment and CollisionTimingSignalVanishesAtFullWindow below). Swept
// across many demand lines, far enough from address 0 that the underflow
// guard never engages, so the +/- window bound is exercised cleanly.
TEST(RandomFillTest, NeighborInvariants)
{
    Random::RandomPtr rng = Random::genRandom(0xC0FFEE);
    const unsigned window = 8;
    const unsigned num_lines = (RegionEnd - RegionBase) / BlkSize;

    for (unsigned dl = 0; dl < num_lines; dl++) {
        const Addr demand = RegionBase + (Addr)dl * BlkSize;
        for (int trial = 0; trial < 200; trial++) {
            const Addr fill = random_fill::pickNeighbor(
                demand, BlkSize, window, window, *rng);

            // block-aligned
            EXPECT_EQ(fill % BlkSize, 0u);
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
    const Addr demand = RegionBase + 20 * BlkSize;

    std::set<Addr> seen;
    bool below = false, above = false, self_seen = false;
    for (int trial = 0; trial < 500; trial++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, window, window, *rng);
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
        demand, BlkSize, /*window_before=*/0, /*window_after=*/0, *rng);
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
    // A window covering a 16-line span, matching the paper's own AES-table
    // case study (Section V.A): a 1 KiB table with 64-byte lines. There is
    // no region to clamp to any more, so the candidate count is always
    // exactly window*2 + 1, regardless of table size.
    const unsigned window = 16;
    constexpr int trials = 200000;

    const unsigned demand_line = 3; // i
    const unsigned other_line = 11; // j != i, arbitrary, within the window
    const Addr demand = RegionBase + (Addr)demand_line * BlkSize;
    const Addr other = RegionBase + (Addr)other_line * BlkSize;
    ASSERT_NE(demand, other);

    int self_hits = 0, other_hits = 0;
    for (int t = 0; t < trials; t++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, window, window, *rng);
        self_hits += (fill == demand);
        other_hits += (fill == other);
    }

    const double p1 = (double)self_hits / trials;  // P(collision hit)
    const double p2 = (double)other_hits / trials; // P(no-collision hit)
    const double expected = 1.0 / (2 * window + 1);

    // With 200k trials at p~1/33, the binomial standard error is ~0.0004, so
    // a tolerance of 0.01 (25 SEs) comfortably absorbs sampling noise while
    // still rejecting the large, window-independent gap the exclusion bug
    // produced.
    constexpr double tol = 0.01;
    EXPECT_NEAR(p1, expected, tol) << "P1 (collision) = " << p1;
    EXPECT_NEAR(p2, expected, tol) << "P2 (no collision) = " << p2;
    EXPECT_NEAR(p1, p2, tol)
        << "collision-timing signal P1-P2 = " << (p1 - p2)
        << " (want ~0 -- the demanded line must be a valid draw outcome, "
        << "not excluded)";
}

// RR1/RR2 (Fig. 4) are independently sized, so window_before and
// window_after must be honoured independently, not silently averaged or
// symmetrised. Use a deliberately unequal pair, far from address 0 so the
// underflow guard never engages, and confirm the draw never goes further
// than window_before lines before or window_after lines after, and that
// both boundaries of the asymmetric window are actually reachable.
TEST(RandomFillTest, AsymmetricWindowRespectsIndependentBounds)
{
    Random::RandomPtr rng = Random::genRandom(0xABCD);
    const unsigned window_before = 2;
    const unsigned window_after = 10;
    const Addr demand = RegionBase + 20 * BlkSize;

    bool saw_before_edge = false, saw_after_edge = false;
    for (int trial = 0; trial < 2000; trial++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, window_before, window_after, *rng);
        const long long delta =
            ((long long)fill - (long long)demand) / (long long)BlkSize;
        EXPECT_LE(delta, (long long)window_after);
        EXPECT_GE(delta, -(long long)window_before);
        saw_before_edge |= (delta == -(long long)window_before);
        saw_after_edge |= (delta == (long long)window_after);
    }
    EXPECT_TRUE(saw_before_edge);
    EXPECT_TRUE(saw_after_edge);
}

// The central trade-off this file documents: the draw is bounded only by
// address-space underflow, never by a protected range. A demand line right
// at a table's start legally draws candidates *before* RegionBase --
// reproducing the paper's own "boundary effect" (Section V-B) rather than
// hiding it, and matching what real, synthesizable RR1/RR2 hardware (which
// has no register that knows where a table ends) would actually do.
TEST(RandomFillTest, WindowSpillsPastRegionEdge)
{
    Random::RandomPtr rng = Random::genRandom(0x5EED);
    const unsigned window = 8;
    // A demand line right at the region's start: the whole "before" side of
    // its window has nowhere to go but outside the region.
    const Addr demand = RegionBase;

    bool ever_outside = false;
    for (int trial = 0; trial < 2000; trial++) {
        const Addr fill = random_fill::pickNeighbor(
            demand, BlkSize, window, window, *rng);
        ever_outside |= (fill < RegionBase || fill >= RegionEnd);
    }

    EXPECT_TRUE(ever_outside)
        << "a demand at a table's edge must be able to draw outside the "
        << "table (that is the paper-faithful boundary effect, not a bug)";
}
