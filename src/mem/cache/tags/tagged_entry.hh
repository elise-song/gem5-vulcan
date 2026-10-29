/**
 * Copyright (c) 2024 Arm Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2020 Inria
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

#ifndef __CACHE_TAGGED_ENTRY_HH__
#define __CACHE_TAGGED_ENTRY_HH__

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "base/cprintf.hh"
#include "base/random.hh"
#include "base/types.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "params/TaggedIndexingPolicy.hh"
#include "params/TaggedSetAssociative.hh"

namespace gem5
{

class TaggedTypes
{
  public:
    struct KeyType
    {
        Addr address;
        bool secure;
        /**
         * Requestor context of the lookup, used by context-keyed indexing
         * to select a per-context address->set mapping. Defaults to
         * InvalidContextID so that the many existing two-field brace
         * initialisers ({address, secure}) keep compiling unchanged;
         * indexing policies that do not key off the context ignore it.
         */
        ContextID context = InvalidContextID;
    };
    using Params = TaggedIndexingPolicyParams;
};

using TaggedIndexingPolicy = IndexingPolicyTemplate<TaggedTypes>;
template class IndexingPolicyTemplate<TaggedTypes>;

/**
 * Deterministically map a (line address, way, key) triple to a wide hash
 * value. The caller keeps the low log2(numSets) bits as the set index for
 * that way.
 *
 * This is a lightweight, non-cryptographic mixing function. The result
 * depends on all three inputs: a different @p key yields a different
 * address->set mapping, a different @p way yields a different set (per-way
 * skewing), and it is stable for a fixed (key, way, line_addr) so that
 * lookups, writebacks and invalidations all agree on the block's location.
 *
 * @param line_addr The block (line) address, i.e. address with the block
 *                  offset shifted out.
 * @param way The cache way the index is being computed for.
 * @param key The key selected for this lookup. Callers must use a non-zero
 *            key with adequate entropy; a zero/constant key would collapse
 *            the mapping.
 * @return A 64-bit hash whose low bits form the set index.
 */
