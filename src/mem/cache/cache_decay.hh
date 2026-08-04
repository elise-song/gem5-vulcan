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
 * Pure helper logic for the Non-deterministic cache via Cache Decay
 * (Keramidas, Antonopoulos, Serpanos, Kaxiras, "Non deterministic caches: a
 * simple and effective defense against side channel attacks", DAES 2008).
 *
 * Cache Decay assigns every line a RANDOMIZED lifetime; when that lifetime
 * elapses the line self-invalidates. Randomizing the per-line lifetimes makes
 * occupancy/eviction non-deterministic, so a contention/reuse attacker sees
 * noisy cache state.
 *
 * These functions are kept free of any SimObject/cache state so that the
 * production cache (BaseCache) and the directed gtest exercise exactly the
 * same lifetime/deadline arithmetic without constructing a full cache. All
 * the randomness lives in the injected @c rng, so the tests can drive it
 * deterministically from a seed.
 */

#ifndef __MEM_CACHE_CACHE_DECAY_HH__
#define __MEM_CACHE_CACHE_DECAY_HH__

#include <cassert>

#include "base/random.hh"
#include "base/types.hh"

namespace gem5
{

namespace cache_decay
{

/**
 * Draw a single randomized decay lifetime, in ticks.
 *
 * The lifetime is uniform over [interval - range, interval + range], clamped
 * so it is always >= 1 tick (a decayed line must always live at least one
 * tick into the future). When @p range is 0 the draw is deterministic and
 * equals @p interval, which is why a non-zero range is what makes the defense
 * genuinely non-deterministic.
 *
 * @param rng      Random number source (all randomness comes from here).
 * @param interval Base/mean lifetime in ticks (> 0).
 * @param range    Randomization half-width in ticks (>= 0).
 * @return A lifetime in ticks in [max(1, interval - range), interval + range].
 */
inline Tick
decayLifetime(Random &rng, Tick interval, Tick range)
{
    assert(interval > 0);
    assert(range >= 0);

    // Lowest legal lifetime is 1 tick; never draw a non-positive lifetime.
    const Tick lo = (interval > range) ? (interval - range) : 1;
    const Tick hi = interval + range;
    // hi >= lo always holds: if interval > range then hi - lo = 2*range >= 0;
    // otherwise lo == 1 and hi = interval + range >= interval >= 1.
    assert(hi >= lo);

    return rng.random<Tick>(lo, hi);
}

/**
 * Compute the absolute decay deadline for a line inserted/touched at @p now,
 * given a freshly drawn @p lifetime. Kept as a trivial, named helper so the
 * "deadline = now + randomized lifetime" contract is exercised by the gtest.
 *
 * @param now      The current tick.
 * @param lifetime A lifetime drawn from decayLifetime() (>= 1).
 * @return The absolute tick at which the line should self-invalidate.
 */
inline Tick
decayDeadline(Tick now, Tick lifetime)
{
    assert(lifetime >= 1);
    return now + lifetime;
}

} // namespace cache_decay

} // namespace gem5

#endif // __MEM_CACHE_CACHE_DECAY_HH__
