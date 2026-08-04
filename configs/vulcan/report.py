#!/usr/bin/env python3
# Copyright (c) 2026 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer; redistributions in binary
# form must reproduce the above copyright notice, this list of conditions and
# the following disclaimer in the documentation and/or other materials provided
# with the distribution; neither the name of the copyright holders nor the
# names of its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DAMAGES ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE.

"""Compute the reuse-attack success_rate from a decay_reuse.py stats file.

Each 'nd-probe' stat block corresponds to one trial's attacker probe of the
victim's secret line S. system.cache.overallHits::total == 1 means the probe
hit -- i.e. S was still resident, so the attacker recovered the victim's
access. gem5 omits zero-valued stats, so an absent overallHits line counts as a
miss (S had decayed away).

    success_rate = (# probe blocks that hit) / (# probe blocks)

A NORMAL cache leaves S resident every trial -> success_rate ~ 1.0. Cache Decay
randomly self-invalidates S between the victim install and the probe, so
success_rate collapses toward chance.

Usage:
    report.py LABEL=stats.txt [LABEL2=stats.txt ...]
"""

import sys

BEGIN = "Begin Simulation Statistics"
HIT = "system.cache.overallHits::total"


def probe_success(path):
    hits = 0
    total = 0
    in_probe = False
    block_hit = 0
    with open(path) as f:
        for line in f:
            if BEGIN in line:
                # Close out the previous probe block.
                if in_probe:
                    hits += block_hit
                    total += 1
                in_probe = "nd-probe" in line
                block_hit = 0
            elif in_probe and line.startswith(HIT):
                block_hit = 1 if int(line.split()[1]) >= 1 else 0
    if in_probe:  # final block had no trailing BEGIN
        hits += block_hit
        total += 1
    return hits, total


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1

    rows = []
    for arg in argv[1:]:
        label, _, path = arg.partition("=")
        hits, total = probe_success(path)
        rate = (hits / total) if total else float("nan")
        rows.append((label, hits, total, rate))

    width = max(len(r[0]) for r in rows)
    print("%-*s  %8s  %s" % (width, "config", "recovered", "success_rate"))
    print("-" * (width + 30))
    for label, hits, total, rate in rows:
        print(
            "%-*s  %4d/%-4d  success_rate = %d/%d = %.4f"
            % (width, label, hits, total, hits, total, rate)
        )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
