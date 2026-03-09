from gem5.components.processors.linear_generator import LinearGenerator
from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.simulate.simulator import Simulator
from gem5.utils.override import *
from just_dcache_hierarchy import JustDCacheHierarchy
from prime_probe_generator import PrimeProbeGenerator
from check_mapping import CheckMapping


cache_hierarchy = JustDCacheHierarchy()
memory = SingleChannelDDR3_1600(size="2GiB")

# https://www.gem5.org/assets/files/hpca2023-tutorial/gem5-tutorial-hpca-2023.pdf slide 51
secret_keys = [0xcafe000, 0xcafe100, 0xcafe200, 0xcafe300, 0xcafe4e0]
for secret in secret_keys:
    print(hex((secret >> 6) & 0b11111111))
generator = PrimeProbeGenerator(secret_keys)

board = TestBoard(
    clk_freq="3GHz",
    generator=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)


# Setup the Simulator and run the simulation.
print("cacheline size: " + str(board.get_cache_line_size()))
print("memory size: " + str(memory.get_size()))

simulator = Simulator(board=board)
simulator.run()

print("success rate ", CheckMapping("configs/vulcan/debug.txt"))
