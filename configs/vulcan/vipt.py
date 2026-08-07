import math
import sys

from just_dcache_hierarchy import JustDCacheHierarchy

from gem5.components.boards.x86_board import X86Board
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator
from gem5.utils.override import *
from gem5.utils.requires import requires

requires(isa_required=ISA.X86)


def get_num_sets(cache_size, block_size):
    units = {"KiB": 1024, "MiB": 1024**2, "GiB": 1024**3, "B": 1}
    for unit, multiplier in units.items():
        if cache_size.endswith(unit):
            size_bytes = int(cache_size[: -len(unit)]) * multiplier
            break
    else:
        raise ValueError(f"Unknown size unit in: {cache_size}")

    return size_bytes // int(block_size)


cache_size = sys.argv[1]

cache_hierarchy = JustDCacheHierarchy(
    "", cache_size
)  # "ceaser" for encrypted indexing

# "" for default indexing policy
memory = SingleChannelDDR3_1600(size="3GiB")  # 32 bit address

# Real CPU + TLB, so accesses carry a genuine virtual address instead of
# the traffic generator's physical-only requests (see prime_probe.py).
processor = SimpleProcessor(cpu_type=CPUTypes.TIMING, num_cores=1, isa=ISA.X86)

board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

# Run the real prime/probe binary (built from prime_cache.c) so the CPU's
# own loads/stores generate real virtual addresses through the TLB.
board.set_se_binary_workload(
    BinaryResource(local_path="configs/vulcan/prime_cache")
)


# Setup the Simulator and run the simulation.
print(f"num sets: {get_num_sets(cache_size, 64)}")
print("cacheline size: " + str(board.get_cache_line_size()))
print("memory size: " + str(memory.get_size()))
print("addr width: " + str(math.log2(board.get_mem_ranges()[0].size())))

simulator = Simulator(board=board)
simulator.run()
