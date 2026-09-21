#!/usr/bin/env python3
# Determines whether control_mem_dtlb_store leaked its secret, from a gem5
# stdout log captured with --debug-flags=TLB.
#
# This is a from-scratch reimplementation of the DOLMA paper's own
# scripts/parse_tlb_logs.py (github.com/efeslab/dolma/attacks), adapted
# because that script relies on toggling the TLB debug flag on/off at
# specific points via m5_work_begin() exit-event hooks in a custom SE-mode
# config script, which this repo's harness does not replicate. Instead of
# phase-separating the log, this script uses a signal that doesn't need
# phase separation at all:
#
#   - The secret-indexed slot (array2[SECRET_VALUE*STEP_SIZE]) is the only
#     one that a restricted, speculative store ever touches -- every other
#     slot is only ever touched once, by its own single, genuine, real read.
#   - If the speculative store's translation is left uninstalled (DOLMA
#     working), EVERY one of its ~256 repeated speculative touches across
#     the program's outer loop is a genuine miss, so the secret slot's
#     total miss count is a clear outlier compared to every other slot's
#     count of exactly 1.
#   - If the speculative store's translation gets installed the first time
#     it's touched (unprotected), every later touch of that slot -- both
#     further speculative attempts and its own real read -- is a hit, so
#     the secret slot's total miss count is exactly 1, indistinguishable
#     from every other slot's count.
#
# So: secret slot miss count == 1  -> attack succeeded (leaked).
#     secret slot miss count  > 1  -> attack blocked.

import re
import sys
from argparse import ArgumentParser
from collections import Counter

SECRET_VALUE = 42


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


def collect_miss_counts(lines, slots):
    counts = Counter()
    for line in lines:
        if "Handling a TLB miss for address" not in line:
            continue
        m = re.search(r"address (0x[0-9a-fA-F]+)", line)
        if m and m.group(1) in slots:
            counts[slots[m.group(1)]] += 1
    return counts


def main():
    parser = ArgumentParser(
        description="Parse control_mem_dtlb_store's gem5 log"
    )
    parser.add_argument(
        "log", help="gem5 stdout log, captured with --debug-flags=TLB"
    )
    args = parser.parse_args()

    with open(args.log) as f:
        lines = f.readlines()

    slots = get_slot_addresses(lines)
    if len(slots) < 200:
        print(
            f"WARNING: only found {len(slots)}/256 slot addresses in the log; "
            "did the program run to completion?",
            file=sys.stderr,
        )

    counts = collect_miss_counts(lines, slots)
    secret_count = counts.get(SECRET_VALUE, 0)
    other_counts = [c for idx, c in counts.items() if idx != SECRET_VALUE]
    other_max = max(other_counts) if other_counts else 0

    print(f"secret slot ({SECRET_VALUE}) miss count: {secret_count}")
    print(f"other slots: max miss count: {other_max}")

    if secret_count <= 1 and secret_count <= other_max + 1:
        print(f"Attack succeeded. Secret correctly guessed as {SECRET_VALUE}")
        sys.exit(0)
    else:
        print(
            f"Attack failed. Could not determine the secret ({SECRET_VALUE})"
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
