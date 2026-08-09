"""Single-core, single D-cache hierarchy shared by the PLcache benchmark
scripts. Modeled on just_dcache_hierarchy2.py (partition_generator
branch), but switches the *tags* class instead of the replacement policy:
"baseline" uses plain BaseSetAssoc (no locking at all), "plcache" uses
PartitionLockedTags with a configurable protected_ranges list.

The underlying replacement policy is fixed to Tree-PLRU.
"""

from m5.objects import (
    BadAddr,
    BaseSetAssoc,
    Cache,
    PartitionLockedTags,
    SystemXBar,
    TaggedSetAssociative,
    TreePLRURP,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.cachehierarchies.classic.abstract_classic_cache_hierarchy import (
    AbstractClassicCacheHierarchy,
)
from gem5.isas import ISA
from gem5.utils.override import overrides


class DCache(Cache):
    """Data cache whose tags class implements (or doesn't) the PL
    defense, sharing the same size/assoc either way."""

    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20

    def __init__(self, size, assoc, tags_mode, protected_ranges):
        super().__init__()
        self.size = size
        self.assoc = assoc
        self.replacement_policy = TreePLRURP()

        if tags_mode == "plcache":
            tags = PartitionLockedTags()
            tags.protected_ranges = protected_ranges
        elif tags_mode == "baseline":
            tags = BaseSetAssoc()
        else:
            raise ValueError(f"Unknown tags_mode: {tags_mode}")

        tags.indexing_policy = TaggedSetAssociative()
        self.tags = tags

    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports


class CacheHierarchy(AbstractClassicCacheHierarchy):
    def __init__(
        self,
        size="1KiB",
        assoc=2,
        tags_mode="baseline",
        protected_ranges=None,
    ):
        super().__init__()
        self.membus = SystemXBar(width=64)
        self.membus.badaddr_responder = BadAddr()
        self.membus.default = self.membus.badaddr_responder.pio
        self._size = size
        self._assoc = assoc
        self._tags_mode = tags_mode
        self._protected_ranges = protected_ranges or []

    @overrides(AbstractClassicCacheHierarchy)
    def get_mem_side_port(self):
        return self.membus.mem_side_ports

    @overrides(AbstractClassicCacheHierarchy)
    def get_cpu_side_port(self):
        return self.membus.cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard):
        board.connect_system_port(self.membus.cpu_side_ports)

        for _, port in board.get_mem_ports():
            self.membus.mem_side_ports = port

        assert board.get_processor().get_num_cores() == 1

        self.l1dcache = DCache(
            self._size,
            self._assoc,
            self._tags_mode,
            self._protected_ranges,
        )

        if board.has_coherent_io():
            self._setup_io_cache(board)

        cpu = board.get_processor().get_cores()[0]

        # No icache: connect straight to the membus.
        cpu.connect_icache(self.membus.cpu_side_ports)
        cpu.connect_dcache(self.l1dcache.cpu_side)
        self.l1dcache.mem_side = self.membus.cpu_side_ports

        if board.get_processor().get_isa() == ISA.X86:
            int_req_port = self.membus.mem_side_ports
            int_resp_port = self.membus.cpu_side_ports
            cpu.connect_interrupt(int_req_port, int_resp_port)
        else:
            cpu.connect_interrupt()
