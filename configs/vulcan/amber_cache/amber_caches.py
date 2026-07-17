# Copyright (c) 2017 Jason Power
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""This file creates a three-level (L1/L2/L3) set of Ruby caches, the Ruby
network, and a simple point-to-point topology, using gem5's built-in
MESI_Three_Level protocol.

Derived from Part 3 in the Learning gem5 book:
http://gem5.org/documentation/learning_gem5/part3/MSIintro

Protocol machine-type naming vs. the cache levels used here:
    MachineType L0Cache -> L1Cache (private, split I/D, per-core)
    MachineType L1Cache -> L2Cache (private, unified, per-core)
    MachineType L2Cache -> L3Cache (shared, unified, single bank)
    MachineType Directory -> DirController
"""

import math

from m5.defines import buildEnv
from m5.objects import *
from m5.util import (
    fatal,
    panic,
)


def get_block_size_bits(system):
    bits = int(math.log(system.cache_line_size, 2))
    if 2**bits != system.cache_line_size.value:
        panic("Cache line size not a power of 2!")
    return bits


class MyCacheSystem(RubySystem):
    def __init__(self):
        if not buildEnv.get("RUBY_PROTOCOL_MESI_Three_Level", False):
            fatal("This system assumes the MESI_Three_Level protocol!")

        super().__init__()

    def setup(self, system, cpus, mem_ctrls):
        """Set up the Ruby cache subsystem. Note: This can't be done in the
        constructor because many of these items require a pointer to the
        ruby system (self). This causes infinite recursion in initialize()
        if we do this in the __init__.
        """
        # Ruby's global network.
        self.network = MyNetwork(self)

        # MESI_Three_Level uses 3 virtual networks, same as the request/
        # response/forward split used by the base MSI protocol.
        self.number_of_virtual_networks = 3
        self.network.number_of_virtual_networks = 3

        # One private L1 (split I/D) and one private L2 controller per CPU,
        # a single shared L3 bank, and a single directory controller.
        l1_controllers = [L1Cache(system, self, cpu) for cpu in cpus]
        l2_controllers = [L2Cache(system, self, cpu) for cpu in cpus]
        l3_controllers = [L3Cache(system, self)]
        dir_controller = DirController(self, system.mem_ranges, mem_ctrls)

        # There is a single global list of all of the controllers to make it
        # easier to connect everything to the global network. This can be
        # customized depending on the topology/network requirements.
        self.controllers = (
            l1_controllers + l2_controllers + l3_controllers + [dir_controller]
        )

        # Create one sequencer per CPU and attach it to that CPU's L1
        # controller. In many systems this is more complicated since you
        # have to create sequencers for DMA controllers and other
        # controllers, too.
        self.sequencers = [
            RubySequencer(
                version=i,
                dcache=l1_controllers[i].Dcache,
                clk_domain=l1_controllers[i].clk_domain,
                ruby_system=self,
            )
            for i in range(len(cpus))
        ]

        for i, c in enumerate(l1_controllers):
            c.sequencer = self.sequencers[i]

        self.num_of_sequencers = len(self.sequencers)

        # Connect each CPU's private L1 to its private L2.
        for l1, l2 in zip(l1_controllers, l2_controllers):
            l1.connectToL2(l2)

        # Connect each private L2 to the (single, shared) L3 via the network.
        for l2 in l2_controllers:
            l2.connectToNetwork(self.network)

        # Connect the shared L3 and the directory to the network.
        for l3 in l3_controllers:
            l3.connectToNetwork(self.network)
        dir_controller.connectToNetwork(self.network)

        # Create the network and connect the controllers.
        # NOTE: This is quite different if using Garnet!
        self.network.connectControllers(self.controllers)
        self.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.sys_port_proxy = RubyPortProxy(ruby_system=self)
        system.system_port = self.sys_port_proxy.in_ports

        # Connect the cpu's cache, interrupt, and TLB ports to Ruby
        for i, cpu in enumerate(cpus):
            self.sequencers[i].connectCpuPorts(cpu)


class L1Cache(MESI_Three_Level_L0Cache_Controller):
    """Private, per-core, split instruction/data L1 cache."""

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, system, ruby_system, cpu):
        """CPUs are needed to grab the clock domain and system is needed for
        the cache block size.
        """
        super().__init__()

        self.version = self.versionCount()
        block_size_bits = get_block_size_bits(system)
        self.Icache = RubyCache(
            size="16KiB",
            assoc=8,
            is_icache=True,
            start_index_bit=block_size_bits,
        )
        self.Dcache = RubyCache(
            size="16KiB",
            assoc=8,
            is_icache=False,
            start_index_bit=block_size_bits,
        )
        self.prefetcher = RubyPrefetcher(block_size=system.cache_line_size)
        self.enable_prefetch = False
        self.clk_domain = cpu.clk_domain
        self.send_evictions = self.sendEvicts(cpu)
        self.ruby_system = ruby_system
        self.connectQueues(ruby_system)

    def sendEvicts(self, cpu):
        """True if the CPU model or ISA requires sending evictions from caches
        to the CPU. Two scenarios warrant forwarding evictions to the CPU:
        1. The O3 model must keep the LSQ coherent with the caches
        2. The x86 mwait instruction is built on top of coherence
        3. The local exclusive monitor in ARM systems

        As this is an X86 simulation we return True.
        """
        return True

    def connectQueues(self, ruby_system):
        """Connect the queues that don't leave this L1<->L2 pair or that
        never leave this controller at all. The link to the L2 controller is
        made separately in connectToL2() below, once both controllers exist.
        """
        # mandatoryQueue is a special variable. It is used by the sequencer to
        # send RubyRequests from the CPU (or other processor). It isn't
        # explicitly connected to anything.
        self.mandatoryQueue = MessageBuffer()
        # Buffer for prefetch requests; not connected to the network.
        self.prefetchQueue = MessageBuffer()

    def connectToL2(self, l2):
        """Connect this private L1 to its private L2. This link is
        point-to-point and does not go through the Ruby network.
        """
        self.bufferToL1 = MessageBuffer(ordered=True)
        l2.bufferFromL0 = self.bufferToL1
        self.bufferFromL1 = MessageBuffer(ordered=True)
        l2.bufferToL0 = self.bufferFromL1


class L2Cache(MESI_Three_Level_L1Cache_Controller):
    """Private, per-core, unified L2 cache."""

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, system, ruby_system, cpu):
        super().__init__()

        self.version = self.versionCount()
        self.cache = RubyCache(
            size="128KiB",
            assoc=8,
            start_index_bit=get_block_size_bits(system),
        )
        # We only have a single shared L3 bank, so no address bits are
        # needed to select between banks.
        self.l2_select_num_bits = 0
        self.ruby_system = ruby_system

    def connectToNetwork(self, network):
        """Connect the queues that go to/from the shared L3 (protocol
        MachineType L2) through the Ruby network.
        """
        self.requestToL2 = MessageBuffer()
        self.requestToL2.out_port = network.in_port
        self.responseToL2 = MessageBuffer()
        self.responseToL2.out_port = network.in_port
        self.unblockToL2 = MessageBuffer()
        self.unblockToL2.out_port = network.in_port

        self.requestFromL2 = MessageBuffer()
        self.requestFromL2.in_port = network.out_port
        self.responseFromL2 = MessageBuffer()
        self.responseFromL2.in_port = network.out_port


class L3Cache(MESI_Three_Level_L2Cache_Controller):
    """Shared last-level cache. Only a single bank is created, matching the
    single directory controller/memory controller used by this system.
    """

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, system, ruby_system):
        super().__init__()

        self.version = self.versionCount()
        self.L2cache = RubyCache(
            size="2MiB",
            assoc=16,
            start_index_bit=get_block_size_bits(system),
        )
        self.ruby_system = ruby_system

    def connectToNetwork(self, network):
        self.DirRequestFromL2Cache = MessageBuffer()
        self.DirRequestFromL2Cache.out_port = network.in_port
        self.L1RequestFromL2Cache = MessageBuffer()
        self.L1RequestFromL2Cache.out_port = network.in_port
        self.responseFromL2Cache = MessageBuffer()
        self.responseFromL2Cache.out_port = network.in_port

        self.unblockToL2Cache = MessageBuffer()
        self.unblockToL2Cache.in_port = network.out_port
        self.L1RequestToL2Cache = MessageBuffer()
        self.L1RequestToL2Cache.in_port = network.out_port
        self.responseToL2Cache = MessageBuffer()
        self.responseToL2Cache.in_port = network.out_port


class DirController(MESI_Three_Level_Directory_Controller):

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, ruby_system, ranges, mem_ctrls):
        """ranges are the memory ranges assigned to this controller."""
        if len(mem_ctrls) > 1:
            panic("This cache system can only be connected to one mem ctrl")
        super().__init__()
        self.version = self.versionCount()
        self.addr_ranges = ranges
        self.ruby_system = ruby_system
        self.directory = RubyDirectoryMemory(
            block_size=ruby_system.block_size_bytes
        )
        # Connect this directory to the memory side.
        self.memory = mem_ctrls[0].port
        # These are other special message buffers. They are used to send
        # requests to memory and responses from memory back to the
        # controller. Any messages sent or received on the memory port (see
        # self.memory above) will be directed through these message buffers.
        self.requestToMemory = MessageBuffer()
        self.responseFromMemory = MessageBuffer()

    def connectToNetwork(self, network):
        self.requestToDir = MessageBuffer(ordered=True)
        self.requestToDir.in_port = network.out_port
        self.responseToDir = MessageBuffer(ordered=True)
        self.responseToDir.in_port = network.out_port

        self.responseFromDir = MessageBuffer(ordered=True)
        self.responseFromDir.out_port = network.in_port


class MyNetwork(SimpleNetwork):
    """A simple point-to-point network. This doesn't not use garnet."""

    def __init__(self, ruby_system):
        super().__init__()
        self.netifs = []
        self.ruby_system = ruby_system

    def connectControllers(self, controllers):
        """Connect all of the controllers to routers and connec the routers
        together in a point-to-point network.
        """
        # Create one router/switch per controller in the system
        self.routers = [Switch(router_id=i) for i in range(len(controllers))]

        # Make a link from each controller to the router. The link goes
        # externally to the network.
        self.ext_links = [
            SimpleExtLink(link_id=i, ext_node=c, int_node=self.routers[i])
            for i, c in enumerate(controllers)
        ]

        # Make an "internal" link (internal to the network) between every pair
        # of routers.
        link_count = 0
        int_links = []
        for ri in self.routers:
            for rj in self.routers:
                if ri == rj:
                    continue  # Don't connect a router to itself!
                link_count += 1
                int_links.append(
                    SimpleIntLink(link_id=link_count, src_node=ri, dst_node=rj)
                )
        self.int_links = int_links
