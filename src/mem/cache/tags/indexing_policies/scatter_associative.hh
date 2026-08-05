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
 * Declaration of the ScatterCache randomized skewed-associative indexing
 * policy (Werner et al., "ScatterCache: Thwarting Cache Attacks via Cache Set
 * Randomization", USENIX Security 2019).
 *
 * This revision replaces the original per-domain key bookkeeping (an
 * unbounded software map keyed by ContextID) with a small resident key
 * table, modeling the fact that a real key-derivation unit only keeps a
 * bounded amount of key state on-chip rather than growing without limit as
 * new security domains are observed.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__
#define __MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/random.hh"
#include "base/types.hh"
#include "mem/cache/tags/indexing_policies/scatter_hash.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/ScatterAssociative.hh"

namespace gem5
{

class ReplaceableEntry;

/**
 * ScatterCache indexing policy: a skewed-associative cache in which the set
 * index for each way is produced by a KEYED index-derivation function whose
 * key depends on the requesting SECURITY DOMAIN (req->contextId()).
 *
 * Because every domain is (in principle) assigned a different, high-entropy
 * key, the same address maps to a different, unpredictable set in every way
 * for every domain. Two domains therefore almost never share a full set of
 * congruent lines, which makes cross-domain eviction-set construction
 * infeasible and defeats eviction-based attacks (Prime+Probe, Evict+Time,
 * ...). No line is ever locked or made non-evictable; the protection comes
 * entirely from the randomized mapping.
 *
 * Unlike the conventional gem5 indexing policies, the full line address is
 * kept as the tag (extractTag() drops only the block offset). This is what
 * ScatterCache does in hardware: because the set index is a hash of the whole
 * address, address reconstruction (regenerateAddr()) reads the address
 * straight out of the tag and is therefore exact and independent of the keyed
 * set mapping -- there is no hash to invert, so writebacks/snoops always
 * reconstruct the correct physical address.
 */
class ScatterAssociative : public TaggedIndexingPolicy
{
  protected:
    using KeyType = TaggedTypes::KeyType;

    /**
     * The key-derivation unit keeps one resident key per active hardware
     * thread context sharing this cache level, not one per software-visible
     * security domain -- domains can vastly outnumber the physical thread
     * contexts that actually drive requests into a given cache instance, so
     * sizing key storage off the domain count would leave it unbounded.
     *
     * kThreadContextsPerCache is the number of hardware thread contexts this
     * cache instance serves; kKeysPerThreadContext is the number of resident
     * keys kept per context (a single key per context keeps the unit's local
     * key storage trivially small). A domain is mapped onto a resident slot
     * via (domain % kKeyTableSlots), direct-mapped-cache style.
     */
    static constexpr std::size_t kThreadContextsPerCache = 1;
    static constexpr std::size_t kKeysPerThreadContext = 1;
    static constexpr std::size_t kKeyTableSlots =
        kThreadContextsPerCache * kKeysPerThreadContext;

    /**
     * Resident key table. keyTable_[slot] == 0 means "unpopulated"; a
     * populated slot holds whichever domain's key last resolved into it.
     * Mutable because it is populated from the const lookup path.
     */
    mutable std::vector<uint64_t> keyTable_;

    /**
     * Optional fixed keys (from the Python `keys` param), indexed by security
     * domain, used to make experiments reproducible. A domain beyond this
     * vector, or with a zero entry, is assigned a fresh random key instead.
     */
    const std::vector<uint64_t> fixedKeys_;

    /** RNG used to generate per-domain keys with adequate entropy. */
    Random::RandomPtr rng_;

    /**
     * Experiment-only override of the active security domain. When set (via
     * setActiveDomainAll(), typically from the config between attack phases),
     * it takes precedence over the per-request contextId so that a
     * traffic-generator workload can drive distinct domains. Defaults to
     * "not set", in which case the domain is taken from req->contextId().
     */
    bool overrideActive_ = false;
    ContextID overrideDomain_ = 0;

    /** Registry of live instances, so the override can be applied globally. */
    static std::vector<ScatterAssociative *> liveInstances_;

    /** Map a security domain onto its resident key-table slot. */
    std::size_t
    slotFor(ContextID domain) const
    {
        return static_cast<std::size_t>(domain) % kKeyTableSlots;
    }

    /**
     * Resolve the effective security domain for a lookup: the experiment
     * override if one is active, otherwise the request's contextId (falling
     * back to domain 0 for requests that carry no context).
     */
    ContextID
    normalizeDomain(ContextID raw) const
    {
        if (overrideActive_) {
            return overrideDomain_;
        }
        return (raw != InvalidContextID) ? raw : 0;
    }

    /**
     * Get (populating on first use) the resident key for a security domain.
     */
    uint64_t keyForDomain(ContextID domain) const;

    /**
     * Apply the keyed index-derivation function to obtain the set for a given
     * key/way. The full line address and the resolved domain's key are mixed
     * together, so the mapping is address-, way- and domain-specific.
     */
    uint32_t
    computeWaySet(const KeyType &key, const uint32_t way) const
    {
        const uint64_t line_addr = key.address >> setShift;
        const uint64_t dkey = keyForDomain(normalizeDomain(key.domain));
        return scatterIndexHash(line_addr, way, dkey) & setMask;
    }

  public:
    PARAMS(ScatterAssociative);

    ScatterAssociative(const Params &p);
    ~ScatterAssociative();

    /**
     * The tag holds the whole line address (only the block offset is dropped),
     * so that address reconstruction never needs to invert the keyed hash.
     */
    Addr
    extractTag(const Addr addr) const override
    {
        return addr >> setShift;
    }

    /**
     * Find all possible entries for insertion and replacement of an address:
     * one candidate per way, at the keyed set index for that way.
     */
    std::vector<ReplaceableEntry*>
    getPossibleEntries(const KeyType &key) const override;

    /**
     * Regenerate an entry's block address from its (full) tag. Independent of
     * the keyed mapping: the address is simply the tag shifted back up.
     */
    Addr
    regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const override;

    /**
     * Set the experiment override domain on every live ScatterAssociative
     * (exposed to Python as _m5.sim.scatterSetDomain). Subsequent accesses use
     * this domain's key regardless of the request's contextId.
     */
    static void setActiveDomainAll(ContextID domain);

    /** Clear the experiment override on every live ScatterAssociative. */
    static void clearActiveDomainAll();

    /**
     * Test/analysis helper: the vector of per-way set indices that @p addr
     * maps to under @p domain's key. Exposed so unit tests and configs can
     * inspect the mapping directly.
     */
    std::vector<uint32_t> setIndexVector(Addr addr, ContextID domain) const;
};

} // namespace gem5

#endif // __MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__
