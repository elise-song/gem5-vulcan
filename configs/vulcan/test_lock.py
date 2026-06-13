"""
run_test.py  —  gem5 config used by run_tests.py.
Accepts:
    --assoc=N
    --cache_size=XKiB
    --binary=/path/to/test_binary
"""

import argparse
import m5
from m5.objects import *
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.simulate.simulator import Simulator
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from just_dcache_hierarchy2 import JustDCacheHierarchy
from gem5.components.boards.simple_board import SimpleBoard
from gem5.resources.resource import BinaryResource

parser = argparse.ArgumentParser()
parser.add_argument("--assoc",      type=int, default=4)
parser.add_argument("--cache_size", type=str, default="16KiB")
parser.add_argument("--binary",     type=str, required=True)
args = parser.parse_args()

cache_hierarchy = JustDCacheHierarchy(
    assoc=args.assoc,
    ceaser="",
    cache_size=args.cache_size,
    locked_lru=True,
)

memory = SingleChannelDDR3_1600(size="512MiB")

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING,
    isa=ISA.X86,
    num_cores=1,
)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.set_se_binary_workload(BinaryResource(args.binary))

simulator = Simulator(board=board)
simulator.run()