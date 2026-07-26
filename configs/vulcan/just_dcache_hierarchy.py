from m5.objects import (
    BadAddr,
    Cache,
    SystemXBar,
    BaseSetAssoc,
    Ceaser,
    TaggedSetAssociative
)
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.cachehierarchies.classic.abstract_classic_cache_hierarchy import (
    AbstractClassicCacheHierarchy,
)
from gem5.isas import ISA
from gem5.utils.override import *
import os

class DCache(Cache):
    """Simple data cache with default values, direct mapped, no prefetcher"""

    # Set the default size
    assoc = 1
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20
    tags = BaseSetAssoc()
    tags.indexing_policy = Ceaser()

    def __init__(self, ceaser, size, box_file=None):
        self.size = size
        super().__init__()
        if ceaser == "ceaser":
            policy = Ceaser()
            if box_file is None:
                # Default: per-size curated "known good" box.
                policy.box_file = os.getcwd() + "/configs/vulcan/box/" + size + ".txt"
            else:
                # "" means fully random (Ceaser generates + logs a fresh
                # sbox/pbox/key); a path means load that frozen box.
                policy.box_file = box_file
            self.tags.indexing_policy = policy
        else:
            self.tags.indexing_policy = TaggedSetAssociative()


    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU dcache port"""
        self.cpu_side = cpu.dcache_port
    
    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.cpu_side_ports



# from cachehierarchies/classic/private_l1_cache_hierarchy.py
class JustDCacheHierarchy(AbstractClassicCacheHierarchy):
    def __init__(self, ceaser="", cache_size="16KiB", box_file=None):
        super().__init__()
        self.membus = SystemXBar(width=64)
        self.membus.badaddr_responder = BadAddr()
        self.membus.default = self.membus.badaddr_responder.pio
        self._ceaser = ceaser
        self._cache_size = cache_size
        self._box_file = box_file

    def get_l1dcache_size(self):
        return self.l1dcache.size 

    @overrides(AbstractClassicCacheHierarchy)
    def get_mem_side_port(self):
        return self.membus.mem_side_ports

    @overrides(AbstractClassicCacheHierarchy)
    def get_cpu_side_port(self):
        return self.membus.cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard):
        # Set up the system port for functional access from the simulator.
        board.connect_system_port(self.membus.cpu_side_ports)

        for _, port in board.get_mem_ports():
            self.membus.mem_side_ports = port

        assert board.get_processor().get_num_cores() == 1

        self.l1dcache = DCache(self._ceaser, self._cache_size, self._box_file)

        if board.has_coherent_io():
            self._setup_io_cache(board)

        cpu = board.get_processor().get_cores()[0]

        # no icache, connect cpu to membus directly
        cpu.connect_icache(self.membus.cpu_side_ports)

        cpu.connect_dcache(self.l1dcache.cpu_side)
        self.l1dcache.mem_side = self.membus.cpu_side_ports

        if board.get_processor().get_isa() == ISA.X86:
            int_req_port = self.membus.mem_side_ports
            int_resp_port = self.membus.cpu_side_ports
            cpu.connect_interrupt(int_req_port, int_resp_port)
        else:
            cpu.connect_interrupt()
