# Copyright (c) 2021-2025 The Regents of the University of California
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

"""

This script runs the 'timing_histogram' SE-mode binary (a simple
multi-threaded application with false sharing used to stress the cache
hierarchy) on a 2-core X86 O3 processor with a MESI_Three_Level Ruby cache
hierarchy (private L1/L2 per core, shared L3).

Usage
-----

```
scons build/ALL/gem5.opt
./build/ALL/gem5.opt configs/vulcan/amber_cache/timing_histogram.py
```
"""

import os

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import (
    SimpleProcessor,
)
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

# This checks if the gem5
# binary is compiled to include the MESI_Three_Level cache coherence protocol.
requires(
    coherence_protocol_required=CoherenceProtocol.MESI_THREE_LEVEL,
)

from gem5.components.cachehierarchies.ruby.mesi_three_level_cache_hierarchy import (
    MESIThreeLevelCacheHierarchy,
)

# Here we set up a MESI Three Level Cache Hierarchy.
cache_hierarchy = MESIThreeLevelCacheHierarchy(
    l1i_size="32KiB",
    l1i_assoc=8,
    l1d_size="32KiB",
    l1d_assoc=8,
    l2_size="256KiB",
    l2_assoc=8,
    l3_size="10MiB",
    l3_assoc=20,
    num_l3_banks=1,
)

# Set up the system memory.
memory = SingleChannelDDR3_1600(size="3GiB")

# Here we set up the processor. 2-core X86 timing processor. 

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING,
    isa=ISA.X86,
    num_cores=2,
)

# Here we set up the board. The X86Board allows for FS mode (full system) or
# SE mode (syscall emulation) X86 simulations.

board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)


thispath = os.path.dirname(os.path.abspath(__file__))
binary_path = os.path.join(thispath, "timing_histogram")
binary = BinaryResource(local_path=binary_path)
board.set_se_binary_workload(binary, arguments=["gem5"])

simulator = Simulator(board=board)

simulator.run()
