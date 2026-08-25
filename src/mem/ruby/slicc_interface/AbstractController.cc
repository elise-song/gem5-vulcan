/*
 * Copyright (c) 2017 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 2011-2014 Mark D. Hill and David A. Wood
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

#include "mem/ruby/slicc_interface/AbstractController.hh"

#include "debug/RubyQueue.hh"
#include "debug/MemSpecBuffer.hh"
#include "mem/protocol/MemoryMsg.hh"
#include "mem/ruby/network/Network.hh"
#include "mem/ruby/system/GPUCoalescer.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "mem/ruby/system/Sequencer.hh"
#include "sim/system.hh"

AbstractController::AbstractController(const Params *p)
    : MemObject(p), Consumer(this), m_version(p->version),
      m_clusterID(p->cluster_id),
      m_masterId(p->system->getMasterId(name())), m_is_blocking(false),
      m_number_of_TBEs(p->number_of_TBEs),
      m_transitions_per_cycle(p->transitions_per_cycle),
      m_buffer_size(p->buffer_size), m_recycle_latency(p->recycle_latency),
      memoryPort(csprintf("%s.memory", name()), this, ""),
      addrRanges(p->addr_ranges.begin(), p->addr_ranges.end())
{
    if (m_version == 0) {
        // Combine the statistics from all controllers
        // of this particular type.
        Stats::registerDumpCallback(new StatsCallback(this));
    }

    // [InvisiSpec] m_specBuf entries start invalid; without this, the
    // struct's POD fields (address, epoch) are left with indeterminate
    // values by default-initialization, which could spuriously "hit"
    // against a real request before any core has filled the entry.
    for (int c = 0; c < 8; ++c) {
        for (int i = 0; i < 8194; ++i) {
            m_specBuf[c][i].address = 0;
            m_specBuf[c][i].epoch = 0;
            m_specBuf[c][i].valid = false;
        }
    }
}

void
AbstractController::init()
{
    params()->ruby_system->registerAbstractController(this);
    m_delayHistogram.init(10);
    uint32_t size = Network::getNumberOfVirtualNetworks();
    for (uint32_t i = 0; i < size; i++) {
        m_delayVCHistogram.push_back(new Stats::Histogram());
        m_delayVCHistogram[i]->init(10);
    }
}

void
AbstractController::resetStats()
{
    m_delayHistogram.reset();
    uint32_t size = Network::getNumberOfVirtualNetworks();
    for (uint32_t i = 0; i < size; i++) {
        m_delayVCHistogram[i]->reset();
    }
}

void
AbstractController::regStats()
{
    MemObject::regStats();

    m_fully_busy_cycles
        .name(name() + ".fully_busy_cycles")
        .desc("cycles for which number of transistions == max transitions")
        .flags(Stats::nozero);
    m_expose_hits
        .name(name() + ".expose_hits")
        .desc("number of expose hits at LLC spec buffer")
        .flags(Stats::nozero);
    m_expose_misses
        .name(name() + ".expose_misses")
        .desc("number of expose misses at LLC spec buffer")
        .flags(Stats::nozero);
}

void
AbstractController::profileMsgDelay(uint32_t virtualNetwork, Cycles delay)
{
    assert(virtualNetwork < m_delayVCHistogram.size());
    m_delayHistogram.sample(delay);
    m_delayVCHistogram[virtualNetwork]->sample(delay);
}

void
AbstractController::stallBuffer(MessageBuffer* buf, Addr addr)
{
    if (m_waiting_buffers.count(addr) == 0) {
        MsgVecType* msgVec = new MsgVecType;
        msgVec->resize(m_in_ports, NULL);
        m_waiting_buffers[addr] = msgVec;
    }
    DPRINTF(RubyQueue, "stalling %s port %d addr %#x\n", buf, m_cur_in_port,
            addr);
    assert(m_in_ports > m_cur_in_port);
    (*(m_waiting_buffers[addr]))[m_cur_in_port] = buf;
}

void
AbstractController::wakeUpBuffers(Addr addr)
{
    if (m_waiting_buffers.count(addr) > 0) {
        //
        // Wake up all possible lower rank (i.e. lower priority) buffers that could
        // be waiting on this message.
        //
        for (int in_port_rank = m_cur_in_port - 1;
             in_port_rank >= 0;
             in_port_rank--) {
            if ((*(m_waiting_buffers[addr]))[in_port_rank] != NULL) {
                (*(m_waiting_buffers[addr]))[in_port_rank]->
                    reanalyzeMessages(addr, clockEdge());
            }
        }
        delete m_waiting_buffers[addr];
        m_waiting_buffers.erase(addr);
    }
}

void
AbstractController::wakeUpAllBuffers(Addr addr)
{
    if (m_waiting_buffers.count(addr) > 0) {
        //
        // Wake up all possible lower rank (i.e. lower priority) buffers that could
        // be waiting on this message.
        //
        for (int in_port_rank = m_in_ports - 1;
             in_port_rank >= 0;
             in_port_rank--) {
            if ((*(m_waiting_buffers[addr]))[in_port_rank] != NULL) {
                (*(m_waiting_buffers[addr]))[in_port_rank]->
                    reanalyzeMessages(addr, clockEdge());
            }
        }
        delete m_waiting_buffers[addr];
        m_waiting_buffers.erase(addr);
    }
}

void
AbstractController::wakeUpAllBuffers()
{
    //
    // Wake up all possible buffers that could be waiting on any message.
    //

    std::vector<MsgVecType*> wokeUpMsgVecs;
    MsgBufType wokeUpMsgBufs;

    if (m_waiting_buffers.size() > 0) {
        for (WaitingBufType::iterator buf_iter = m_waiting_buffers.begin();
             buf_iter != m_waiting_buffers.end();
             ++buf_iter) {
             for (MsgVecType::iterator vec_iter = buf_iter->second->begin();
                  vec_iter != buf_iter->second->end();
                  ++vec_iter) {
                  //
                  // Make sure the MessageBuffer has not already be reanalyzed
                  //
                  if (*vec_iter != NULL &&
                      (wokeUpMsgBufs.count(*vec_iter) == 0)) {
                      (*vec_iter)->reanalyzeAllMessages(clockEdge());
                      wokeUpMsgBufs.insert(*vec_iter);
                  }
             }
             wokeUpMsgVecs.push_back(buf_iter->second);
        }

        for (std::vector<MsgVecType*>::iterator wb_iter = wokeUpMsgVecs.begin();
             wb_iter != wokeUpMsgVecs.end();
             ++wb_iter) {
             delete (*wb_iter);
        }

        m_waiting_buffers.clear();
    }
}

void
AbstractController::blockOnQueue(Addr addr, MessageBuffer* port)
{
    m_is_blocking = true;
    m_block_map[addr] = port;
}

bool
AbstractController::isBlocked(Addr addr) const
{
    return m_is_blocking && (m_block_map.find(addr) != m_block_map.end());
}

void
AbstractController::unblock(Addr addr)
{
    m_block_map.erase(addr);
    if (m_block_map.size() == 0) {
       m_is_blocking = false;
    }
}

bool
AbstractController::isBlocked(Addr addr)
{
    return (m_block_map.count(addr) > 0);
}

BaseMasterPort &
AbstractController::getMasterPort(const std::string &if_name,
                                  PortID idx)
{
    return memoryPort;
}

void
AbstractController::queueMemoryRead(const MachineID &id, Addr addr,
                                    Cycles latency, MachineID origin, int idx, int type,
                                    int epoch)
{
    int coreId = origin.num;
    int sbeId = idx;
    // type 0: non-spec 1: spec 2: expose
    // DPRINTFR(MemSpecBuffer, "%10s MemRead (core=%d, type=%d, idx=%d, addr=%#x)\n", curTick(), coreId, type, sbeId, printAddress(addr));
    // if idx == -1, it is a write request which cannot be spec or expose.
    assert(!(type != 0 && sbeId == -1));
    // [InvisiSpec] m_specBuf is a fixed-size array indexed directly by
    // coreId/sbeId. Out-of-range indices are latent memory corruption in
    // opt/fast builds, where assert() compiles away under NDEBUG -- use
    // panic_if (never stripped) so a config with more cores or a larger
    // load queue than m_specBuf supports fails loudly instead of silently
    // corrupting memory.
    panic_if(sbeId < -1 || sbeId > 8193,
             "InvisiSpec LLC-SB index %d out of range for m_specBuf "
             "(configured load queue is larger than the LLC-SB supports)",
             sbeId);
    panic_if(coreId < 0 || coreId >= 8,
             "InvisiSpec LLC-SB core id %d out of range for m_specBuf "
             "(configured core count exceeds the LLC-SB's per-core arrays)",
             coreId);
    assert(type >=0 && type <= 2);
    if (type == 0) {
        // [InvisiSpec] A safe (non-speculative) access misses in the LLC:
        // per Section VI-C, purge this line from every core's LLC-SB so a
        // later Spec-GetS/Expose/Validate cannot read data made stale by
        // this access.
        for (int c = 0; c < 8; ++c) {
            for (int i = 0; i < 8194; ++i) {
                if (m_specBuf[c][i].valid &&
                        m_specBuf[c][i].address == addr) {
                    DPRINTFR(MemSpecBuffer, "%10s Cleared by Read "
                             "(core=%d, type=%d, idx=%d, addr=%#x)\n",
                             curTick(), c, type, i, printAddress(addr));
                    m_specBuf[c][i].address = 0;
                    m_specBuf[c][i].data.clear();
                    m_specBuf[c][i].valid = false;
                }
            }
        }
    } else if (type == 1) {
        // [InvisiSpec] A USL's initial Spec-GetS request always bypasses
        // the LLC-SB and goes to memory (Section VI-C: "InvisiSpec forbids
        // a USL from obtaining data from the LLC-SB"); the epoch-staleness
        // check happens when its response arrives, in recvTimingResp().

    } else if (type == 2) {
        // [InvisiSpec] A Validate/Expose request checks its indexed
        // LLC-SB entry; per Section VI-C, it counts as a hit only if both
        // the address *and* the Epoch ID match the entry that is there,
        // so a stale entry left behind by an old (squashed) epoch cannot
        // be mistaken for this request's own data.
        SBE &sbe = m_specBuf[coreId][sbeId];
        if (sbe.valid && sbe.address == addr && sbe.epoch == epoch) {
            DPRINTFR(MemSpecBuffer, "%10s Expose Hit (core=%d, type=%d, idx=%d, addr=%#x)\n", curTick(), coreId, type, sbeId, printAddress(addr));
            ++m_expose_hits;
            assert(getMemoryQueue());
            std::shared_ptr<MemoryMsg> msg = std::make_shared<MemoryMsg>(clockEdge());
            (*msg).m_addr = addr;
            (*msg).m_Sender = m_machineID;
            (*msg).m_OriginalRequestorMachId = id;
            (*msg).m_Type = MemoryRequestType_MEMORY_READ;
            (*msg).m_MessageSize = MessageSizeType_Response_Data;
            (*msg).m_DataBlk = sbe.data;
            getMemoryQueue()->enqueue(msg, clockEdge(), cyclesToTicks(Cycles(1)));
            for (int c = 0; c < 8; ++c) {
                for (int i = 0; i < 8194; ++i) {
                    if (m_specBuf[c][i].valid &&
                            m_specBuf[c][i].address == addr) {
                        DPRINTFR(MemSpecBuffer, "%10s Cleared by Expose Hit "
                                 "(core=%d, type=%d, idx=%d, addr=%#x)\n",
                                 curTick(), c, type, i, printAddress(addr));
                        m_specBuf[c][i].address = 0;
                        m_specBuf[c][i].data.clear();
                        m_specBuf[c][i].valid = false;
                    }
                }
            }
            return;
        } else {
            DPRINTFR(MemSpecBuffer, "%10s Expose Miss (core=%d, type=%d, idx=%d, addr=%#x)\n", curTick(), coreId, type, sbeId, printAddress(addr));
            ++m_expose_misses;
            for (int c = 0; c < 8; ++c) {
                for (int i = 0; i < 8194; ++i) {
                    if (m_specBuf[c][i].valid &&
                            m_specBuf[c][i].address == addr) {
                        DPRINTFR(MemSpecBuffer, "%10s Cleared by Expose Miss "
                                 "(core=%d, type=%d, idx=%d, addr=%#x)\n",
                                 curTick(), c, type, i, printAddress(addr));
                        m_specBuf[c][i].address = 0;
                        m_specBuf[c][i].data.clear();
                        m_specBuf[c][i].valid = false;
                    }
                }
            }
        }
    }

    RequestPtr req = new Request(addr, RubySystem::getBlockSizeBytes(), 0,
                                 m_masterId);

    PacketPtr pkt = Packet::createRead(req);
    uint8_t *newData = new uint8_t[RubySystem::getBlockSizeBytes()];
    pkt->dataDynamic(newData);

    SenderState *s = new SenderState(id);
    s->type = type;
    s->coreId = coreId;
    s->sbeId = sbeId;
    s->epoch = epoch;
    pkt->pushSenderState(s);

    // Use functional rather than timing accesses during warmup
    if (RubySystem::getWarmupEnabled()) {
        memoryPort.sendFunctional(pkt);
        recvTimingResp(pkt);
        return;
    }

    memoryPort.schedTimingReq(pkt, clockEdge(latency));
}

void
AbstractController::queueMemoryWrite(const MachineID &id, Addr addr,
                                     Cycles latency, const DataBlock &block)
{
    RequestPtr req = new Request(addr, RubySystem::getBlockSizeBytes(), 0,
                                 m_masterId);

    PacketPtr pkt = Packet::createWrite(req);
    uint8_t *newData = new uint8_t[RubySystem::getBlockSizeBytes()];
    pkt->dataDynamic(newData);
    memcpy(newData, block.getData(0, RubySystem::getBlockSizeBytes()),
           RubySystem::getBlockSizeBytes());

    SenderState *s = new SenderState(id);
    pkt->pushSenderState(s);

    // Use functional rather than timing accesses during warmup
    if (RubySystem::getWarmupEnabled()) {
        memoryPort.sendFunctional(pkt);
        recvTimingResp(pkt);
        return;
    }

    // Create a block and copy data from the block.
    memoryPort.schedTimingReq(pkt, clockEdge(latency));
}

void
AbstractController::queueMemoryWritePartial(const MachineID &id, Addr addr,
                                            Cycles latency,
                                            const DataBlock &block, int size)
{
    RequestPtr req = new Request(addr, size, 0, m_masterId);

    PacketPtr pkt = Packet::createWrite(req);
    uint8_t *newData = new uint8_t[size];
    pkt->dataDynamic(newData);
    memcpy(newData, block.getData(getOffset(addr), size), size);

    SenderState *s = new SenderState(id);
    pkt->pushSenderState(s);

    // Create a block and copy data from the block.
    memoryPort.schedTimingReq(pkt, clockEdge(latency));
}

void
AbstractController::functionalMemoryRead(PacketPtr pkt)
{
    memoryPort.sendFunctional(pkt);
}

int
AbstractController::functionalMemoryWrite(PacketPtr pkt)
{
    int num_functional_writes = 0;

    // Check the buffer from the controller to the memory.
    if (memoryPort.checkFunctional(pkt)) {
        num_functional_writes++;
    }

    // Update memory itself.
    memoryPort.sendFunctional(pkt);
    return num_functional_writes + 1;
}

void
AbstractController::recvTimingResp(PacketPtr pkt)
{
    assert(getMemoryQueue());
    assert(pkt->isResponse());

    std::shared_ptr<MemoryMsg> msg = std::make_shared<MemoryMsg>(clockEdge());
    (*msg).m_addr = pkt->getAddr();
    (*msg).m_Sender = m_machineID;

    SenderState *s = dynamic_cast<SenderState *>(pkt->senderState);
    (*msg).m_OriginalRequestorMachId = s->id;
    int type = s->type;
    int coreId = s->coreId;
    int sbeId = s->sbeId;
    int epoch = s->epoch;
    delete s;

    if (pkt->isRead()) {
        (*msg).m_Type = MemoryRequestType_MEMORY_READ;
        (*msg).m_MessageSize = MessageSizeType_Response_Data;

        // Copy data from the packet
        (*msg).m_DataBlk.setData(pkt->getPtr<uint8_t>(), 0,
                                 RubySystem::getBlockSizeBytes());
        if (type == 1) {
            // [InvisiSpec] Section VI-C: before saving into the indexed
            // LLC-SB entry, check whether a fresher (higher-epoch) fill
            // is already sitting there. This can happen if this core
            // squashed and reissued a Spec-GetS for the same LQ slot
            // while the old request was still in flight to memory, and
            // the two responses arrive out of order -- the stale,
            // lower-epoch response must be dropped rather than clobber
            // the newer entry.
            SBE &sbe = m_specBuf[coreId][sbeId];
            if (sbe.valid && sbe.epoch > epoch) {
                DPRINTFR(MemSpecBuffer, "%10s Stale ReadSpec dropped (core=%d, type=%d, idx=%d, addr=%#x, reqEpoch=%d, entryEpoch=%d)\n", curTick(), coreId, type, sbeId, printAddress(pkt->getAddr()), epoch, sbe.epoch);
            } else {
                DPRINTFR(MemSpecBuffer, "%10s Updated by ReadSpec (core=%d, type=%d, idx=%d, addr=%#x)\n", curTick(), coreId, type, sbeId, printAddress(pkt->getAddr()));
                sbe.address = pkt->getAddr();
                sbe.data.setData(pkt->getPtr<uint8_t>(), 0,
                                 RubySystem::getBlockSizeBytes());
                sbe.epoch = epoch;
                sbe.valid = true;
            }
        }
    } else if (pkt->isWrite()) {
        (*msg).m_Type = MemoryRequestType_MEMORY_WB;
        (*msg).m_MessageSize = MessageSizeType_Writeback_Control;
    } else {
        panic("Incorrect packet type received from memory controller!");
    }

    getMemoryQueue()->enqueue(msg, clockEdge(), cyclesToTicks(Cycles(1)));
    delete pkt->req;
    delete pkt;
}

Tick
AbstractController::recvAtomic(PacketPtr pkt)
{
   return ticksToCycles(memoryPort.sendAtomic(pkt));
}

MachineID
AbstractController::mapAddressToMachine(Addr addr, MachineType mtype) const
{
    NodeID node = m_net_ptr->addressToNodeID(addr, mtype);
    MachineID mach = {mtype, node};
    return mach;
}

bool
AbstractController::MemoryPort::recvTimingResp(PacketPtr pkt)
{
    controller->recvTimingResp(pkt);
    return true;
}

AbstractController::MemoryPort::MemoryPort(const std::string &_name,
                                           AbstractController *_controller,
                                           const std::string &_label)
    : QueuedMasterPort(_name, _controller, reqQueue, snoopRespQueue),
      reqQueue(*_controller, *this, _label),
      snoopRespQueue(*_controller, *this, _label),
      controller(_controller)
{
}
