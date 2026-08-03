# Copyright (c) 2024 The Regents of the gem5-vulcan study.
# SPDX-License-Identifier: BSD-3-Clause
#
# Cross-domain Prime+Probe evaluation harness for the ScatterCache defense.
#
# Threat model: an ATTACKER (security domain 0) and a VICTIM (security domain
# 1) share a single cache. For each secret address the victim touches, the
# attacker tries to detect the access via Prime+Probe:
#
#   1. PRIME : the attacker (domain 0) fills the cache set(s) it believes the
#              secret maps to, using its own congruent addresses.
#   2. VICTIM: the victim (domain 1) accesses the secret.
#   3. PROBE : the attacker (domain 0) re-accesses its primed lines. A miss
#              means the victim evicted an attacker line, i.e. the secret was
#              detected.
#
# The attacker's eviction set is built with the CONVENTIONAL address->set map
# ((addr >> block_bits) & set_mask), which is all it can compute. With a plain
# set-associative (or plain skewed) cache this is exactly where the secret
# lands, so the channel is wide open (success_rate ~ 1.0). With ScatterCache
# and DISTINCT per-domain keys, the secret's real set is an unpredictable,
# key-dependent function of the address that the attacker cannot reproduce, so
# the primed lines and the victim's line almost never collide and the channel
# collapses to noise.
#
# The security domain is switched per phase through the _m5.sim.scatterSetDomain
# hook (a no-op for the baseline cache, which ignores the domain). This mirrors
# the reference PL defense's use of _m5.sim.cacheLock between run phases.
#
# Usage:
#   gem5.opt --debug-flags=Cache configs/vulcan/prime_probe.py \
#       <mode: baseline|scatter> <assoc> <cache_size> <num_secrets> <seed>

import random
import sys

import m5
from m5.objects import (
    AddrRange,
    BaseSetAssoc,
    Cache,
    LRURP,
    Root,
    ScatterAssociative,
    SimpleMemory,
    SrcClockDomain,
    System,
    SystemXBar,
    TaggedSetAssociative,
    VoltageDomain,
)
from _m5 import sim as m5sim

BLOCK_SIZE = 64
BLOCK_BITS = 6

ATTACKER_DOMAIN = 0
VICTIM_DOMAIN = 1

# Fixed, distinct, high-entropy per-domain keys so the experiment is fully
# reproducible. Domain 0 (attacker) and domain 1 (victim) get DIFFERENT keys;
# this is what gives each domain an independent address->set mapping.
DOMAIN_KEYS = [0xA5A5A5A5DEADBEEF, 0x5A5A5A5AFEEDFACE]


def size_to_bytes(size_str):
    units = {"KiB": 1024, "MiB": 1024**2, "GiB": 1024**3, "B": 1}
    for unit, mult in units.items():
        if size_str.endswith(unit):
            return int(size_str[: -len(unit)]) * mult
    raise ValueError(f"Unknown size unit in: {size_str}")


def build_system(mode, assoc, size):
    system = System()
    system.clk_domain = SrcClockDomain(
        clock="3GHz", voltage_domain=VoltageDomain()
    )
    system.mem_mode = "timing"
    system.mem_ranges = [AddrRange("2GiB")]

    system.membus = SystemXBar()

    system.mem_ctrl = SimpleMemory(
        range=system.mem_ranges[0], latency="30ns", bandwidth="0GiB/s"
    )
    system.mem_ctrl.port = system.membus.mem_side_ports

    # Tag store with the indexing policy under test.
    tags = BaseSetAssoc()
    if mode == "scatter":
        tags.indexing_policy = ScatterAssociative(keys=DOMAIN_KEYS)
    elif mode == "baseline":
        tags.indexing_policy = TaggedSetAssociative()
    else:
        raise ValueError(f"Unknown mode: {mode}")

    system.cache = Cache(
        size=size,
        assoc=assoc,
        tag_latency=1,
        data_latency=1,
        response_latency=1,
        mshrs=8,
        tgts_per_mshr=16,
        tags=tags,
        replacement_policy=LRURP(),
    )
    system.cache.mem_side = system.membus.cpu_side_ports

    system.tgen = PyTrafficGen()
    system.tgen.port = system.cache.cpu_side

    return system


