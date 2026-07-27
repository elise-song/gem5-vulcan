/*
 * Copyright (c) 2012 ARM Limited
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
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
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

#include "cpu/o3/rob.hh"

#include <algorithm>
#include <cstdio>
#include <list>

#include "base/logging.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/limits.hh"
#include "debug/Fetch.hh"
#include "debug/ROB.hh"
#include "params/BaseO3CPU.hh"

namespace gem5
{

namespace o3
{

ROB::ROB(CPU *_cpu, const BaseO3CPUParams &params)
    : robPolicy(params.smtROBPolicy),
      cpu(_cpu),
      numEntries(params.numROBEntries),
      squashWidth(params.squashWidth),
      numInstsInROB(0),
      numThreads(params.numThreads),
      stats(_cpu)
{
    assert(!squashWidth.has_value() || (squashWidth > 0));
    //Figure out rob policy
    if (robPolicy == SMTQueuePolicy::Dynamic) {
        //Set Max Entries to Total ROB Capacity
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = numEntries;
        }

    } else if (robPolicy == SMTQueuePolicy::Partitioned) {
        DPRINTF(Fetch, "ROB sharing policy set to Partitioned\n");

        //@todo:make work if part_amt doesnt divide evenly.
        int part_amt = numEntries / numThreads;

        //Divide ROB up evenly
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = part_amt;
        }

    } else if (robPolicy == SMTQueuePolicy::Threshold) {
        DPRINTF(Fetch, "ROB sharing policy set to Threshold\n");

        int threshold =  params.smtROBThreshold;;

        //Divide up by threshold amount
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = threshold;
        }
    }

    for (ThreadID tid = numThreads; tid < MaxThreads; tid++) {
        maxEntries[tid] = 0;
    }

    resetState();
}

void
ROB::resetState()
{
    for (ThreadID tid = 0; tid  < MaxThreads; tid++) {
        threadEntries[tid] = 0;
        squashIt[tid] = instList[tid].end();
        squashedSeqNum[tid] = 0;
        doneSquashing[tid] = true;
    }
    numInstsInROB = 0;

    // Initialize the "universal" ROB head & tail point to invalid
    // pointers
    head = instList[0].end();
    tail = instList[0].end();
}

std::string
ROB::name() const
{
    return cpu->name() + ".rob";
}

void
ROB::setActiveThreads(std::list<ThreadID> *at_ptr)
{
    DPRINTF(ROB, "Setting active threads list pointer.\n");
    activeThreads = at_ptr;
}

void
ROB::drainSanityCheck() const
{
    for (ThreadID tid = 0; tid  < numThreads; tid++)
        assert(instList[tid].empty());
    assert(isEmpty());
}

void
ROB::takeOverFrom()
{
    resetState();
}

void
ROB::resetEntries()
{
    if (robPolicy != SMTQueuePolicy::Dynamic || numThreads > 1) {
        auto active_threads = activeThreads->size();

        for (ThreadID tid : *activeThreads) {
            if (robPolicy == SMTQueuePolicy::Partitioned) {
                maxEntries[tid] = numEntries / active_threads;
            } else if (robPolicy == SMTQueuePolicy::Threshold &&
                       active_threads == 1) {
                maxEntries[tid] = numEntries;
            }
        }
    }
}

int
ROB::entryAmount(ThreadID num_threads)
{
    if (robPolicy == SMTQueuePolicy::Partitioned) {
        return numEntries / num_threads;
    } else {
        return 0;
    }
}

int
ROB::countInsts()
{
    int total = 0;

    for (ThreadID tid = 0; tid < numThreads; tid++)
        total += countInsts(tid);

    return total;
}

size_t
ROB::countInsts(ThreadID tid)
{
    return instList[tid].size();
}

void
ROB::insertInst(const DynInstPtr &inst)
{
    assert(inst);

    stats.writes++;

    DPRINTF(ROB, "Adding inst PC %s to the ROB.\n", inst->pcState());

    assert(numInstsInROB != numEntries);

    ThreadID tid = inst->threadNumber;

    // [STT] Taint no longer needs a per-instruction producer pointer here
    // Rename already computed inst's yrot/addrYrot

    instList[tid].push_back(inst);

    //Set Up head iterator if this is the 1st instruction in the ROB
    if (numInstsInROB == 0) {
        head = instList[tid].begin();
        assert((*head) == inst);
    }

    //Must Decrement for iterator to actually be valid  since __.end()
    //actually points to 1 after the last inst
    tail = instList[tid].end();
    tail--;

    inst->setInROB();

    ++numInstsInROB;
    ++threadEntries[tid];

    assert((*tail) == inst);

    DPRINTF(ROB, "[tid:%i] Now has %d instructions.\n", tid,
            threadEntries[tid]);
}

void
ROB::retireHead(ThreadID tid)
{
    stats.writes++;

    assert(numInstsInROB > 0);

    // Get the head ROB instruction by copying it and remove it from the list
    InstIt head_it = instList[tid].begin();

    DynInstPtr head_inst = std::move(*head_it);
    instList[tid].erase(head_it);

    assert(head_inst->readyToCommit());

    DPRINTF(ROB, "[tid:%i] Retiring head instruction, "
            "instruction PC %s, [sn:%llu]\n", tid, head_inst->pcState(),
            head_inst->seqNum);

    --numInstsInROB;
    --threadEntries[tid];

    head_inst->clearInROB();
    head_inst->setCommitted();

    //Update "Global" Head of ROB
    updateHead();

    // @todo: A special case is needed if the instruction being
    // retired is the only instruction in the ROB; otherwise the tail
    // iterator will become invalidated.
    cpu->removeFrontInst(head_inst);
}

bool
ROB::isHeadReady(ThreadID tid)
{
    stats.reads++;
    if (threadEntries[tid] != 0) {
        return instList[tid].front()->readyToCommit();
    }

    return false;
}

bool
ROB::canCommit()
{
    //@todo: set ActiveThreads through ROB or CPU
    for (ThreadID tid : *activeThreads) {
        if (isHeadReady(tid)) {
            return true;
        }
    }

    return false;
}

unsigned
ROB::numFreeEntries()
{
    return numEntries - numInstsInROB;
}

unsigned
ROB::numFreeEntries(ThreadID tid)
{
    return maxEntries[tid] - threadEntries[tid];
}

void
ROB::doSquash(ThreadID tid)
{
    stats.writes++;
    DPRINTF(ROB, "[tid:%i] Squashing instructions until [sn:%llu].\n",
            tid, squashedSeqNum[tid]);

    assert(squashIt[tid] != instList[tid].end());

    if ((*squashIt[tid])->seqNum < squashedSeqNum[tid]) {
        DPRINTF(ROB, "[tid:%i] Done squashing instructions.\n",
                tid);

        squashIt[tid] = instList[tid].end();

        doneSquashing[tid] = true;
        return;
    }

    bool robTailUpdate = false;

    auto numInstsToSquash = squashWidth.has_value() ? squashWidth : numEntries;

    // If the CPU is exiting, squash all of the instructions
    // it is told to, even if that exceeds the squashWidth.
    // Set the number to the number of entries (the max).
    if (cpu->isThreadExiting(tid))
    {
        numInstsToSquash = numEntries;
    }

    for (int numSquashed = 0;
         numSquashed < numInstsToSquash &&
         squashIt[tid] != instList[tid].end() &&
         (*squashIt[tid])->seqNum > squashedSeqNum[tid];
         ++numSquashed)
    {
        DPRINTF(ROB, "[tid:%i] Squashing instruction PC %s, seq num %i.\n",
                (*squashIt[tid])->threadNumber,
                (*squashIt[tid])->pcState(),
                (*squashIt[tid])->seqNum);

        // Mark the instruction as squashed, and ready to commit so that
        // it can drain out of the pipeline.
        (*squashIt[tid])->setSquashed();

        // [STT] This instruction is being squashed right now (by some
        // *older* squash sweeping it away). Whatever deferred squash it
        // might have been carrying on itself (see IEW::executeInsts()/
        // Commit::commit() setting hasPendingSquash(true) on a still-
        // tainted branch/load) no longer matters -- the inst is being
        // wiped out anyway, so there is nothing left to "resolve" later.
        // Clear the flag so ROB::getResolvedPendingSquashInst() and
        // DynInst::readyToCommit() don't keep waiting on it.
        (*squashIt[tid])->hasPendingSquash(false);

        (*squashIt[tid])->setCanCommit();


        if (squashIt[tid] == instList[tid].begin()) {
            DPRINTF(ROB, "Reached head of instruction list while "
                    "squashing.\n");

            squashIt[tid] = instList[tid].end();

            doneSquashing[tid] = true;

            return;
        }

        InstIt tail_thread = instList[tid].end();
        tail_thread--;

        if ((*squashIt[tid]) == (*tail_thread))
            robTailUpdate = true;

        squashIt[tid]--;
    }


    // Check if ROB is done squashing.
    if ((*squashIt[tid])->seqNum <= squashedSeqNum[tid]) {
        DPRINTF(ROB, "[tid:%i] Done squashing instructions.\n",
                tid);

        squashIt[tid] = instList[tid].end();

        doneSquashing[tid] = true;
    }

    if (robTailUpdate) {
        updateTail();
    }
}


void
ROB::updateHead()
{
    InstSeqNum lowest_num = 0;
    bool first_valid = true;

    // @todo: set ActiveThreads through ROB or CPU
    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty())
            continue;

        if (first_valid) {
            head = instList[tid].begin();
            lowest_num = (*head)->seqNum;
            first_valid = false;
            continue;
        }

        InstIt head_thread = instList[tid].begin();

        DynInstPtr head_inst = (*head_thread);

        assert(head_inst);

        if (head_inst->seqNum < lowest_num) {
            head = head_thread;
            lowest_num = head_inst->seqNum;
        }
    }

    if (first_valid) {
        head = instList[0].end();
    }

}

void
ROB::updateTail()
{
    tail = instList[0].end();
    bool first_valid = true;

    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty()) {
            continue;
        }

        // If this is the first valid then assign w/out
        // comparison
        if (first_valid) {
            tail = instList[tid].end();
            tail--;
            first_valid = false;
            continue;
        }

        // Assign new tail if this thread's tail is younger
        // than our current "tail high"
        InstIt tail_thread = instList[tid].end();
        tail_thread--;

        if ((*tail_thread)->seqNum > (*tail)->seqNum) {
            tail = tail_thread;
        }
    }
}

// [STT] Called once per cycle (from Commit::markCompletedInsts()) to decide,
// for every in-flight instruction, whether it is still possible for some
// *older* instruction to cause it to be squashed. This is the "unsquashable"
// concept from the STT paper: a transmit instruction (e.g. a load) is only
// safe to treat as non-speculative -- i.e. safe to let its own seqNum clear
// the taint of anything whose yrot/addrYrot points at it (see
// getVisibilityPointSeqNum()) -- once nothing ahead of it in program order
// could still undo it.
void
ROB::updateVisibleState()
{
    // Threat models differ on *which* older events still threaten a
    // squash:
    //   - Spectre threat model (isFuturistic unset): only unresolved
    //     *branches* ahead of an instruction can still misspeculate and
    //     squash it, so we only need prevBrsResolved.
    //   - Futuristic threat model (isFuturistic set): the paper's more
    //     conservative model also worries about non-control squash sources
    //     (e.g. memory-order violations, faults), so it waits for *every*
    //     older instruction to complete, not just branches.
    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty()) {
            continue;
        }

        // These two booleans are folded forward across the loop: they
        // start true (nothing older has resolved yet, i.e. vacuously
        // true for the very first/oldest instruction) and latch false the
        // moment we hit an instruction that hasn't resolved -- from that
        // point on, every younger instruction is "not yet safe" too,
        // because it's still ordered behind an unresolved one.
        bool prevInstsComplete = true;
        bool prevBrsResolved = true;

        // [STT] tracks the youngest seqNum marked unsquashable so far
        InstSeqNum newVPThreshold = visibilityPointSeqNum[tid];

        // Walk the ROB oldest-to-youngest so the "all *previous*
        // instructions/branches resolved" folds correctly in program
        // order.
        for (auto instIt = instList[tid].begin();
             instIt != instList[tid].end(); ++instIt) {
            DynInstPtr inst = *instIt;

            // Once both trackers have gone false, every remaining
            // (younger) instruction in the ROB is also blocked by them,
            // so there's no need to keep scanning this thread this cycle.
            if (!prevInstsComplete && !prevBrsResolved) {
                break;
            }

            // These two flags are latched sticky per-instruction (see
            // DynInst::setPrevInstsCompleted/setPrevBrsResolved --
            // status.set(), never reset here) since an instruction, once
            // it has seen every older inst/branch resolve, stays true even
            // if a *younger* instruction later re-taints things.
            if (prevInstsComplete) {
                inst->setPrevInstsCompleted();
            }
            if (prevBrsResolved) {
                inst->setPrevBrsResolved();
            }

            // Update prev control insts state
            // If this instruction is itself a branch and hasn't resolved
            // yet (not ready to commit / has a fault / already squashed
            // counts as "not resolved" for this purpose), then everything
            // *younger* than it must wait on it -- flip the tracker off
            // for the rest of this scan.
            if (inst->isControl()) {
                if (!inst->readyToCommit() || inst->getFault() != NoFault ||
                    inst->isSquashed()) {
                    prevBrsResolved = false;
                }
            }

            // Update prev insts state. Some instructions directly set
            // prevInstsComplete = false when entering the ROB.
            // Certain instruction classes are inherently "not complete"
            // until they commit (they can trigger squashes/replays of
            // their own: non-speculative insts, store-conditionals, memory
            // barriers, strictly-ordered loads) -- so being merely
            // "issued"/"executed" isn't enough; force the Futuristic
            // tracker off for anything younger.
            if (inst->isNonSpeculative() || inst->isStoreConditional() ||
                inst->isFullMemBarrier() || inst->isReadBarrier() ||
                inst->isWriteBarrier() ||
                (inst->isLoad() && inst->strictlyOrdered())) {
                prevInstsComplete = false;
            }
            // Same idea as the branch check above, but for the
            // Futuristic (all-instructions) tracker: not ready to commit,
            // faulted, or squashed all mean this instruction hasn't
            // resolved, so it still threatens everything younger.
            if (!inst->readyToCommit() || inst->getFault() != NoFault ||
                inst->isSquashed()) {
                prevInstsComplete = false;
            }

            // [STT] an instruction is unsquashable once it can no longer be
            // squashed by an older, not-yet-resolved instruction/branch.
            if (cpu->protectionEnabled) {
                // Pick which resolved-ness this threat model cares about:
                // Futuristic waits on isPrevInstsCompleted(), Spectre only
                // waits on isPrevBrsResolved(). Whichever one applies, if
                // *this* instruction saw it hold at the point it was
                // visited above, it is now safe to call unsquashable.
                if ((cpu->isFuturistic && inst->isPrevInstsCompleted()) ||
                    (!cpu->isFuturistic && inst->isPrevBrsResolved())) {
                    inst->isUnsquashable(true);
                    newVPThreshold = std::max(newVPThreshold, inst->seqNum);
                } else {
                    // Still squashable by something older -- yrot/addrYrot
                    // comparisons against the visibility-point threshold
                    // will therefore not yet treat this seqNum as cleared,
                    // even if it's an access (load) instruction.
                    inst->isUnsquashable(false);
                }
            } else {
                // UnsafeBaseline: everything is immediately unsquashable
                // (no protection is applied, so there's no reason to ever
                // delay treating an instruction as resolved).
                inst->isUnsquashable(true);
                newVPThreshold = std::max(newVPThreshold, inst->seqNum);
            }
        }

        // [STT] Commit the advanced threshold. Monotonic by construction:
        // newVPThreshold started at the old value and only ever took max().
        visibilityPointSeqNum[tid] = newVPThreshold;
    }
}

// [STT] A branch mispredict or load-order violation on a still-tainted
// instruction was deferred (hasPendingSquash(true) set in
// IEW::executeInsts()/Commit::commit() instead of squashing immediately --
// see comments there) because acting on it right away would itself leak
// the tainted condition through the implicit/control channel. Every cycle,
// Commit::commit() calls this to check whether any such deferred squash
// has since become safe to act on.
DynInstPtr
ROB::getResolvedPendingSquashInst(ThreadID tid)
{
    // Scan oldest-to-youngest so that if more than one pending squash has
    // resolved, we return the oldest one first (squashing it will also
    // discard any younger ones, pending or not).
    for (auto &inst : instList[tid]) {
        // A pending squash is safe to act on once:
        //  - it's still marked pending (hasPendingSquash()) -- not already
        //    resolved/cleared elsewhere, and
        //  - it's no longer tainted (!isArgsTainted()) -- the condition
        //    that made deferring it necessary has cleared, so squashing on
        //    it now no longer correlates observably with the secret, and
        //  - it hasn't already been squashed by something else in the
        //    meantime (!isSquashed()) -- e.g. an older squash already
        //    swept it away (see ROB::doSquash() clearing hasPendingSquash
        //    in that case), in which case there's nothing left to act on.
        if (inst->hasPendingSquash() && !inst->isArgsTainted() &&
            !inst->isSquashed()) {
            return inst;
        }
    }
    // No pending squash has resolved yet this cycle.
    return nullptr;
}

// [STT] Debug dump
void
ROB::print_robs()
{
    for (ThreadID tid : *activeThreads) {
        printf("\nROB for thread %d\n", tid);
        for (int i = 0; i < 50; i++) {
            printf("-");
        }
        printf("\n");

        for (auto &inst : instList[tid]) {
            printf("ptr=%p, [sn:%lu], inst=%s ", inst.get(), inst->seqNum,
                   inst->staticInst->getName().c_str());
            for (int j = 0; j < inst->numDestRegs(); j++) {
                printf("%d(%s), ", inst->destRegIdx(j).index(),
                       inst->destRegIdx(j).className());
            }
            printf("| ");
            for (int j = 0; j < inst->numSrcRegs(); j++) {
                printf("%d(%s), ", inst->srcRegIdx(j).index(),
                       inst->srcRegIdx(j).className());
            }
            printf("| ");

            for (int j = 0; j < inst->numDestRegs(); j++) {
                printf("destPhys[%d] = %d(%d), ", j,
                       inst->renamedDestIdx(j)->index(),
                       inst->renamedDestIdx(j)->flatIndex());
            }
            for (int j = 0; j < inst->numSrcRegs(); j++) {
                printf("srcPhys[%d] = %d(%d), ", j,
                       inst->renamedSrcIdx(j)->index(),
                       inst->renamedSrcIdx(j)->flatIndex());
            }
            printf("fenceDelay=%d, ", inst->fenceDelay());
            printf("squash=%d, fault?=%d, ", inst->isSquashed(),
                   inst->getFault() != NoFault);
            printf("pendingSquash?=%d, ", inst->hasPendingSquash());
            printf("status=");
            if (inst->isCommitted()) {
                printf("Committed, ");
            } else if (inst->readyToCommit()) {
                if (inst->isExecuted()) {
                    printf("CanCommit(Exec), ");
                } else {
                    printf("CanCommit(NonExec), ");
                }
            } else if (inst->isExecuted()) {
                printf("Executed, ");
            } else if (inst->isIssued()) {
                printf("Issued, ");
            } else {
                printf("Not Issued, ");
            }
            printf("unsquashable=%d, yrot=%lu, addrYrot=%lu, "
                   "ArgsTainted=%d, AddrTainted=%d, ",
                   inst->isUnsquashable(), inst->getYRoT(),
                   inst->getAddrYRoT(), inst->isArgsTainted(),
                   inst->isAddrTainted());
            printf("PrevBrsResolved=%d, PrevInstsCompleted=%d, "
                   "visibilityPointSeqNum=%lu",
                   inst->isPrevBrsResolved(), inst->isPrevInstsCompleted(),
                   getVisibilityPointSeqNum(tid));
            printf("\n");
            for (int i = 0; i < 50; i++) {
                printf("-");
            }
            printf("\n");
        }
    }
}

void
ROB::squash(InstSeqNum squash_num, ThreadID tid)
{
    if (isEmpty(tid)) {
        DPRINTF(ROB, "Does not need to squash due to being empty "
                "[sn:%llu]\n",
                squash_num);

        return;
    }

    DPRINTF(ROB, "Starting to squash within the ROB.\n");

    robStatus[tid] = ROBSquashing;

    doneSquashing[tid] = false;

    squashedSeqNum[tid] = squash_num;

    if (!instList[tid].empty()) {
        InstIt tail_thread = instList[tid].end();
        tail_thread--;

        squashIt[tid] = tail_thread;

        doSquash(tid);
    }
}

const DynInstPtr&
ROB::readHeadInst(ThreadID tid)
{
    if (threadEntries[tid] != 0) {
        InstIt head_thread = instList[tid].begin();

        assert((*head_thread)->isInROB());

        return *head_thread;
    } else {
        return dummyInst;
    }
}

DynInstPtr
ROB::readTailInst(ThreadID tid)
{
    InstIt tail_thread = instList[tid].end();
    tail_thread--;

    return *tail_thread;
}

ROB::ROBStats::ROBStats(statistics::Group *parent)
  : statistics::Group(parent, "rob"),
    ADD_STAT(reads, statistics::units::Count::get(),
        "The number of ROB reads"),
    ADD_STAT(writes, statistics::units::Count::get(),
        "The number of ROB writes")
{
}

DynInstPtr
ROB::findInst(ThreadID tid, InstSeqNum squash_inst)
{
    for (InstIt it = instList[tid].begin(); it != instList[tid].end(); it++) {
        if ((*it)->seqNum == squash_inst) {
            return *it;
        }
    }
    return NULL;
}

} // namespace o3
} // namespace gem5
