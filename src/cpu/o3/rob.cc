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

#include <limits>
#include <list>
#include <set>

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

    // DOLMA: classify this instruction's own control-taint status before
    // it's visible to anything else. Two independent things happen here:
    //  (1) if any still-unresolved ControlInducer already precedes it in
    //      the ROB, this instruction is itself ControlRestricted;
    //  (2) if this instruction is itself a branch whose direction/target
    //      can't be decided purely from its PC-relative encoding (any
    //      conditional or indirect control instruction), it becomes a
    //      ControlInducer for everything younger than it.
    if (cpu->isDolma() && !inst->isSquashed()) {
        if (!cpu->isDolmaMemOnly()) {
            for (auto &prevInst : instList[tid]) {
                if (prevInst->isControlInducer()) {
                    inst->setControlRestricted();
                    break;
                }
            }
        }
        if (inst->isControl() &&
            !(inst->isDirectCtrl() && inst->isUncondCtrl())) {
            inst->setControlInducer();
        }
    }

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

    // DOLMA: a retiring instruction must leave the ROB clean of every DOLMA
    // predicate (clearInROB() asserts this). In conservative mode a
    // DataInducer is deliberately retained live until it actually commits
    // (see updateSafeStatus()'s ydis-retention rule), so it never gets
    // cleared by the per-cycle sweep -- clear it here instead, and re-sweep
    // since dependents may now be able to clear DataRestricted too. This is
    // also a defensive backstop for any other mode/edge case that reaches
    // retirement with an inducer flag still set.
    if (cpu->isDolma()) {
        if (head_inst->isDataInducer()) {
            head_inst->clearDataInducer();
            updateSafeStatus(tid);
        }
        if (head_inst->isControlInducer()) {
            head_inst->clearControlInducer();
        }
    }

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

// DOLMA: the once-per-cycle clearing sweep. Walks each thread's ROB
// oldest-to-youngest and recomputes which control/data inducers are still
// "live" (able to still restrict something), clearing ControlRestricted/
// DataRestricted once they're not. Called once per cycle from
// Commit::markCompletedInsts(), after execute.
void
ROB::updateSafeStatus(ThreadID tid)
{
    if (!cpu->isDolma() || instList[tid].empty()) {
        return;
    }

    bool foundUnresolvedBranch = false;
    bool foundUnresolvedStore = false;
    std::set<InstSeqNum> liveYdis;
    InstSeqNum oldestViolator = std::numeric_limits<InstSeqNum>::max();

    for (auto &inst : instList[tid]) {
        if (inst->isSquashed()) {
            continue;
        }

        if (inst->isDolmaRestricted()) {
            if (inst->isControlRestricted() && !foundUnresolvedBranch) {
                inst->clearControlRestricted();
            }
            if (inst->isDataRestricted() &&
                liveYdis.find(inst->ydi) == liveYdis.end()) {
                inst->clearDataRestricted();
            }

            if (!inst->isDolmaRestricted()) {
                // Both cleared this cycle: this instruction is now fully
                // safe.
                if (inst->isDolmaStalled()) {
                    // Delay-on-miss resolved: it can be retried.
                    inst->clearDolmaStalled();
                } else if (inst->isStore() && !cpu->isSTT()) {
                    DynInstPtr violator = inst->getViolator();
                    if (violator && !violator->isSquashed()) {
                        violator->setPendingMemOrder(inst);
                    }
                }
            }
        }

        if (inst->isDataInducer()) {
            // Retention rules: an inducer stays "live" (and keeps
            // restricting its dependents) under any of these conditions;
            // otherwise it's no longer a threat and is cleared outright.
            if (cpu->isDolmaConservative() || inst->isPendingMemOrder()) {
                liveYdis.insert(inst->seqNum);
            } else if (cpu->isDolmaMemOnly() && foundUnresolvedBranch) {
                liveYdis.insert(inst->seqNum);
            } else if (!cpu->isSTT() && (foundUnresolvedStore ||
                                         inst->seqNum >= oldestViolator)) {
                liveYdis.insert(inst->seqNum);
            } else {
                inst->clearDataInducer();
            }
        }
        if (inst->isControlInducer()) {
            foundUnresolvedBranch = true;
        }

        if (inst->isStore() && !cpu->isSTT() && !cpu->isDolmaConservative()) {
            if (!inst->effAddrValid()) {
                foundUnresolvedStore = true;
            } else if (inst->violatorSeqNum &&
                       inst->violatorSeqNum < oldestViolator) {
                DynInstPtr violator = findInst(tid, inst->violatorSeqNum);
                if (violator && !violator->isSquashed()) {
                    oldestViolator = inst->violatorSeqNum;
                } else {
                    inst->violatorSeqNum = 0;
                }
            }
        }
    }
}

// DOLMA: scan for the oldest instruction whose redirect (branch mispredict
// or memory-order violation) was deferred while it was restricted, and
// which has since become safe to act on.
DynInstPtr
ROB::getResolvedRedirect(ThreadID tid)
{
    if (instList[tid].empty()) {
        return nullptr;
    }
    for (auto &inst : instList[tid]) {
        if (inst->isDolmaRestricted()) {
            continue;
        }
        if (inst->isPendingBranch()) {
            return inst;
        }
        if (inst->isPendingMemOrder() &&
            (inst->getColliderPC() || cpu->isSTT())) {
            return inst;
        }
    }
    return nullptr;
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
