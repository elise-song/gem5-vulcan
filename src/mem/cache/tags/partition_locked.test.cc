/**
 * Copyright (c) 2026 Aaryan Patel
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

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/partition_locked.hh"

using namespace gem5;

namespace
{

const ContextID OwnerA = 1;
const ContextID OwnerB = 2;

} // namespace

/// The lock bit and owner default to unset, and are cleared by invalidate(),
/// mirroring every other piece of CacheBlk metadata.
TEST(CacheBlkPlMetadataTest, DefaultAndInvalidate)
{
    CacheBlk blk;
    EXPECT_FALSE(blk.isPlLocked());
    EXPECT_EQ(blk.getPlOwner(), InvalidContextID);

    blk.setPlLocked(OwnerA);
    EXPECT_TRUE(blk.isPlLocked());
    EXPECT_EQ(blk.getPlOwner(), OwnerA);

    blk.invalidate();
    EXPECT_FALSE(blk.isPlLocked());
    EXPECT_EQ(blk.getPlOwner(), InvalidContextID);
}

/// isProtectedAddr must match addresses within a configured range and
/// reject addresses immediately outside of it (the range is half-open,
/// [start, end)).
TEST(PartitionLockedTagsTest, IsProtectedAddrBoundaries)
{
    const std::vector<AddrRange> ranges = {
        AddrRange(0x1000, 0x2000),
        AddrRange(0x5000, 0x5100),
    };

    EXPECT_TRUE(PartitionLockedTags::isProtectedAddr(0x1000, ranges));
    EXPECT_TRUE(PartitionLockedTags::isProtectedAddr(0x1fff, ranges));
    EXPECT_TRUE(PartitionLockedTags::isProtectedAddr(0x5080, ranges));

    EXPECT_FALSE(PartitionLockedTags::isProtectedAddr(0x0fff, ranges));
    EXPECT_FALSE(PartitionLockedTags::isProtectedAddr(0x2000, ranges));
    EXPECT_FALSE(PartitionLockedTags::isProtectedAddr(0x3000, ranges));

    EXPECT_FALSE(PartitionLockedTags::isProtectedAddr(0x1500, {}));
}

/// An unlocked candidate is always evictable, regardless of the incoming
/// line.
TEST(PartitionLockedTagsTest, CanEvictUnlockedCandidate)
{
    CacheBlk unlocked;
    EXPECT_TRUE(PartitionLockedTags::canEvict(&unlocked, false,
                                               InvalidContextID));
    EXPECT_TRUE(PartitionLockedTags::canEvict(&unlocked, true, OwnerA));
}

/// A locked candidate cannot be evicted by a non-locked incoming line.
TEST(PartitionLockedTagsTest, CanEvictLockedByNonLockedDenied)
{
    CacheBlk locked;
    locked.setPlLocked(OwnerA);
    EXPECT_FALSE(PartitionLockedTags::canEvict(&locked, false,
                                                InvalidContextID));
}

/// A locked candidate cannot be evicted by a locked incoming line owned by
/// a different context.
TEST(PartitionLockedTagsTest, CanEvictLockedByDifferentOwnerDenied)
{
    CacheBlk locked;
    locked.setPlLocked(OwnerA);
    EXPECT_FALSE(PartitionLockedTags::canEvict(&locked, true, OwnerB));
}

/// A locked candidate CAN be evicted by a locked incoming line owned by the
/// same context (an owner may reclaim its own locked capacity).
TEST(PartitionLockedTagsTest, CanEvictLockedBySameOwnerAllowed)
{
    CacheBlk locked;
    locked.setPlLocked(OwnerA);
    EXPECT_TRUE(PartitionLockedTags::canEvict(&locked, true, OwnerA));
}

/// Fixture simulating a fully-associative set under replacement pressure:
/// one PL-locked line (owner A) sharing the set with several unlocked
/// lines. CacheBlk deliberately disables copy/move construction (it is
/// meant to live in a fixed tag-store array), so candidates are held as
/// unique_ptrs rather than by value.
class PartitionLockedTagsFilterTest : public ::testing::Test
{
  protected:
    CacheBlk lockedBlk;
    std::vector<std::unique_ptr<CacheBlk>> unlockedBlks;
    std::vector<ReplaceableEntry *> entries;

    void
    SetUp() override
    {
        lockedBlk.setPlLocked(OwnerA);

        for (int i = 0; i < 4; i++) {
            unlockedBlks.push_back(std::make_unique<CacheBlk>());
        }

        entries.push_back(&lockedBlk);
        for (auto &blk : unlockedBlks) {
            entries.push_back(blk.get());
        }
    }
};

/// (a) Under repeated replacement pressure from non-locked incoming lines,
/// the PL-locked line is never left as a replacement candidate, so it can
/// never be chosen as a victim by the replacement policy.
TEST_F(PartitionLockedTagsFilterTest, LockedNeverEvictedUnderPressure)
{
    for (int access = 0; access < 8; access++) {
        std::vector<ReplaceableEntry *> candidates = entries;
        PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/
                                              false, InvalidContextID);

        // The locked block must never survive as a candidate...
        EXPECT_EQ(std::find(candidates.begin(), candidates.end(),
                             &lockedBlk),
                  candidates.end());

        // ...while every unlocked block remains eligible.
        ASSERT_EQ(candidates.size(), unlockedBlks.size());
        for (auto &blk : unlockedBlks) {
            EXPECT_NE(std::find(candidates.begin(), candidates.end(),
                                 blk.get()),
                      candidates.end());
        }
    }
}

/// A locked incoming line from a different owner also cannot dislodge the
/// resident locked line.
TEST_F(PartitionLockedTagsFilterTest, LockedNeverEvictedByDifferentOwner)
{
    std::vector<ReplaceableEntry *> candidates = entries;
    PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/true,
                                          OwnerB);

    EXPECT_EQ(std::find(candidates.begin(), candidates.end(), &lockedBlk),
              candidates.end());
    EXPECT_EQ(candidates.size(), unlockedBlks.size());
}

/// A locked incoming line from the SAME owner may reclaim its own locked
/// line.
TEST_F(PartitionLockedTagsFilterTest, LockedEvictedBySameOwner)
{
    std::vector<ReplaceableEntry *> candidates = entries;
    PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/true,
                                          OwnerA);

    EXPECT_NE(std::find(candidates.begin(), candidates.end(), &lockedBlk),
              candidates.end());
    EXPECT_EQ(candidates.size(), entries.size());
}

/// (b) When every candidate in the set is a locked line the incoming
/// request is not allowed to evict, no eligible victim remains. This is
/// exactly the condition PartitionLockedTags::findVictim uses to bypass
/// the cache (return nullptr) instead of evicting protected data.
TEST(PartitionLockedTagsTest, AllLockedAndUnevictableYieldsNoCandidates)
{
    CacheBlk a, b, c;
    a.setPlLocked(OwnerA);
    b.setPlLocked(OwnerA);
    c.setPlLocked(OwnerA);
    std::vector<ReplaceableEntry *> candidates = {&a, &b, &c};

    // Incoming line is unlocked: none of the locked, same-owner-only lines
    // may be evicted.
    PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/
                                          false, InvalidContextID);
    EXPECT_TRUE(candidates.empty());

    // Incoming line is locked, but owned by a different context: still no
    // eligible victim.
    candidates = {&a, &b, &c};
    PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/true,
                                          OwnerB);
    EXPECT_TRUE(candidates.empty());
}

/// A mix of same- and different-owner locked lines: only the incoming
/// owner's own locked lines are eligible, everything else is bypassed.
TEST(PartitionLockedTagsTest, MixedOwnersOnlySameOwnerEvictable)
{
    CacheBlk ownedByA, ownedByB;
    ownedByA.setPlLocked(OwnerA);
    ownedByB.setPlLocked(OwnerB);
    std::vector<ReplaceableEntry *> candidates = {&ownedByA, &ownedByB};

    PartitionLockedTags::filterEvictable(candidates, /*incoming_locked=*/true,
                                          OwnerA);

    ASSERT_EQ(candidates.size(), 1u);
    EXPECT_EQ(candidates[0], &ownedByA);
}
