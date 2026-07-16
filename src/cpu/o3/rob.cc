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

    // [STT] link inst's argProducers to whichever in-flight instructions
    // (still in the ROB) most recently produced each of its source regs.
    for (auto &prevInst : instList[tid]) {
        for (int i = 0; i < inst->numSrcRegs(); i++) {
            if (inst->renamedSrcIdx(i)->is(InvalidRegClass)) {
                continue;
            }
            for (int j = 0; j < prevInst->numDestRegs(); j++) {
                if (inst->renamedSrcIdx(i) == prevInst->renamedDestIdx(j)) {
                    inst->setArgProducer(i, prevInst);
                }
            }
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

    head_inst->clearInROB();
    head_inst->setCommitted();

    // [STT] clear any argProducer links pointing at the retiring inst
    for (auto &nextInst : instList[tid]) {
        for (int i = 0; i < nextInst->numSrcRegs(); i++) {
            if (nextInst->getArgProducer(i) == head_inst) {
                nextInst->clearArgProducer(i);
            }
        }
    }
    for (int i = 0; i < head_inst->numSrcRegs(); i++) {
        head_inst->clearArgProducer(i);
    }

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

        // [STT] the squash itself resolves any pending squash on this inst
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

// [STT]
void
ROB::updateVisibleState()
{
    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty()) {
            continue;
        }

        bool prevInstsComplete = true;
        bool prevBrsResolved = true;

        for (auto instIt = instList[tid].begin();
             instIt != instList[tid].end(); ++instIt) {
            DynInstPtr inst = *instIt;

            if (!prevInstsComplete && !prevBrsResolved) {
                break;
            }

            if (prevInstsComplete) {
                inst->setPrevInstsCompleted();
            }
            if (prevBrsResolved) {
                inst->setPrevBrsResolved();
            }

            // Update prev control insts state
            if (inst->isControl()) {
                if (!inst->readyToCommit() || inst->getFault() != NoFault ||
                    inst->isSquashed()) {
                    prevBrsResolved = false;
                }
            }

            // Update prev insts state. Some instructions directly set
            // prevInstsComplete = false when entering the ROB.
            if (inst->isNonSpeculative() || inst->isStoreConditional() ||
                inst->isFullMemBarrier() || inst->isReadBarrier() ||
                inst->isWriteBarrier() ||
                (inst->isLoad() && inst->strictlyOrdered())) {
                prevInstsComplete = false;
            }
            if (!inst->readyToCommit() || inst->getFault() != NoFault ||
                inst->isSquashed()) {
                prevInstsComplete = false;
            }

            // [STT] an instruction is unsquashable once it can no longer be
            // squashed by an older, not-yet-resolved instruction/branch.
            if (cpu->protectionEnabled) {
                if ((cpu->isFuturistic && inst->isPrevInstsCompleted()) ||
                    (!cpu->isFuturistic && inst->isPrevBrsResolved())) {
                    inst->isUnsquashable(true);
                } else {
                    inst->isUnsquashable(false);
                }
            } else {
                // UnsafeBaseline: everything is immediately unsquashable
                inst->isUnsquashable(true);
            }
        }
    }
}

// [STT]
void
ROB::explicit_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    for (int i = 0; i < inst->numSrcRegs(); i++) {
        DynInstPtr argProducer = inst->getArgProducer(i);
        if (argProducer && argProducer->isDestTainted() &&
            !argProducer->isCommitted()) {
            inst->hasExplicitFlow(true);
            return;
        }
    }
    inst->hasExplicitFlow(false);
}

// [STT]
void
ROB::address_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    if (!inst->isMemRef()) {
        inst->isAddrTainted(false);
        return;
    }

    // For a store, index 0 is the value to be stored; the remaining
    // sources feed the effective address. For a load, all sources feed
    // the effective address.
    int startIdx = inst->isStore() ? 1 : 0;
    for (int i = startIdx; i < inst->numSrcRegs(); i++) {
        DynInstPtr argProducer = inst->getArgProducer(i);
        if (argProducer && argProducer->isDestTainted() &&
            !argProducer->isCommitted()) {
            inst->isAddrTainted(true);
            return;
        }
    }
    inst->isAddrTainted(false);
}

// [STT]
void
ROB::implicit_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    if (cpu->impChannel) {
        for (auto prevInstIt = instList[tid].begin(); prevInstIt != instIt;
             ++prevInstIt) {
            DynInstPtr prevInst = *prevInstIt;
            if (prevInst->isControl() && prevInst->hasExplicitFlow()) {
                inst->hasImplicitFlow(true);
                return;
            }
        }
    }
    inst->hasImplicitFlow(false);
}

// [STT]
void
ROB::compute_taint()
{
    assert(cpu->STT);

    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty()) {
            continue;
        }

        for (auto instIt = instList[tid].begin();
             instIt != instList[tid].end(); ++instIt) {
            explicit_flow(tid, instIt);
            implicit_flow(tid, instIt);
            address_flow(tid, instIt);

            DynInstPtr inst = *instIt;
            inst->isArgsTainted(inst->hasExplicitFlow());
            inst->isDestTainted(inst->isArgsTainted());
            // an access instruction (e.g. a load) is itself the root of
            // taint once it's no longer squashable
            if (inst->isAccess() && !inst->isUnsquashable()) {
                inst->isDestTainted(true);
            }
        }
    }
}

// [STT]
DynInstPtr
ROB::getResolvedPendingSquashInst(ThreadID tid)
{
    for (auto &inst : instList[tid]) {
        if (inst->hasPendingSquash() && !inst->isArgsTainted() &&
            !inst->isSquashed()) {
            return inst;
        }
    }
    return nullptr;
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
