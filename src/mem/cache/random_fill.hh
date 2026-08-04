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
 * Pure helper logic for the Random Fill secure cache (Liu & Lee, MICRO
 * 2014). These functions are kept free of any SimObject/cache state so that
 * the production cache (BaseCache) and the directed gtest exercise exactly
 * the same code without needing to construct a full cache instance.
 */

#ifndef __MEM_CACHE_RANDOM_FILL_HH__
#define __MEM_CACHE_RANDOM_FILL_HH__

#include <cassert>
#include <vector>

#include "base/addr_range.hh"
#include "base/intmath.hh"
#include "base/random.hh"
#include "base/types.hh"

namespace gem5
{

namespace random_fill
{

/**
 * Is the (block-aligned) address covered by any protected range?
 *
 * @param ranges   The configured protected address ranges.
 * @param blk_addr A block-aligned physical address.
 * @return true iff blk_addr falls inside at least one protected range.
 */
inline bool
isProtected(const std::vector<AddrRange> &ranges, Addr blk_addr)
{
    for (const auto &range : ranges) {
        if (range.contains(blk_addr)) {
            return true;
        }
    }
    return false;
}

/**
 * Pick the block-aligned address of the random-fill line for a protected
 * demand access to @p demand_blk_addr.
 *
 * The random fill is drawn from the same protected range that contains the
 * demand, given here as the block-aligned half-open bounds
 * [@p region_lo, @p region_hi). Because that range is memory the
 * configuration has explicitly marked as protected, every candidate address
 * is guaranteed legal and cacheable (it can never fault or hit MMIO). The
 * returned address is guaranteed to be:
 *   - block-aligned,
 *   - inside [region_lo, region_hi),
 *   - within +/- @p window lines of @p demand_blk_addr (clamped to the
 *     region),
 *   - never equal to @p demand_blk_addr itself (caching the demanded line
 *     would defeat the defense).
 *
 * @param demand_blk_addr Block-aligned physical address of the protected
 *                        demand access.
 * @param blk_size        Cache block size in bytes (> 0).
 * @param region_lo       Block-aligned start of the safe region (inclusive).
 * @param region_hi       Block-aligned end of the safe region (exclusive);
 *                        the region must hold at least two lines.
 * @param window          Neighbourhood radius in lines.
 * @param rng             Random number source.
 * @return The block-aligned address of the line to random-fill.
 */
inline Addr
pickNeighbor(Addr demand_blk_addr, unsigned blk_size,
             Addr region_lo, Addr region_hi, unsigned window, Random &rng)
{
    assert(blk_size > 0);
    assert(demand_blk_addr % blk_size == 0);
    assert(demand_blk_addr >= region_lo && demand_blk_addr < region_hi);

    const unsigned num_lines = (region_hi - region_lo) / blk_size;
    assert(num_lines >= 2);
    const unsigned demand_line = (demand_blk_addr - region_lo) / blk_size;

    // Clamp the +/- window to the region as an inclusive [lo, hi] range of
    // line indices.
    const unsigned lo = (demand_line > window) ? (demand_line - window) : 0;
    const unsigned hi = (demand_line + window < num_lines) ?
        (demand_line + window) : (num_lines - 1);

    // Candidate lines are [lo, hi] excluding demand_line; there is always at
    // least one because the region holds >= 2 lines.
    const unsigned num_candidates = (hi - lo + 1) - 1;

    if (num_candidates == 0) {
        // Degenerate window (e.g. window == 0): still return a distinct,
        // in-region neighbour so the demanded line is never cached.
        return (demand_line + 1 < num_lines) ?
            region_lo + (Addr)(demand_line + 1) * blk_size :
            region_lo + (Addr)(demand_line - 1) * blk_size;
    }

    // Draw uniformly from the candidates and map the pick to a line index,
    // skipping the demanded line.
    unsigned line = lo + rng.random<unsigned>(0, num_candidates - 1);
    if (line >= demand_line) {
        line += 1;
    }
    return region_lo + (Addr)line * blk_size;
}

} // namespace random_fill

} // namespace gem5

#endif // __MEM_CACHE_RANDOM_FILL_HH__
