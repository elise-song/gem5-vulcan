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

    // [STT] Taint has to flow from producer to consumer through registers,
    // so for every new instruction entering the ROB we record, per source
    // register, which still-in-flight instruction last wrote it. Later,
    // compute_taint() reads inst->getArgProducer(i)->isDestTainted() to see
    // if a source is tainted, instead of re-deriving the whole dependence
    // chain from scratch every cycle.
    //
    // Walk every instruction currently in the ROB, oldest to youngest
    // (this is the candidate list of "producers").
    for (auto &prevInst : instList[tid]) {
        // For each source register of the instruction being inserted...
        for (int i = 0; i < inst->numSrcRegs(); i++) {
            // Skip sources that aren't real registers since nothing can produce/taint these.
            if (inst->renamedSrcIdx(i)->is(InvalidRegClass)) {
                continue;
            }
            // compare against every destination register prevInst
            // writes (usually 0 or 1, but some insts have multiple dests).
            for (int j = 0; j < prevInst->numDestRegs(); j++) {
                // Same physical/renamed register number == prevInst is the
                // (renamed) producer of this source. Because we walk
                // oldest-to-youngest, if multiple in-flight instructions
                // wrote the same renamed register (shouldn't normally
                // happen post-rename, but the loop is written so the last
                // match wins) we end up with the most recent producer.
                if (inst->renamedSrcIdx(i) == prevInst->renamedDestIdx(j)) {
                    // Record it: inst->argProducers[i] = prevInst.
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

    // [STT] head_inst (the oldest instruction) is retiring/committing, so
    // it is architecturally visible now and can no longer carry taint --
    // any argProducer pointer that still points at it would be a dangling
    // reference once it's freed, and semantically wrong besides (a
    // committed instruction's value is no longer "in flight"). Clear every
    // pointer to it before it leaves.
    //
    // Scan every remaining (still in-flight, younger) instruction...
    for (auto &nextInst : instList[tid]) {
        // ...over each of its source registers...
        for (int i = 0; i < nextInst->numSrcRegs(); i++) {
            // ...and if that source's recorded producer is the inst we're
            // retiring, drop the link (getArgProducer will now return
            // nullptr for that source, which explicit_flow/address_flow
            // treat as "producer already committed, not tainted").
            if (nextInst->getArgProducer(i) == head_inst) {
                nextInst->clearArgProducer(i);
            }
        }
    }
    // Also clear head_inst's own argProducer entries (pointers to *its*
    // producers), since head_inst itself is about to be destructed/reused
    // and shouldn't hold references to other instructions past this point.
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
// safe to treat as non-speculative -- i.e. safe to let it become the root
// of taint in compute_taint() -- once nothing ahead of it in program order
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
                } else {
                    // Still squashable by something older -- compute_taint()
                    // will therefore not yet treat it as a root of taint
                    // even if it's an access (load) instruction.
                    inst->isUnsquashable(false);
                }
            } else {
                // UnsafeBaseline: everything is immediately unsquashable
                // (no protection is applied, so there's no reason to ever
                // delay treating an instruction as resolved).
                inst->isUnsquashable(true);
            }
        }
    }
}

// [STT] "Explicit flow" == ordinary data-flow taint: does this instruction
// read a value that came (directly, through registers) from a tainted
// producer? This is the classic taint-propagation rule: taint(dest) =
// OR over all sources of taint(source).
void
ROB::explicit_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    // Check every source register this instruction reads...
    for (int i = 0; i < inst->numSrcRegs(); i++) {
        // ...look up which in-flight instruction produced it (set in
        // ROB::insertInst(), cleared in ROB::retireHead()).
        DynInstPtr argProducer = inst->getArgProducer(i);
        // A source counts as tainted only if:
        //  (1) its producer is still in flight (argProducer non-null --
        //      a null producer means either no in-flight producer was
        //      found, i.e. the value came from an architectural register
        //      already committed, which is by definition untainted), and
        //  (2) that producer's destination is currently marked tainted, and
        //  (3) the producer hasn't committed yet -- once committed its
        //      value is architecturally visible/resolved and no longer a
        //      speculative secret in flight (mirrors the isCommitted()
        //      check retireHead() effectively already enforces by clearing
        //      argProducer links on retirement, but this is a second,
        //      cheap guard against stale reads within the same cycle).
        if (argProducer && argProducer->isDestTainted() &&
            !argProducer->isCommitted()) {
            // Found one tainted source -- that's enough to taint this
            // instruction's own result; no need to check the rest.
            inst->hasExplicitFlow(true);
            return;
        }
    }
    // None of the sources were tainted.
    inst->hasExplicitFlow(false);
}

// [STT] "Address flow" == is this memory instruction's *effective address*
// (as opposed to, for a store, the value being stored) computed from
// tainted data? This is what actually matters for the covert channel: a
// load/store whose address depends on a secret is what leaks the secret
// into the cache (which line gets touched), regardless of whether the
// loaded/stored *value* itself is tainted.
void
ROB::address_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    // Only loads/stores have an "address" to taint at all.
    if (!inst->isMemRef()) {
        inst->isAddrTainted(false);
        return;
    }

    // For a store, index 0 is the value to be stored; the remaining
    // sources feed the effective address. For a load, all sources feed
    // the effective address.
    // A store's source-register list puts the store *data* at index 0
    // and the address-computation operands after it (gem5 convention),
    // so skip index 0 for stores -- we only care about taint that reaches
    // the address, not the payload being written. A load has no separate
    // data source, so every source feeds the address.
    int startIdx = inst->isStore() ? 1 : 0;
    for (int i = startIdx; i < inst->numSrcRegs(); i++) {
        // Same producer lookup and taint check as explicit_flow() above,
        // but restricted to the address-computation sources only.
        DynInstPtr argProducer = inst->getArgProducer(i);
        if (argProducer && argProducer->isDestTainted() &&
            !argProducer->isCommitted()) {
            inst->isAddrTainted(true);
            return;
        }
    }
    inst->isAddrTainted(false);
}

