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
 * This draws uniformly from the window [demand_blk_addr - window,
 * demand_blk_addr + window] (clamped to the containing protected range),
 * matching the random fill engine of Liu & Lee, "Random Fill Cache
 * Architecture" (MICRO 2014), Section IV.B.2. Per the paper's Eq. 6, the
 * window's (a+b+1) candidates are equally likely *including* the demanded
 * line itself: that is what makes the fill statistically indistinguishable
 * between "the victim reused this exact line" (P1) and "the victim touched a
 * different, in-window line" (P2) once the window covers the whole
 * security-critical region, closing the collision-timing channel. Excluding
 * the demanded line from the draw would make P1 always 0 while P2 stays
 * positive -- reopening exactly that channel. The caller is responsible for
 * handling the case where the draw comes back equal to demand_blk_addr: no
 * separate injection is needed (or possible) then, since the ordinary demand
 * fill already targets that same address.
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
 *     region); it may equal @p demand_blk_addr itself.
 *
 * @param demand_blk_addr Block-aligned physical address of the protected
 *                        demand access.
 * @param blk_size        Cache block size in bytes (> 0).
 * @param region_lo       Block-aligned start of the safe region (inclusive).
 * @param region_hi       Block-aligned end of the safe region (exclusive);
 *                        must hold at least one line and contain
 *                        demand_blk_addr.
 * @param window          Neighbourhood radius in lines.
 * @param rng             Random number source.
 * @return The block-aligned address of the line to random-fill; may equal
 *         demand_blk_addr.
 */
inline Addr
pickNeighbor(Addr demand_blk_addr, unsigned blk_size,
             Addr region_lo, Addr region_hi, unsigned window, Random &rng)
{
    assert(blk_size > 0);
    assert(demand_blk_addr % blk_size == 0);
    assert(region_hi > region_lo);
    assert(demand_blk_addr >= region_lo && demand_blk_addr < region_hi);

    const unsigned num_lines = (region_hi - region_lo) / blk_size;
    const unsigned demand_line = (demand_blk_addr - region_lo) / blk_size;

    // Clamp the +/- window to the region as an inclusive [lo, hi] range of
    // line indices. The demanded line itself (offset 0) is always one of the
    // (hi - lo + 1) equally likely candidates.
    const unsigned lo = (demand_line > window) ? (demand_line - window) : 0;
    const unsigned hi = (demand_line + window < num_lines) ?
        (demand_line + window) : (num_lines - 1);

    const unsigned line = lo + rng.random<unsigned>(0, hi - lo);
    return region_lo + (Addr)line * blk_size;
}

} // namespace random_fill

} // namespace gem5

#endif // __MEM_CACHE_RANDOM_FILL_HH__
