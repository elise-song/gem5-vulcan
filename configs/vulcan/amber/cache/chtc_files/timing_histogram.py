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

This script runs the 'timing_histogram' binary (Shuwen Deng's cache-hierarchy
timing probe, configs/vulcan/amber/cache/src/timing_histogram.c) in FS mode
on a 4-core X86 timing processor with a MESI_Three_Level Ruby cache
hierarchy (private L1/L2 per core, shared L3).

FS mode boots Ubuntu 24.04, so the binary's sched_setaffinity() calls are
handled by a real Linux scheduler: it measures on CPU 1 and uses CPU 3 as
the "remote core" for the REMOTE_* / L*_REMOTE_L*_HIT_CLEAN categories.
That is why NUM_CORES must be at least 4.

The (statically linked) binary is embedded base64-encoded in the readfile
script, run inside the guest, and its histogram outputs are copied back to
the host's m5out/ directory with `m5 writefile` as `histogram.out` and
`coarse_histogram.out`.

Usage
-----

```
scons build/ALL/gem5.opt
gcc -std=gnu99 -O0 -static [-DDO_READ=1|-DDO_WRITE=1|-DDO_FLUSH=1] \
    configs/vulcan/amber/cache/src/timing_histogram.c \
    configs/vulcan/amber/cache/src/cpu_info.c \
    -o configs/vulcan/amber/cache/bin/timing_histogram
./build/ALL/gem5.opt \
    configs/vulcan/amber/cache/chtc_files/timing_histogram.py \
    --binary configs/vulcan/amber/cache/bin/timing_histogram [--kvm]
```

The -DDO_* flag picks which operation to histogram (none means all three);
chtc_files/timing_histogram.sh runs each as a separate job.

`--kvm` boots the guest on KVM cores and switches to timing cores once
systemd has booted (before the binary runs). Without it the whole boot is
simulated on timing cores, which is much slower.
"""

import argparse
import base64

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
from gem5.simulate.exit_handler import ExitHandler
from gem5.simulate.simulator import Simulator
from gem5.utils.override import overrides
from gem5.utils.requires import requires

parser = argparse.ArgumentParser()
parser.add_argument(
    "--binary",
    required=True,
    help="Path to the statically linked timing_histogram binary.",
)
parser.add_argument(
    "--kvm",
    action="store_true",
    help="Boot on KVM cores, then switch to timing cores after boot.",
)
args = parser.parse_args()

# This checks if the gem5 binary is compiled to include the MESI_Three_Level
# cache coherence protocol (and KVM, if requested).
requires(
    isa_required=ISA.X86,
    coherence_protocol_required=CoherenceProtocol.MESI_THREE_LEVEL,
    kvm_required=args.kvm,
)

from gem5.components.cachehierarchies.ruby.mesi_three_level_cache_hierarchy import (
    MESIThreeLevelCacheHierarchy,
)

# timing_histogram.c pins itself to CPU 1 and uses CPU 3 as the remote core.
NUM_CORES = 4

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

# Set up the system memory. The binary's probe array alone is ~1.3GiB, and
# 3GiB is the most the X86Board supports.
memory = SingleChannelDDR3_1600(size="3GiB")

# Here we set up the processor: a NUM_CORES-core X86 timing processor,
# optionally booting on KVM first.
if args.kvm:
    processor = SimpleSwitchableProcessor(
        starting_core_type=CPUTypes.KVM,
        switch_core_type=CPUTypes.TIMING,
        isa=ISA.X86,
        num_cores=NUM_CORES,
    )
else:
    processor = SimpleProcessor(
        cpu_type=CPUTypes.TIMING,
        isa=ISA.X86,
        num_cores=NUM_CORES,
    )

# Here we set up the board. The X86Board allows for FS mode (full system) or
# SE mode (syscall emulation) X86 simulations.
board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

with open(args.binary, "rb") as f:
    binary_b64 = base64.encodebytes(f.read()).decode()

# Run inside the guest after boot. The binary writes
# histogram_<cpuname>.out and coarse_histogram_<cpuname>.out into the
# directory given as its first argument; copy them out under fixed names.
command = f"""
M5=$(command -v gem5-bridge || command -v m5)
OUT=/tmp/timing_histogram_out
mkdir -p $OUT
base64 -d > /tmp/timing_histogram << 'EOF'
{binary_b64}EOF
chmod +x /tmp/timing_histogram
/tmp/timing_histogram $OUT
$M5 writefile $OUT/histogram_* histogram.out
$M5 writefile $OUT/coarse_histogram_* coarse_histogram.out
"""

board.set_kernel_disk_workload(
    kernel=obtain_resource("x86-linux-kernel-6.8.0-52-generic"),
    disk_image=obtain_resource("x86-ubuntu-24.04-img"),
    readfile_contents=command,
)

if args.kvm:

    class SwitchAfterBootExitHandler(ExitHandler, hypercall_num=2):
        @overrides(ExitHandler)
        def _process(self, simulator: "Simulator") -> None:
            print("Booted: switching from KVM to timing cores")
            simulator.switch_processor()

        @overrides(ExitHandler)
        def _exit_simulation(self) -> bool:
            return False


simulator = Simulator(board=board)

simulator.run()
