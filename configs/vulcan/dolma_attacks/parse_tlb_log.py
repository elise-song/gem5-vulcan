#!/usr/bin/env python3
# Determines whether control_mem_dtlb_store leaked its secret, from a gem5
# stdout log captured with --debug-flags=TLB,PseudoInst.
#
# This is a from-scratch reimplementation of the DOLMA paper's own
# scripts/parse_tlb_logs.py (github.com/efeslab/dolma/attacks). That script
# toggles the TLB debug flag on and off around the probe access via
# m5_work_begin() exit-event hooks in a custom SE-mode config script, which
# this repo's harness does not replicate. This one gets the same phase
# separation by leaving TLB logging on throughout and using the markers
# themselves -- which PseudoInst prints as "pseudo_inst::workbegin(42, 0)"
# and "...(43, 0)" -- to bracket the probe.
#
# The attack's outer loop runs, for each slot i in 0..255:
#
#     <training + one speculative, secret-indexed store>
#     m5_work_begin(42, 0)          <- probe window opens
#     time a single read of array2[i * STEP_SIZE]
#     m5_work_begin(43, 0)          <- probe window closes
#
# The probe is the only access to a slot address inside that window, so
# whether it took a dTLB miss is exactly the signal the attacker is timing:
#
#   - Unprotected: the speculative store's translation is installed, so slot
#     SECRET_VALUE is already TLB-resident when its probe runs and the probe
#     HITS, while every other slot's probe misses. The outlier identifies the
#     secret.
#   - Protected: the speculative store leaves no resident mapping, so slot
#     SECRET_VALUE's probe misses just like every other slot's and there is
#     no outlier to read the secret out of.
#
# Note the miss counts are per probe window, so a miss on some other page
# (the stack, the timer's own spill slot) is filtered out by only counting
# addresses that the program printed as slot addresses. In SE mode a dTLB
# miss is functionally free, so there is no timing difference to measure
# directly -- the log is the proxy for what would be observable on real
# hardware, which is why the paper's suite reads this channel out of a
# debug log rather than out of the attack's own rdtscp deltas.

import re
import sys
from argparse import ArgumentParser

SECRET_VALUE = 42
PROBE_OPEN = "pseudo_inst::workbegin(42, 0)"
PROBE_CLOSE = "pseudo_inst::workbegin(43, 0)"


def get_slot_addresses(lines):
    slots = {}
    for line in lines:
        m = re.match(
            r"array2\[(\d+) \* STEP_SIZE\] @ (0x[0-9a-fA-F]+)", line.strip()
        )
        if m:
            idx, addr = m.groups()
            slots[addr] = int(idx)
    return slots


def collect_probe_misses(lines, slots):
    """Slots whose probe took a dTLB miss, and the number of probe windows
    seen, so a truncated or marker-less log can be reported as such."""
    missed = set()
    windows = 0
    in_window = False
    for line in lines:
        if PROBE_OPEN in line:
            in_window = True
            windows += 1
            continue
        if PROBE_CLOSE in line:
            in_window = False
            continue
        if not in_window:
            continue
        if "Handling a TLB miss for address" not in line:
            continue
        m = re.search(r"address (0x[0-9a-fA-F]+)", line)
        if m and m.group(1) in slots:
            missed.add(slots[m.group(1)])
    return missed, windows


def main():
    parser = ArgumentParser(
        description="Parse control_mem_dtlb_store's gem5 log"
    )
    parser.add_argument(
        "log",
        help="gem5 stdout log, captured with --debug-flags=TLB,PseudoInst",
    )
    args = parser.parse_args()

    with open(args.log, errors="replace") as f:
        lines = f.readlines()

    slots = get_slot_addresses(lines)
    if len(slots) < 200:
        print(
            f"WARNING: only found {len(slots)}/256 slot addresses in the log; "
            "did the program run to completion?",
            file=sys.stderr,
        )

    missed, windows = collect_probe_misses(lines, slots)
    if windows < 200:
        print(
            f"INCONCLUSIVE: only {windows} probe windows in the log -- was it "
            "captured with --debug-flags=TLB,PseudoInst?"
        )
        sys.exit(2)

    secret_missed = SECRET_VALUE in missed
    others_missed = len(missed - {SECRET_VALUE})
    others_total = len(slots) - 1

    print(f"probe windows: {windows}")
    print(f"secret slot ({SECRET_VALUE}) probe missed: {secret_missed}")
    print(f"other slots whose probe missed: {others_missed}/{others_total}")

    # The secret is readable only if its slot stands out: its probe hits
    # while the field of innocent slots overwhelmingly misses.
    if not secret_missed and others_missed >= 0.9 * others_total:
        print(f"Attack succeeded. Secret correctly guessed as {SECRET_VALUE}")
        sys.exit(0)
    else:
        print(
            f"Attack failed. Could not determine the secret ({SECRET_VALUE})"
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
