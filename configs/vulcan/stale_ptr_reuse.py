#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# Cache Decay stale-pointer / TOCTOU regression check.
#
# processDecay() defers eviction of an expired-but-busy line by caching a raw
# CacheBlk* in decayPendingBusy and rechecking it directly on retry, instead
# of rediscovering it by address on a fresh tag-array scan. If that block's
# physical slot is reallocated to a *different* line while still pending, the
# retry loop keeps polling the same physical slot under the old pointer and
# has no way to tell the line's identity changed underneath it.
#
# This harness makes that concrete with a single direct-mapped (assoc=1)
# line:
#   1. install A (dirty)                  -- A's deadline will elapse busy
#   2. install B in A's slot               -- evicts A; B is a fresh, distinct
#      line with its own real deadline
#   3. touch B (a genuine hit)             -- legitimately RENEWS B's real
#      decayDeadline into the future (ndDecayReschedules, dumped as nd-touch)
#   4. probe B shortly after               -- dumped as nd-probe
#
# With the correct mechanism, B's renewed deadline protects it: the probe is
# a HIT. With the stale-pointer bug, the pending retry loop never re-checks
# B's decayDeadline at all -- it only rechecks busy() -- so as soon as
# whatever busy condition was inherited from A's slot clears, B gets
# decay-invalidated regardless of its just-renewed deadline: the probe is a
# MISS despite the renewal.
# ---------------------------------------------------------------------------

import argparse

import m5
from m5.objects import *

BLK = 64


def parse_args():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--interval-ns", type=int, default=5)
    p.add_argument("--mem-latency-ns", type=int, default=30)
    p.add_argument("--data-latency-cycles", type=int, default=50)
    p.add_argument(
        "--evict-delay-ns", type=int, default=45,
        help="Idle after installing A before installing B; must span past "
        "A's decay deadline so A is discovered expired-but-busy."
    )
    p.add_argument(
        "--touch-delay-ns", type=int, default=88,
        help="Idle after installing B before touching it; calibrated to "
        "land while the slot is still busy (inherited from A) but before "
        "the pending retry would otherwise have resolved it."
    )
    p.add_argument("--post-touch-delay-ns", type=int, default=2)
    return p.parse_args()


def build_system(args):
    system = System()
    system.clk_domain = SrcClockDomain(
        clock="1GHz", voltage_domain=VoltageDomain()
    )
    system.mem_mode = "timing"
    system.cache_line_size = BLK

    system.mem_ctrl = SimpleMemory(
        range=AddrRange("64MiB"), bandwidth="100GB/s",
        latency="%dns" % args.mem_latency_ns
    )
    system.membus = IOXBar(width=16)
    system.mem_ctrl.port = system.membus.mem_side_ports
    system.system_port = system.membus.cpu_side_ports

    # A single direct-mapped line: any two distinct addresses collide into
    # the same physical slot, guaranteeing B reuses A's slot.
    system.cache = NoncoherentCache(
        size="%dB" % BLK, assoc=1,
        tag_latency=1, data_latency=args.data_latency_cycles,
        response_latency=1,
        mshrs=4, tgts_per_mshr=8, write_buffers=4,
        decay_enabled=True,
        decay_interval="%dns" % args.interval_ns,
        decay_range="0ns",  # deterministic: every lifetime == interval
    )
    system.cache.mem_side = system.membus.cpu_side_ports

    system.tgen = PyTrafficGen()
    system.tgen.port = system.cache.cpu_side
    return system


def main():
    args = parse_args()
    system = build_system(args)
    root = Root(full_system=False, system=system)
    m5.instantiate()

    A = 0x0010_0000
    B = 0x0020_0000

    tgen = system.tgen
    period = 1000
    dur = 0
    wsz = 8

    def one_write(addr):
        return tgen.createLinear(dur, addr, addr + wsz, wsz, period, period,
                                  0, wsz)

    def one_read(addr):
        return tgen.createLinear(dur, addr, addr + BLK, BLK, period, period,
                                  100, BLK)

    evict_delay = args.evict_delay_ns * 1000
    touch_delay = args.touch_delay_ns * 1000
    post_touch_delay = args.post_touch_delay_ns * 1000

    gens = [
        one_write(A),                       # install A dirty
        tgen.createIdle(evict_delay),       # A's deadline elapses, busy
        tgen.createExit(1),
        one_write(B),                       # install B: evicts A's slot
        tgen.createIdle(touch_delay),       # wait near the inherited busy
        tgen.createExit(1),                 # window's clear point
        one_read(B),                        # touch B: legit hit, renews
        tgen.createIdle(post_touch_delay),  # B's real decayDeadline
        tgen.createExit(1),
        one_read(B),                        # probe: hit iff B survived
        tgen.createExit(1),
        tgen.createExit(1),
    ]
    tgen.start(gens)

    m5.simulate()               # install A, idle
    m5.stats.reset()
    m5.simulate()                # install B, idle
    m5.stats.reset()
    m5.simulate()                # touch B (hit + renew)
    m5.stats.dump(message="nd-touch")
    m5.stats.reset()
    m5.simulate()                # probe B
    m5.stats.dump(message="nd-probe")
    m5.simulate()                # drain

    print("DONE stale-ptr-reuse (see nd-touch / nd-probe blocks)")


main()
