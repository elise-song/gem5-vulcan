from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.simulate.simulator import Simulator
from gem5.utils.override import *
from just_dcache_hierarchy2 import JustDCacheHierarchy
from prime_probe_generator import PrimeProbeGenerator
from _m5 import sim as m5sim
import math
import random
import sys

# usage: gem5.opt run_prime_probe.py <num_accesses> <cache_size> <lock_mode> <assoc> [seed]
#   lock_mode: locked | unlocked | nolock
#   seed: optional int; use the SAME seed across all 3 scenarios so they
#         compare against identical victim_accesses.

def get_num_sets(cache_size, block_size, assoc):
    units = {"KiB": 1024, "MiB": 1024**2, "GiB": 1024**3, "B": 1}
    for unit, multiplier in units.items():
        if cache_size.endswith(unit):
            size_bytes = int(cache_size[:-len(unit)]) * multiplier
            break
    else:
        raise ValueError(f"Unknown size unit in: {cache_size}")

    total_blocks = size_bytes // int(block_size)
    return total_blocks // assoc

# https://www.gem5.org/assets/files/hpca2023-tutorial/gem5-tutorial-hpca-2023.pdf slide 51
num_accesses = int(sys.argv[1])
cache_size = sys.argv[2]
lock_mode = sys.argv[3]
assoc = int(sys.argv[4])

if lock_mode not in ("locked", "unlocked", "nolock"):
    raise ValueError(f"Unknown lock_mode: {lock_mode}")

if len(sys.argv) > 5:
    random.seed(int(sys.argv[5]))

num_sets = get_num_sets(cache_size, 64, assoc)

victim_accesses = []
for i in range(num_accesses):
    num = random.getrandbits(28) << 2
    victim_accesses.append(num + 256 if num < 256 else num)

print(f"lock_mode={lock_mode}")
print(f"assoc={assoc}, num_sets={num_sets}")
print('victim_accesses = [{}]\n'.format(', '.join(hex(x) for x in victim_accesses)))

cache_hierarchy = JustDCacheHierarchy(
    assoc=assoc,
    ceaser="",
    cache_size=cache_size,
    locked_lru=True
)
memory = SingleChannelDDR3_1600(size="4GiB")

generator = PrimeProbeGenerator(victim_accesses, num_sets)

board = TestBoard(
    clk_freq="3GHz",
    generator=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)
# Setup the Simulator and run the simulation.
print(f"num sets: {num_sets}")
print("cacheline size: " + str(board.get_cache_line_size()))
print("memory size: " + str(memory.get_size()))
print("addr width: " + str(math.log2(board.get_mem_ranges()[0].size())))

simulator = Simulator(board=board)

# block addresses corresponding to each secret
blk_addrs = sorted(set(secret & ~63 for secret in victim_accesses))

#all victim accesses done
simulator.run()

if lock_mode == "locked":
    for blk_addr in blk_addrs:
        print(f"[{lock_mode}] Lock line {hex(blk_addr)}")
        m5sim.cacheLock(blk_addr)
elif lock_mode == "unlocked":
    for blk_addr in blk_addrs:
        print(f"[{lock_mode}] Lock+immediately unlock line {hex(blk_addr)}")
        m5sim.cacheLock(blk_addr)
        m5sim.cacheUnlock(blk_addr)
else:  # nolock
    for blk_addr in blk_addrs:
        print(f"[{lock_mode}] No lock for line {hex(blk_addr)}")

#prime phase
simulator.run()

#victim reaccess and probe phase
simulator.run()

if lock_mode == "locked":
    for blk_addr in blk_addrs:
        m5sim.cacheUnlock(blk_addr)

