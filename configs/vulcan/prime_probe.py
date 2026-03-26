from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.simulate.simulator import Simulator
from gem5.utils.override import *
from just_dcache_hierarchy import JustDCacheHierarchy
from prime_probe_generator import PrimeProbeGenerator
import math
import random
import sys

cache_hierarchy = JustDCacheHierarchy("ceaser") # "ceaser" for encrypted indexing
                                                # "" for default indexing policy
memory = SingleChannelDDR3_1600(size="1GiB") #30 bit address

# https://www.gem5.org/assets/files/hpca2023-tutorial/gem5-tutorial-hpca-2023.pdf slide 51
num_accesses = int(sys.argv[1])

# victim_accesses = [0xcafecaf]
victim_accesses = [0] * num_accesses
for i in range(num_accesses):
    num = random.getrandbits(28) << 2
    victim_accesses[i] = num + 256 if num < 256 else num

print('[{}]\n'.format(', '.join(hex(x) for x in victim_accesses)))


generator = PrimeProbeGenerator(victim_accesses)

board = TestBoard(
    clk_freq="3GHz",
    generator=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)


# Setup the Simulator and run the simulation.
print("cacheline size: " + str(board.get_cache_line_size()))
print("memory size: " + str(memory.get_size()))
print("addr width: " + str(math.log2(board.get_mem_ranges()[0].size())))

simulator = Simulator(board=board)
simulator.run()

