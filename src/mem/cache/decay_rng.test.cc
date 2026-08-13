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
 * Demonstrates, directly against gem5::Random (the class BaseCache's
 * decayRng uses), the bug fixed in the Cache Decay RNG hardening and the
 * mechanism of the fix. This does NOT prove decayRng is cryptographically
 * secure -- it is not, and is not claimed to be. It shows two narrower,
 * checkable facts: the old seeding produced identical streams across cache
 * instances, and the new seeding/reseeding does not.
 */

#include <cstdint>
#include <random>
#include <vector>

#include "base/random.hh"
#include "gtest/gtest.h"

using namespace gem5;

namespace
{

std::vector<uint64_t>
drawSequence(const Random::RandomPtr &rng, int n)
{
    std::vector<uint64_t> out;
    for (int i = 0; i < n; i++) {
        out.push_back(rng->random<uint64_t>(0, UINT64_MAX - 1));
    }
    return out;
}

} // namespace

// Pre-fix behaviour: Random::genRandom() with no argument always seeds from
// the shared Random::globalSeed. Two independently constructed caches (both
// calling genRandom() this way) would therefore draw byte-for-byte the same
// decay-lifetime sequence.
TEST(DecayRngHardening, DefaultSeedCollidedAcrossInstances)
{
    Random::RandomPtr a = Random::genRandom();
    Random::RandomPtr b = Random::genRandom();
    EXPECT_EQ(drawSequence(a, 8), drawSequence(b, 8));
}

// Post-fix behaviour: each cache draws its own seed from OS entropy
// (std::random_device), so two instances no longer collide.
TEST(DecayRngHardening, EntropySeedDecorrelatesInstances)
{
    std::random_device rd;
    Random::RandomPtr a = Random::genRandom(rd());
    Random::RandomPtr b = Random::genRandom(rd());
    EXPECT_NE(drawSequence(a, 8), drawSequence(b, 8));
}

// Post-fix behaviour: drawDecayLifetime()'s periodic decayRng->init(...)
// reseed changes the subsequent output stream, so an observer who inferred
// something about the pre-reseed state gains nothing about post-reseed
// draws.
TEST(DecayRngHardening, ReseedChangesFutureOutputs)
{
    Random::RandomPtr a = Random::genRandom(12345);
    std::vector<uint64_t> pre = drawSequence(a, 4);
    a->init(67890);
    std::vector<uint64_t> post = drawSequence(a, 4);
    EXPECT_NE(pre, post);
}
