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

"""Assert the Cache Decay core invariants from a --directed stats file.

Runs the checks the task asks for on the deterministic phases produced by
`decay_reuse.py --defense on --directed`:

  * a line's decay event fires at ~its randomized deadline and self-invalidates
    the line (nd-decay: ndDecayInvalidations >= 1);
  * a DIRTY decayed line is written back before it is invalidated -- no silent
    data loss (nd-decay: ndDirtyDecayWritebacks >= 1);
  * once decayed, the line is gone, so an attacker probe of it MISSES
    (nd-probe: overallHits == 0, overallMisses >= 1); and
  * touching a resident line reschedules (re-draws) its decay deadline
    (nd-touch: ndDecayReschedules >= 1, and the touching access hit).

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
    decay = next((s for lbl, s in blocks if lbl == "nd-decay"), None)
    probe = next((s for lbl, s in blocks if lbl == "nd-probe"), None)
    touch = next((s for lbl, s in blocks if lbl == "nd-touch"), None)

    if decay is None or probe is None or touch is None:
        print("FAIL: could not find nd-decay/nd-probe/nd-touch blocks "
              "(run decay_reuse.py --defense on --directed)")
        return 2

    checks = [
        ("decay self-invalidated the line at its deadline",
         decay.get("system.cache.ndDecayInvalidations", 0) >= 1),
        ("dirty decayed line was written back before invalidation",
         decay.get("system.cache.ndDirtyDecayWritebacks", 0) >= 1),
        ("decayed line left NON-resident (probe misses)",
         probe.get("system.cache.overallHits::total", 0) == 0
         and probe.get("system.cache.overallMisses::total", 0) >= 1),
        ("touching a resident line rescheduled its decay deadline",
         touch.get("system.cache.ndDecayReschedules", 0) >= 1
         and touch.get("system.cache.overallHits::total", 0) >= 1),
    ]

    ok = True
    for name, passed in checks:
        print("  [%s] %s" % ("PASS" if passed else "FAIL", name))
        ok = ok and passed

    print("DIRECTED CHECK: %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
