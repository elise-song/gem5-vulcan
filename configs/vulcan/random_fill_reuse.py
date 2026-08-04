# Copyright (c) 2026 The Regents of The University of Michigan
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

# ---------------------------------------------------------------------------
# Random Fill cache: reuse-based side-channel evaluation harness.
#
# Threat model (reuse attack; Liu & Lee, MICRO 2014):
#   The victim performs a secret-dependent access to a line S inside a
#   protected region. The attacker then *probes* whether S is resident/fast
#   (a hit) to recover which line the victim touched.
#
# Per trial the harness plays three phases through a single classic cache:
#   1. flush : stream a buffer much larger than the cache, evicting S so the
#              victim access is a genuine miss (the case Random Fill acts on).
#   2. victim: one read to the secret line S (a protected demand read when the
#              defense is on).
#   3. probe : one read to S; stats are reset just before it, so the dumped
#              stat block reflects the probe alone. overallHits==1 means the
#              attacker recovered the line (S was resident); 0 means it did not.
#
# With a normal cache the victim access caches S, so the probe hits every time
# (success_rate ~ 1.0). With Random Fill the victim access does NOT cache S -- a
# random neighbour is cached instead -- so the probe misses (success_rate ~ 0).
#
# --directed runs a single trial and dumps per-phase stat blocks so an external
# checker can assert the defense's core invariant (S left non-resident, exactly
# one random fill injected).
# ---------------------------------------------------------------------------

import argparse
import random

import m5
from m5.objects import *

BLK = 64  # cache line size in bytes


def parse_args():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--defense",
        choices=["off", "on"],
        default="off",
        help="Whether Random Fill is enabled on the cache",
    )
    p.add_argument("--trials", type=int, default=200)
    p.add_argument("--window", type=int, default=8, help="Random-fill radius")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument(
        "--cache-size", default="32KiB", help="Cache capacity (small so the "
        "flush buffer reliably evicts the secret line)"
    )
    p.add_argument("--assoc", type=int, default=8)
    p.add_argument(
        "--directed",
        action="store_true",
        help="Run a single trial with per-phase stat dumps for assertion",
    )
    return p.parse_args()


def build_system(args, prot_lo, prot_hi):
    system = System()
    system.clk_domain = SrcClockDomain(
        clock="1GHz", voltage_domain=VoltageDomain()
    )
    system.mem_mode = "timing"
    system.cache_line_size = BLK

    # Backing memory large enough for the protected region and flush buffer.
    system.mem_ctrl = SimpleMemory(
        range=AddrRange("64MiB"), bandwidth="100GB/s", latency="30ns"
    )
    system.membus = IOXBar(width=16)
    system.mem_ctrl.port = system.membus.mem_side_ports
    system.system_port = system.membus.cpu_side_ports

    cache_kwargs = dict(
        size=args.cache_size,
        assoc=args.assoc,
        tag_latency=1,
        data_latency=1,
        response_latency=1,
        mshrs=4,
        tgts_per_mshr=8,
        write_buffers=4,
    )
    if args.defense == "on":
        cache_kwargs["random_fill_ranges"] = [AddrRange(prot_lo, prot_hi)]
        cache_kwargs["random_fill_window"] = args.window

    system.cache = NoncoherentCache(**cache_kwargs)
    system.cache.mem_side = system.membus.cpu_side_ports

    system.tgen = PyTrafficGen()
    system.tgen.port = system.cache.cpu_side
    return system


def main():
    args = parse_args()

    # Layout inside memory: a protected region and a large, disjoint flush
    # buffer that is far bigger than the cache.
    prot_lo = 0x0010_0000  # 1 MiB
    prot_hi = 0x0020_0000  # 2 MiB  (16384 protected lines)
    flush_lo = 0x0100_0000  # 16 MiB
    flush_hi = 0x0110_0000  # 17 MiB (1 MiB >> cache size)
    flush_size = flush_hi - flush_lo

    system = build_system(args, prot_lo, prot_hi)

    root = Root(full_system=False, system=system)
    m5.instantiate()

    tgen = system.tgen
    period = 1000  # ticks between packets (1 ns @ 1 GHz)
    # duration 0 => the generator transitions to the next one as soon as it
    # has issued its data_limit worth of accesses (rather than idling for a
    # fixed duration, which would trip the traffic gen's progress watchdog).
    dur = 0
    idle = 5_000_000  # 5 us: let the injected random fill complete

    rng = random.Random(args.seed)
    num_lines = (prot_hi - prot_lo) // BLK
    trials = 1 if args.directed else args.trials

    # Choose the secret line for each trial up front.
    targets = [prot_lo + rng.randrange(num_lines) * BLK for _ in range(trials)]

    def flush_gen():
        return tgen.createLinear(
            dur, flush_lo, flush_hi, BLK, period, period, 100, flush_size
        )

    def one_read_gen(addr):
        return tgen.createLinear(dur, addr, addr + BLK, BLK, period, period,
                                 100, BLK)

    def window_scan_gen(s):
        # Read every line in the window [s-W, s+W] (clamped to the region).
        lo = max(prot_lo, s - args.window * BLK)
        hi = min(prot_hi, s + (args.window + 1) * BLK)
        return tgen.createLinear(dur, lo, hi, BLK, period, period, 100, hi - lo)

    # Build one flat generator stream segmented by exit markers so that a
    # sequence of m5.simulate() calls can drive the phases and interleave
    # stats.reset()/dump() between them.
    gens = []
    for s in targets:
        gens.append(flush_gen())
        gens.append(tgen.createIdle(idle))       # let flush MSHRs drain
        gens.append(tgen.createExit(1))          # -> stop after flush
        gens.append(one_read_gen(s))             # victim read of S
        gens.append(tgen.createIdle(idle))       # let random fill settle
        gens.append(tgen.createExit(1))          # -> stop after victim
        if args.directed:
            gens.append(window_scan_gen(s))      # count resident window lines
            gens.append(tgen.createExit(1))      # -> stop after window scan
        gens.append(one_read_gen(s))             # attacker probe of S
        gens.append(tgen.createExit(1))          # -> stop after probe
    gens.append(tgen.createExit(1))
    tgen.start(gens)

    # Each trial contributes exactly one probe stat block (or, in --directed
    # mode, a victim block, a window-scan block, then a probe block). report.py
    # classifies rf-probe blocks by system.cache.overallHits::total to compute
    # the success_rate; directed_check.py inspects the victim / window blocks.
    for i in range(trials):
        m5.simulate()          # flush (+ settle so its MSHRs drain)
        m5.stats.reset()       # drop flush stats
        m5.simulate()          # victim (+ idle for the random fill)
        if args.directed:
            m5.stats.dump(message="rf-victim")  # rf* counters for victim
            m5.stats.reset()   # isolate the window scan
            m5.simulate()      # read every window line once
            # overallHits here == number of window lines resident after the
            # victim access: exactly the one injected random fill.
            m5.stats.dump(message="rf-window")
        m5.stats.reset()       # isolate the probe
        m5.simulate()          # probe
        m5.stats.dump(message="rf-probe")       # overallHits==1 iff recovered
        m5.stats.reset()

    m5.simulate()  # drain final exit
    print(
        "DONE defense=%s trials=%d window=%d -> see stats file "
        "(classify rf-probe blocks with report.py)"
        % (args.defense, trials, args.window)
    )


main()