inline uint64_t
wayIndexHash(uint64_t line_addr, uint32_t way, uint64_t key)
{
    // Fold the address, the key and a per-way sub-key together. The odd
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

/**
 * This version of set associative indexing deals with
 * a Lookup structure made of address and secure bit.
 * It extracts the address but discards the secure bit which
 * is used for tagging only.
 *
 * With context_keyed enabled the policy becomes skewed-associative with a
 * keyed index: each way's set index is wayIndexHash(line address, way, key)
 * where the key is selected by the requestor context of the lookup
 * (KeyType::context). Every context is assigned its own high-entropy key on
 * first use (or a fixed one from context_keys), so the same address maps to
 * a different set in every way for every context. The full line address is
 * then kept as the tag (extractTag() drops only the block offset), so that
 * regenerateAddr() reads the address straight out of the tag and never has
 * to invert the hash.
 */
class TaggedSetAssociative : public TaggedIndexingPolicy
{
  protected:
    /** Whether the per-context keyed, per-way skewed index is enabled. */
    const bool contextKeyed;

    /**
     * Per-context keys, generated lazily the first time a context is
     * seen. Mutable because it is populated from the const lookup path.
     */
    mutable std::unordered_map<ContextID, uint64_t> contextKeys;

    /**
     * Optional fixed keys (from the Python param) indexed by context, used
     * to make experiments reproducible. Contexts beyond this vector, or
     * with a zero entry, get a fresh random key instead.
     */
    const std::vector<uint64_t> fixedKeys;

    /** RNG used to generate per-context keys with adequate entropy. */
    Random::RandomPtr rng;

    /**
     * Experiment-only override of the active context. When set (via
     * setActiveContextAll(), typically from a config), it takes precedence
     * over the per-request contextId so that a traffic-generator workload,
     * whose requests carry no context, can still drive distinct keys.
     */
    bool activeContextSet = false;
    ContextID activeContext = 0;

    /** Registry of live context-keyed instances, so the override can be
     * applied globally. */
    static std::vector<TaggedSetAssociative*> allPolicies;

    /**
     * Resolve the effective context for a lookup: the experiment override
     * if one is active, otherwise the request's contextId (falling back to
     * context 0 for requests that carry none).
     */
    ContextID
    resolveContext(ContextID raw) const
    {
        if (activeContextSet) {
            return activeContext;
        }
        return (raw != InvalidContextID) ? raw : 0;
    }

    /** Get (generating on first use) the key for a context. */
    uint64_t getContextKey(ContextID context) const;

    /**
     * Keyed set index for a given lookup key and way. The full line address
     * and the resolved context's key are mixed together, so the mapping is
     * address-, way- and context-specific.
     */
    uint32_t
    extractSet(const KeyType &key, const uint32_t way) const
    {
        const uint64_t line_addr = key.address >> setShift;
        const uint64_t ckey = getContextKey(resolveContext(key.context));
        return wayIndexHash(line_addr, way, ckey) & setMask;
    }

    /** Plain set index (context_keyed disabled). */
    virtual uint32_t
    extractSet(const KeyType &key) const
    {
        return (key.address >> setShift) & setMask;
    }

  public:
    PARAMS(TaggedSetAssociative);
    TaggedSetAssociative(const Params &p);
    ~TaggedSetAssociative();

    /**
     * With context_keyed the tag holds the whole line address (only the
     * block offset is dropped), so that address reconstruction never needs
     * to invert the keyed hash. Otherwise the usual tag.
     */
    Addr extractTag(const Addr addr) const override;

    /**
     * Find all possible entries for insertion and replacement of an
     * address. With context_keyed: one candidate per way, at the keyed set
     * index for that way.
     */
    std::vector<ReplaceableEntry*>
    getPossibleEntries(const KeyType &key) const override;

    /** Regenerate an entry's block address from its tag. */
    Addr regenerateAddr(const KeyType &key,
                        const ReplaceableEntry *entry) const override;

    /**
     * Set the experiment override context on every live context-keyed
     * policy (exposed to Python as _m5.sim.setIndexContext). Subsequent
     * accesses use this context's key regardless of the request's
     * contextId.
     */
    static void setActiveContextAll(ContextID context);

    /** Clear the experiment override on every live policy. */
    static void clearActiveContextAll();

    /**
     * Test/analysis helper: the vector of per-way set indices that @p addr
     * maps to under @p context's key.
     */
    std::vector<uint32_t> setIndexVector(Addr addr, ContextID context) const;
};

/**
 * A tagged entry is an entry containing a tag. Each tag is accompanied by a
 * secure bit, which informs whether it belongs to a secure address space.
 * A tagged entry's contents are only relevant if it is marked as valid.
 */
class TaggedEntry : public ReplaceableEntry
{
  public:
    using KeyType = TaggedTypes::KeyType;
    using IndexingPolicy = TaggedIndexingPolicy;
    using TagExtractor = std::function<Addr(Addr)>;

    TaggedEntry()
      : _valid(false), _secure(false), _tag(MaxAddr)
    {}
    ~TaggedEntry() = default;

    void
    registerTagExtractor(TagExtractor ext)
    {
        extractTag = ext;
    }

    /**
     * Checks if the entry is valid.
     *
     * @return True if the entry is valid.
     */
    virtual bool isValid() const { return _valid; }

    /**
     * Check if this block holds data from the secure memory space.
     *
     * @return True if the block holds data from the secure memory space.
     */
    bool isSecure() const { return _secure; }

    /**
     * Get tag associated to this block.
     *
     * @return The tag value.
     */
    virtual Addr getTag() const { return _tag; }

    /**
     * Checks if the given tag information corresponds to this entry's.
     *
     * @param tag The tag value to compare to.
     * @param is_secure Whether secure bit is set.
     * @return True if the tag information match this entry's.
     */
    bool
    match(const KeyType &key) const
    {
        assert(extractTag);
        return isValid() && (getTag() == extractTag(key.address)) &&
            (isSecure() == key.secure);
    }

    /**
     * Insert the block by assigning it a tag and marking it valid. Touches
     * block if it hadn't been touched previously.
     *
     * @param tag The tag value.
     */
    virtual void
    insert(const KeyType &key)
    {
        assert(extractTag);
        setValid();
        setTag(extractTag(key.address));
        if (key.secure) {
            setSecure();
        }
    }

    /** Invalidate the block. Its contents are no longer valid. */
    virtual void invalidate()
    {
        _valid = false;
        setTag(MaxAddr);
        clearSecure();
    }

    std::string
    print() const override
    {
        return csprintf("tag: %#x secure: %d valid: %d | %s", getTag(),
            isSecure(), isValid(), ReplaceableEntry::print());
    }

  protected:
    /**
     * Set tag associated to this block.
     *
     * @param tag The tag value.
     */
    virtual void setTag(Addr tag) { _tag = tag; }

    /** Set secure bit. */
    virtual void setSecure() { _secure = true; }

    /** Clear secure bit. Should be only used by the invalidation function. */
    void clearSecure() { _secure = false; }

    /** Set valid bit. The block must be invalid beforehand. */
    virtual void
    setValid()
    {
        assert(!isValid());
        _valid = true;
    }

    /** Callback used to extract the tag from the entry */
    TagExtractor extractTag;

  private:
    /**
     * Valid bit. The contents of this entry are only valid if this bit is set.
     * @sa invalidate()
     * @sa insert()
     */
    bool _valid;

    /**
     * Secure bit. Marks whether this entry refers to an address in the secure
     * memory space. Must always be modified along with the tag.
     */
    bool _secure;

    /** The entry's tag. */
    Addr _tag;
};

/**
 * This helper generates an a tag extractor function object
 * which will be typically used by Replaceable entries indexed
 * with the TaggedIndexingPolicy.
 * It allows to "decouple" indexing from tagging. Those entries
 * would call the functor without directly holding a pointer
 * to the indexing policy which should reside in the cache.
 */
static constexpr auto
genTagExtractor(TaggedIndexingPolicy *ip)
{
    return [ip] (Addr addr) { return ip->extractTag(addr); };
}

} // namespace gem5

#endif//__CACHE_TAGGED_ENTRY_HH__
