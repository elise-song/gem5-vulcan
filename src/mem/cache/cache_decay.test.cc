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
 * Directed tests for the Non-deterministic (Cache Decay) helper logic. These
 * exercise the exact production functions used by BaseCache (there is no
 * reimplementation here), verifying:
 *   - a drawn lifetime always lands in the configured randomized window and
 *     is always positive (a decayed line lives at least one tick),
 *   - over many draws the lifetime genuinely varies (the defense is
 *     non-deterministic, not a fixed constant),
 *   - a zero randomization range collapses to a deterministic lifetime, and
 *   - the decay deadline is exactly "now + randomized lifetime".
 */

#include <gtest/gtest.h>

#include <set>

#include "base/random.hh"
#include "base/types.hh"
#include "mem/cache/cache_decay.hh"

using namespace gem5;

namespace {

// 500ns base, 250ns half-width at a 1ps tick (the Cache.py defaults).
constexpr Tick Interval = 500000;
constexpr Tick Range = 250000;

} // namespace

// A drawn lifetime is always within [interval - range, interval + range] and
// always strictly positive, swept over many RNG draws.
TEST(CacheDecayTest, LifetimeWithinWindow)
{
    Random::RandomPtr rng = Random::genRandom(0xDECA1);
    const Tick lo = Interval - Range;
    const Tick hi = Interval + Range;

    for (int i = 0; i < 10000; i++) {
        const Tick life = cache_decay::decayLifetime(*rng, Interval, Range);
        EXPECT_GE(life, lo);
        EXPECT_LE(life, hi);
        EXPECT_GT(life, 0);
    }
}

// The randomization is real: over many draws we see many distinct lifetimes
// spanning both halves of the window (not a fixed value).
TEST(CacheDecayTest, LifetimeIsRandomised)
{
    Random::RandomPtr rng = Random::genRandom(0xB33F);
    std::set<Tick> seen;
    bool below_mean = false, above_mean = false;
    for (int i = 0; i < 2000; i++) {
        const Tick life = cache_decay::decayLifetime(*rng, Interval, Range);
        seen.insert(life);
        below_mean |= (life < Interval);
        above_mean |= (life > Interval);
    }
    EXPECT_GT(seen.size(), 100u);
    EXPECT_TRUE(below_mean);
    EXPECT_TRUE(above_mean);
}

// A zero range collapses to a deterministic lifetime equal to the interval.
TEST(CacheDecayTest, ZeroRangeIsDeterministic)
{
    Random::RandomPtr rng = Random::genRandom(0x5EED);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(cache_decay::decayLifetime(*rng, Interval, 0), Interval);
    }
}

// When the interval is small enough that interval - range would be <= 0, the
// lifetime is clamped to a floor of 1 tick and never draws a non-positive
// value.
TEST(CacheDecayTest, SmallIntervalClampsToPositive)
{
    Random::RandomPtr rng = Random::genRandom(0xF10);
    // interval (100) < range (1000): lower bound must clamp to 1.
    for (int i = 0; i < 5000; i++) {
        const Tick life = cache_decay::decayLifetime(*rng, 100, 1000);
        EXPECT_GE(life, 1);
        EXPECT_LE(life, 1100);
    }
}

// The deadline is exactly now + lifetime.
TEST(CacheDecayTest, DeadlineIsNowPlusLifetime)
{
    Random::RandomPtr rng = Random::genRandom(0xDEAD11);
    const Tick now = 1234567;
    for (int i = 0; i < 1000; i++) {
        const Tick life = cache_decay::decayLifetime(*rng, Interval, Range);
        EXPECT_EQ(cache_decay::decayDeadline(now, life), now + life);
    }
}