// [STT] "Implicit flow" (control-flow / implicit channel): even if an
// instruction's own operands aren't tainted, if it is control-dependent on
// an *earlier branch whose condition was tainted*, its very presence in the
// pipeline (i.e. whether it executes at all, and when) leaks information
// about that branch's tainted condition. This models the classic "if
// (secret) { transmit() }" pattern where the leak is through *which* path
// is taken, not through any register value. Only tracked when
// cpu->impChannel (the --implicit_channel flag) is enabled -- it's strictly
// more conservative/expensive than the explicit/address channels alone.
void
ROB::implicit_flow(ThreadID tid, InstIt instIt)
{
    DynInstPtr inst = *instIt;
    if (cpu->impChannel) {
        // Scan every instruction *older* than this one in program order
        // (from the head of the ROB up to, but not including, instIt)...
        for (auto prevInstIt = instList[tid].begin(); prevInstIt != instIt;
             ++prevInstIt) {
            DynInstPtr prevInst = *prevInstIt;
            // ...looking for an older branch whose own condition was
            // itself explicitly tainted (hasExplicitFlow(), computed
            // earlier this same cycle by explicit_flow() -- compute_taint()
            // calls explicit_flow() before implicit_flow() for exactly
            // this reason). If found, this instruction is on a
            // taint-dependent control path.
            if (prevInst->isControl() && prevInst->hasExplicitFlow()) {
                inst->hasImplicitFlow(true);
                return;
            }
        }
    }
    // Either implicit-channel protection is off, or no tainted branch
    // was found ahead of this instruction.
    inst->hasImplicitFlow(false);
}

// [STT] The top-level taint recomputation, called once per cycle from
// Commit::markCompletedInsts() (only when cpu->STT is set). This is the
// entire STT taint-propagation algorithm for one cycle: for every in-flight
// instruction, in program order (oldest first, which matters because
// implicit_flow() and explicit_flow() both look at *older* instructions'
// already-updated-this-cycle taint state), recompute whether it's tainted
// from scratch. Taint is not sticky/incremental across cycles by design --
// as producers commit or squash, an instruction that was tainted last cycle
// may no longer be tainted this cycle, so it's cheaper and simpler to just
// redo the whole pass than to try to invalidate a stale taint graph.
void
ROB::compute_taint()
{
    assert(cpu->STT);

    for (ThreadID tid : *activeThreads) {
        if (instList[tid].empty()) {
            continue;
        }

        // Oldest-to-youngest order is required: address_flow/explicit_flow
        // for instIt only look at instIt's own argProducers (which are
        // always older), and implicit_flow for instIt scans everything
        // *before* instIt in this same list -- so by the time we reach a
        // given instruction, every instruction it might depend on has
        // already had its taint flags refreshed this cycle.
        for (auto instIt = instList[tid].begin();
             instIt != instList[tid].end(); ++instIt) {
            // Recompute the three taint "sources" for this instruction:
            // explicit (data through registers), implicit (control
            // dependence on a tainted branch), address (tainted effective
            // address for a load/store).
            explicit_flow(tid, instIt);
            implicit_flow(tid, instIt);
            address_flow(tid, instIt);

            DynInstPtr inst = *instIt;
            // An instruction's *result* (its args, from the perspective of
            // anything that consumes it later) is tainted if it has
            // explicit flow -- i.e. it computed its result from a tainted
            // operand. (Implicit/address taint affects whether *this*
            // instruction is dangerous to let execute/transmit, not
            // whether its own register result is poisoned -- see
            // LSQUnit::updateVisibleState(), which reads isArgsTainted()
            // directly to decide fenceDelay, and IEW::executeInsts(),
            // which checks isArgsTainted() on branches -- but taint isn't
            // propagated *from* isAddrTainted/hasImplicitFlow into
            // isDestTainted here.)
            inst->isArgsTainted(inst->hasExplicitFlow());
            // isDestTainted mirrors isArgsTainted by default: an
            // instruction whose args are tainted produces a tainted
            // destination register too (taint propagates through
            // computation).
            inst->isDestTainted(inst->isArgsTainted());
            // an access instruction (e.g. a load) is itself the root of
            // taint once it's no longer squashable
            // A load is where taint is first *introduced*, for as long as
            // the load is still speculative: while it's still possible for
            // some older, unresolved branch/instruction to squash this
            // load away (!isUnsquashable(), computed this same cycle by
            // updateVisibleState() right before compute_taint() runs), we
            // can't yet be sure the load is even on the correct path, so
            // its destination is conservatively forced tainted -- this is
            // what makes downstream transmitters wait on it. Once the load
            // becomes unsquashable (guaranteed to survive), this forced
            // taint is lifted; from then on isDestTainted only reflects
            // ordinary explicit-flow taint (e.g. if the load's own address
            // was itself computed from another still-tainted register).
            if (inst->isAccess() && !inst->isUnsquashable()) {
                inst->isDestTainted(true);
            }
        }
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
