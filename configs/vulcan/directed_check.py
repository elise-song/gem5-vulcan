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

"""Assert the Random Fill core invariant from a --directed stats file.

Runs the checks the task asks for on the single trial produced by
`random_fill_reuse.py --defense on --directed`:

  * the victim's protected demand read is a miss that is forced no-allocate
    (rfDemandNoAllocate == 1) and injects exactly one random fill
    (rfRandomFillsInjected == 1, rfProtectedMisses == 1); and
  * the attacker's probe of the demanded line S is a MISS -- S was left
    non-resident (rf-probe block has overallHits == 0, overallMisses == 1).

Exit status is non-zero if any assertion fails.

Usage:  directed_check.py stats.txt
"""

import sys

BEGIN = "Begin Simulation Statistics"


def parse_blocks(path):
    """Yield (label, {stat: int}) for each stat block in the file."""
    label = None
    stats = {}
    have = False
    with open(path) as f:
        for line in f:
            if BEGIN in line:
                if have:
                    yield label, stats
                have = True
                stats = {}
                # header form: "--- Begin Simulation Statistics : rf-probe ---"
                label = line.split(":", 1)[1].strip().strip("- ") \
                    if ":" in line else ""
            elif have and line and not line.startswith("-"):
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        stats[parts[0]] = int(parts[1])
                    except ValueError:
                        pass
    if have:
        yield label, stats


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2

    blocks = list(parse_blocks(argv[1]))
    victim = next((s for lbl, s in blocks if lbl == "rf-victim"), None)
    window = next((s for lbl, s in blocks if lbl == "rf-window"), None)
    probe = next((s for lbl, s in blocks if lbl == "rf-probe"), None)

    if victim is None or window is None or probe is None:
        print("FAIL: could not find rf-victim/rf-window/rf-probe blocks "
              "(run random_fill_reuse.py --defense on --directed)")
        return 2

    checks = [
        ("victim protected miss observed",
         victim.get("system.cache.rfProtectedMisses", 0) == 1),
        ("victim demand forced no-allocate",
         victim.get("system.cache.rfDemandNoAllocate", 0) == 1),
        ("exactly one random fill injected",
         victim.get("system.cache.rfRandomFillsInjected", 0) == 1),
        ("exactly one window line resident (the random fill)",
         window.get("system.cache.overallHits::total", 0) == 1),
        ("demanded line S left NON-resident (probe misses)",
         probe.get("system.cache.overallHits::total", 0) == 0
         and probe.get("system.cache.overallMisses::total", 0) == 1),
    ]

    ok = True
    for name, passed in checks:
        print("  [%s] %s" % ("PASS" if passed else "FAIL", name))
        ok = ok and passed

    print("DIRECTED CHECK: %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
