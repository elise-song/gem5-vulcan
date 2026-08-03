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
 * Keyed index-derivation function (IDF) for the ScatterCache defense.
 *
 * This header is deliberately self-contained (no SimObject dependencies) so
 * that the pure mapping logic can be unit-tested cheaply and reused by the
 * ScatterAssociative indexing policy.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_SCATTER_HASH_HH__
#define __MEM_CACHE_INDEXING_POLICIES_SCATTER_HASH_HH__

#include <cstdint>

namespace gem5
{

/**
 * Deterministically map a (line address, way, per-domain key) triple to a wide
 * hash value. The caller keeps the low log2(numSets) bits as the set index for
 * that way.
 *
 * This is a lightweight, non-cryptographic mixing function used as a
 * functional model of ScatterCache's keyed index-derivation function. The
 * result provably depends on all three inputs:
 *   - a different domain @p key yields a different address->set mapping
 *     (this is what makes cross-domain eviction-set construction infeasible),
 *   - a different @p way yields a different set (per-way skewing, so a single
 *     address occupies an unpredictable, key-dependent set in every way),
 *   - it is stable for a fixed (key, way, line_addr) so that lookups,
 *     writebacks and invalidations all agree on the block's location.
 *
 * @param line_addr The block (line) address, i.e. address with the block
 *                  offset shifted out.
 * @param way The cache way the index is being computed for.
 * @param key The per-domain secret key. Callers must use a non-zero key with
 *            adequate entropy; a zero/constant key would collapse the mapping.
 * @return A 64-bit hash whose low bits form the set index.
 */
inline uint64_t
scatterIndexHash(uint64_t line_addr, uint32_t way, uint64_t key)
{
    // Fold the address, the domain key and a per-way sub-key together. The odd
    // multipliers keep every input bit live; the way is offset by one so way 0
    // is not a no-op, and the key is mixed in twice (once multiplicatively,
    // once by a shifted xor) so the mapping can never become key-independent.
    uint64_t h = line_addr;
    h += 0x9E3779B97F4A7C15ULL * (key | 1ULL);
    h ^= (uint64_t)(way + 1) * 0xD1B54A32D192ED03ULL;
    h ^= key >> 23;

    // MurmurHash3-style 64-bit finalizer for good avalanche, so that the low
    // set-index bits depend on the full address, key and way.
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33;
    h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;
    return h;
}

} // namespace gem5

#endif // __MEM_CACHE_INDEXING_POLICIES_SCATTER_HASH_HH__
