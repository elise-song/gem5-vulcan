from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.simulate.simulator import Simulator
from gem5.utils.override import *
from just_dcache_hierarchy import JustDCacheHierarchy
from prime_probe_generator import PrimeProbeGenerator
import math
import random
import sys

def get_num_sets(cache_size, block_size):
    units = {"KiB": 1024, "MiB": 1024**2, "GiB": 1024**3, "B": 1}
    for unit, multiplier in units.items():
        if cache_size.endswith(unit):
            size_bytes = int(cache_size[:-len(unit)]) * multiplier
            break
    else:
        raise ValueError(f"Unknown size unit in: {cache_size}")
    
    return size_bytes // int(block_size)

cache_size = sys.argv[2]
num_probes = cache_size

# Optional 3rd arg: box_file override.
#   omitted            -> default per-size curated "known good" box
#   "" (empty string)  -> fully random sbox/pbox/key (also logged via
#                         --debug-flags=Cache so it can be frozen later)
#   a path              -> load that frozen box (sbox/pbox, and key if present)
box_file = sys.argv[3] if len(sys.argv) > 3 else None

cache_hierarchy = JustDCacheHierarchy("ceaser", cache_size, box_file) # "ceaser" for encrypted indexing
                                                # "" for default indexing policy
memory = SingleChannelDDR3_1600(size="4GiB") #32 bit address

# https://www.gem5.org/assets/files/hpca2023-tutorial/gem5-tutorial-hpca-2023.pdf slide 51
num_accesses = int(sys.argv[1])

# victim_accesses = [0xcafecaf]
victim_accesses = [0] * num_accesses
for i in range(num_accesses):
    num = random.getrandbits(28) << 2
    victim_accesses[i] = num + 256 if num < 256 else num

print('[{}]\n'.format(', '.join(hex(x) for x in victim_accesses)))


generator = PrimeProbeGenerator(victim_accesses, get_num_sets(cache_size, 64))

board = TestBoard(
    clk_freq="3GHz",
    generator=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)


# Setup the Simulator and run the simulation.
print(f"num sets: {get_num_sets(cache_size, 64)}")
print("cacheline size: " + str(board.get_cache_line_size()))
print("memory size: " + str(memory.get_size()))
print("addr width: " + str(math.log2(board.get_mem_ranges()[0].size())))

simulator = Simulator(board=board)
simulator.run()

