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
# Non-deterministic cache via Cache Decay: reuse-based side-channel harness.
#
# Threat model (reuse/Prime+Probe determinism; Keramidas et al., DAES 2008):
#   The victim performs a secret-dependent access to a line S. The attacker
#   then *probes* whether S is resident/fast (a hit) to recover which line the
#   victim touched. The attack relies on the cache being DETERMINISTIC: a line
#   the victim installed stays installed until the attacker evicts it.
#
# Cache Decay breaks that determinism: every installed line is given a
# RANDOMIZED lifetime and self-invalidates when the lifetime elapses. So
# between the victim's access and the attacker's probe, S may have randomly
# decayed away -- the probe result no longer reliably reflects the victim.
#
# Per trial the harness plays three phases through a single classic cache:
#   1. flush : stream a buffer much larger than the cache, evicting S so the
#              victim access is a genuine miss that then installs S.
#   2. victim: one read to the secret line S (installs S in the cache).
#   3. gap   : idle for a fixed interval, during which S may randomly decay.
#   4. probe : one read to S; stats are reset just before it, so the dumped
#              stat block reflects the probe alone. overallHits==1 means the
#              attacker recovered the line (S survived); 0 means S had decayed.
#
# With a NORMAL cache S never decays, so the probe hits every trial
# (success_rate ~ 1.0 -- a perfect, deterministic channel). With Cache Decay
# the gap is chosen to straddle the randomized lifetime distribution, so S
# survives to the probe only about half the time and success_rate collapses
# toward chance (~0.5): the attacker can no longer distinguish the victim's
# access from noise.
#
# --directed runs a small, deterministic set of checks (dirty writeback on
# decay, self-invalidation, and touch-reschedule) with per-phase stat dumps so
# an external checker can assert the decay mechanism's core invariants.
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
        help="Whether Cache Decay is enabled on the cache",
    )
    p.add_argument("--trials", type=int, default=200)
    p.add_argument("--seed", type=int, default=1)
    p.add_argument(
        "--gap-ns",
        type=int,
        default=1000,
        help="Idle gap (ns) between the victim install and the attacker "
        "probe; the decay lifetime distribution is centred on this so S "
        "survives about half the time.",
    )
    p.add_argument(
        "--interval-ns",
        type=int,
        default=None,
        help="Base/mean decay lifetime (ns). Defaults to --gap-ns so the "
        "distribution is centred on the prime->probe gap (~chance).",
    )
    p.add_argument(
        "--range-ns",
        type=int,
        default=None,
        help="Randomization half-width of the decay lifetime (ns). Defaults "
        "to half of the interval.",
    )
    p.add_argument(
        "--cache-size", default="32KiB", help="Cache capacity (small so the "
        "flush buffer reliably evicts the secret line)"
    )
    p.add_argument("--assoc", type=int, default=8)
    p.add_argument(
        "--directed",
        action="store_true",
        help="Run the deterministic invariant checks with per-phase dumps",
    )
    return p.parse_args()


def build_system(args, interval_ns, range_ns):
    system = System()
    system.clk_domain = SrcClockDomain(
        clock="1GHz", voltage_domain=VoltageDomain()
    )
    system.mem_mode = "timing"
    system.cache_line_size = BLK

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
        cache_kwargs["decay_enabled"] = True
        cache_kwargs["decay_interval"] = "%dns" % interval_ns
        cache_kwargs["decay_range"] = "%dns" % range_ns

    system.cache = NoncoherentCache(**cache_kwargs)
    system.cache.mem_side = system.membus.cpu_side_ports

    system.tgen = PyTrafficGen()
    system.tgen.port = system.cache.cpu_side
    return system


