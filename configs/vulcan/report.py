#!/usr/bin/env python3
# Copyright (c) 2024 The Regents of the gem5-vulcan study.
# SPDX-License-Identifier: BSD-3-Clause
#
# Evaluation oracle for the ScatterCache Prime+Probe experiment.
#
# It reads the combined stdout/Cache-debug log produced by prime_probe.py and
# computes the attacker's success_rate: the fraction of victim secrets whose
# access the attacker detected via a probe miss.
#
# A secret is "detected" if, during that secret's PROBE phase, at least one of
# the attacker's primed lines missed (i.e. the victim evicted it). Because the
# attacker's eviction set is built from the CONVENTIONAL address->set mapping,
# this is exactly the classic Prime+Probe oracle.
#
# Usage: report.py <debug_log> [out_file]

import sys


def analyze(path):
    total = 0
    detected = 0
    per_secret = []

    in_probe = False
    cur_secret = None
    cur_set = None
    cur_miss = False

    def close_probe():
        nonlocal total, detected
        if cur_secret is None:
            return
        total += 1
        if cur_miss:
            detected += 1
        per_secret.append((cur_secret, cur_set, cur_miss))

    with open(path, "r", errors="ignore") as f:
        for line in f:
            if ">>> PHASE PROBE" in line:
                # Starting a new probe window; close the previous one.
                if in_probe:
                    close_probe()
                in_probe = True
                cur_miss = False
                cur_secret = _kv(line, "secret=")
                cur_set = _kv(line, "set=")
                continue

            if line.startswith(">>>"):
                # Any other phase marker ends the current probe window.
                if in_probe:
                    close_probe()
                in_probe = False
                cur_secret = None
                continue

            if in_probe and "access for" in line and "miss" in line:
                cur_miss = True

    if in_probe:
        close_probe()

    return total, detected, per_secret


def _kv(line, key):
    if key not in line:
        return None
    tail = line.split(key, 1)[1]
    return tail.split()[0]


def main():
    path = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else None

    total, detected, per_secret = analyze(path)
    rate = (detected / total) if total else 0.0

    lines = []
    for secret, tset, miss in per_secret:
        status = "DETECTED" if miss else "hidden"
        lines.append(f"secret={secret} conv_set={tset} -> {status}")
    lines.append("")
    lines.append(f"success_rate = {detected}/{total} = {rate:.4f}")
    text = "\n".join(lines)

    print(text)
    if out:
        with open(out, "w") as w:
            w.write(text + "\n")

    return rate


if __name__ == "__main__":
    main()