# PyTrafficGen is not exported by name above; grab it lazily to keep the import
# block tidy.
from m5.objects import PyTrafficGen  # noqa: E402


def one_access(tgen, addr, read=True):
    # A single BLOCK_SIZE access at `addr` (data_limit == BLOCK_SIZE stops the
    # linear stream after exactly one block).
    return tgen.createLinear(
        10_000_000,          # duration cap (ticks)
        addr,                # start
        addr + BLOCK_SIZE,   # end
        BLOCK_SIZE,          # block size
        1,                   # min period
        1,                   # max period
        100 if read else 0,  # read percentage
        BLOCK_SIZE,          # data limit -> exactly one access
    )


def main():
    mode = sys.argv[1]
    assoc = int(sys.argv[2])
    size = sys.argv[3]
    num_secrets = int(sys.argv[4])
    seed = int(sys.argv[5]) if len(sys.argv) > 5 else 1

    random.seed(seed)

    size_bytes = size_to_bytes(size)
    num_sets = size_bytes // BLOCK_SIZE // assoc
    set_mask = num_sets - 1

    # Disjoint address regions for attacker and victim so no physical line is
    # ever shared between the two domains (keeps the classic cache free of
    # cross-domain aliasing; the leakage we measure is purely set-index based).
    attacker_base = 0x10000000
    victim_base = 0x40000000

    def conv_set(addr):
        return (addr >> BLOCK_BITS) & set_mask

    # Pick distinct victim secrets at random addresses in the victim region.
    secrets = []
    seen = set()
    while len(secrets) < num_secrets:
        blk = random.getrandbits(20)
        addr = victim_base + blk * BLOCK_SIZE
        if addr not in seen:
            seen.add(addr)
            secrets.append(addr)

    print(f"### mode={mode} assoc={assoc} size={size} "
          f"num_sets={num_sets} num_secrets={num_secrets} seed={seed}",
          flush=True)
    print("### victim_accesses = [{}]".format(
        ", ".join(hex(s) for s in secrets)), flush=True)

    system = build_system(mode, assoc, size)
    root = Root(full_system=False, system=system)
    m5.instantiate()

    tgen = system.tgen

    # Build the whole per-secret trace up front: for every secret, a PRIME
    # phase (attacker eviction set), a VICTIM phase (the secret) and a PROBE
    # phase (attacker eviction set again), each terminated by an Exit so we can
    # switch the active security domain between phases from Python.
    trace = []
    eviction_sets = []
    for s in secrets:
        tset = conv_set(s)
        # `assoc` attacker addresses that are congruent with the secret under
        # the conventional mapping -- the eviction set the attacker can build.
        evset = [
            attacker_base + (tset + k * num_sets) * BLOCK_SIZE
            for k in range(assoc)
        ]
        eviction_sets.append(evset)

        # PRIME
        for a in evset:
            trace.append(one_access(tgen, a))
        trace.append(tgen.createExit(0))
        # VICTIM
        trace.append(one_access(tgen, s))
        trace.append(tgen.createExit(0))
        # PROBE
        for a in evset:
            trace.append(one_access(tgen, a))
        trace.append(tgen.createExit(0))

    tgen.start(trace)

    # Drive the phases, switching the active security domain around each one.
    for j, s in enumerate(secrets):
        evset = eviction_sets[j]
        tset = conv_set(s)

        # PRIME as the attacker.
        m5sim.scatterSetDomain(ATTACKER_DOMAIN)
        print(f">>> PHASE PRIME secret={hex(s)} set={hex(tset)} "
              f"evset=[{', '.join(hex(a) for a in evset)}]", flush=True)
        m5.simulate()

        # VICTIM accesses the secret as its own domain.
        m5sim.scatterSetDomain(VICTIM_DOMAIN)
        print(f">>> PHASE VICTIM secret={hex(s)} set={hex(tset)}", flush=True)
        m5.simulate()

        # PROBE as the attacker again.
        m5sim.scatterSetDomain(ATTACKER_DOMAIN)
        print(f">>> PHASE PROBE secret={hex(s)} set={hex(tset)}", flush=True)
        m5.simulate()

    print(">>> DONE", flush=True)


main()