def run_directed(args, tgen, S, T, interval_ns, range_ns):
    """Deterministic checks of the decay mechanism.

    Uses a decay lifetime whose maximum (interval + range) is far shorter than
    the long idle, so a line is GUARANTEED to have decayed by the time we
    probe -- making the invariants deterministic and safe to assert.
    """
    period = 1000
    dur = 0
    # A long idle, >> the maximum lifetime, guarantees decay has fired.
    long_idle = (interval_ns + range_ns) * 20 * 1000  # ns -> ticks, x20
    # A short idle, << the minimum lifetime, guarantees NO decay in between.
    short_idle = max(1, (interval_ns - range_ns) // 4) * 1000

    def one_write(addr):
        # read_percent=0 => a write, so the installed line is dirty. Use a
        # partial (8-byte) write, not a whole-line write: a whole-line write
        # takes gem5's write-coalescing path (which needs a WriteAllocator),
        # whereas a partial write simply installs the line dirty via RFO.
        wsz = 8
        return tgen.createLinear(dur, addr, addr + wsz, wsz, period, period,
                                 0, wsz)

    def one_read(addr):
        return tgen.createLinear(dur, addr, addr + BLK, BLK, period, period,
                                 100, BLK)

    gens = []
    # -- Phase A: a DIRTY line self-invalidates with a writeback --
    gens.append(one_write(S))                # install S dirty
    gens.append(tgen.createExit(1))
    gens.append(tgen.createIdle(long_idle))  # S decays (dirty -> writeback)
    gens.append(tgen.createExit(1))
    # -- probe S: it must be gone (a miss) --
    gens.append(one_read(S))
    gens.append(tgen.createExit(1))
    # -- Phase B: a hit reschedules (touches) the decay deadline. Read T,
    # wait a short idle (long enough for the fill to complete, but far shorter
    # than the minimum decay lifetime so T does NOT decay), then read T again:
    # the second read is a genuine cache hit that touches (reschedules) it. --
    gens.append(one_read(T))                 # miss, installs T (arms decay)
    gens.append(tgen.createIdle(short_idle))  # let the fill land (no decay)
    gens.append(one_read(T))                 # hit -> touch (reschedule)
    gens.append(tgen.createExit(1))
    gens.append(tgen.createExit(1))
    tgen.start(gens)

    m5.simulate()  # install S dirty
    m5.stats.reset()
    m5.simulate()  # long idle: S decays, dirty writeback
    m5.stats.dump(message="nd-decay")
    m5.stats.reset()
    m5.simulate()  # probe S -> miss (S non-resident)
    m5.stats.dump(message="nd-probe")
    m5.stats.reset()
    m5.simulate()  # T: install then hit-touch (reschedule)
    m5.stats.dump(message="nd-touch")
    m5.stats.reset()
    m5.simulate()  # drain
    print("DONE directed (see nd-decay / nd-probe / nd-touch blocks)")


def run_sweep(args, tgen, targets, flush_lo, flush_hi):
    period = 1000
    dur = 0
    flush_size = flush_hi - flush_lo
    flush_idle = 5_000_000  # let flush MSHRs drain
    gap = args.gap_ns * 1000  # ns -> ticks

    def flush_gen():
        return tgen.createLinear(dur, flush_lo, flush_hi, BLK, period, period,
                                 100, flush_size)

    def one_read(addr):
        return tgen.createLinear(dur, addr, addr + BLK, BLK, period, period,
                                 100, BLK)

    gens = []
    for s in targets:
        gens.append(flush_gen())
        gens.append(tgen.createIdle(flush_idle))
        gens.append(tgen.createExit(1))       # -> stop after flush
        gens.append(one_read(s))              # victim installs S
        gens.append(tgen.createIdle(gap))     # gap: S may randomly decay
        gens.append(tgen.createExit(1))       # -> stop after victim+gap
        gens.append(one_read(s))              # attacker probe of S
        gens.append(tgen.createExit(1))       # -> stop after probe
    gens.append(tgen.createExit(1))
    tgen.start(gens)

    for _ in targets:
        m5.simulate()          # flush (+ drain)
        m5.stats.reset()       # drop flush stats
        m5.simulate()          # victim install + gap (S may decay here)
        m5.stats.reset()       # isolate the probe
        m5.simulate()          # probe
        m5.stats.dump(message="nd-probe")  # overallHits==1 iff S survived
        m5.stats.reset()

    m5.simulate()  # drain final exit
    print(
        "DONE defense=%s trials=%d gap=%dns interval=%dns range=%dns"
        % (args.defense, len(targets), args.gap_ns,
           args.interval_ns, args.range_ns)
    )


def main():
    args = parse_args()

    # Centre the decay distribution on the prime->probe gap by default.
    args.interval_ns = args.interval_ns if args.interval_ns is not None \
        else args.gap_ns
    args.range_ns = args.range_ns if args.range_ns is not None \
        else max(1, args.interval_ns // 2)

    # Memory layout: a small region for the secret line(s) and a large,
    # disjoint flush buffer far bigger than the cache.
    prot_lo = 0x0010_0000  # 1 MiB
    prot_hi = 0x0020_0000  # 2 MiB
    flush_lo = 0x0100_0000  # 16 MiB
    flush_hi = 0x0110_0000  # 17 MiB (1 MiB >> cache size)

    system = build_system(args, args.interval_ns, args.range_ns)
    root = Root(full_system=False, system=system)
    m5.instantiate()

    if args.directed:
        S = prot_lo
        T = prot_lo + 0x1000  # a distinct line for the touch check
        run_directed(args, system.tgen, S, T,
                     args.interval_ns, args.range_ns)
        return

    rng = random.Random(args.seed)
    num_lines = (prot_hi - prot_lo) // BLK
    targets = [prot_lo + rng.randrange(num_lines) * BLK
               for _ in range(args.trials)]
    run_sweep(args, system.tgen, targets, flush_lo, flush_hi)


main()
