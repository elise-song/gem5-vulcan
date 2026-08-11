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
`random_fill_reuse.py --defense on --directed`.

The random-fill draw for the victim's protected demand miss (S) is one of
(2*window + 1) equally likely lines *including S itself* (Liu & Lee, Eq. 6)
-- so there are two equally legitimate outcomes, and this script asserts
whichever one actually happened is internally consistent, rather than
hard-coding one of them:

  * the draw picked a DIFFERENT line than S (rfDemandNoAllocate == 1): S's
    own fill is forced no-allocate, exactly one random fill is injected
    (rfRandomFillsInjected == 1), and the probe of S MISSES; or
  * the draw picked S itself (rfDemandSelfAllocate == 1): S's own fill is
    allowed to allocate normally, no separate fill is injected
    (rfRandomFillsInjected == 0), and the probe of S HITS.

In both cases, rfProtectedMisses == 1 and a scan of the window finds exactly
one resident line (the draw's target, whichever line that was).

An earlier, less faithful version of this defense excluded S from the draw
entirely, which made "probe of S misses" a hard invariant -- but only by
also making S's residency statistically distinguishable from any other
in-window line's, reopening the collision-timing channel the defense is
meant to close (see CollisionTimingSignalVanishesAtFullWindow in
src/mem/cache/random_fill.test.cc, and collision_attack.py). Asserting a
fixed outcome here would silently reintroduce that regression.

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

    no_allocate = victim.get("system.cache.rfDemandNoAllocate", 0)
    self_allocate = victim.get("system.cache.rfDemandSelfAllocate", 0)
    injected = victim.get("system.cache.rfRandomFillsInjected", 0)
    probe_hit = (probe.get("system.cache.overallHits::total", 0) == 1
                 and probe.get("system.cache.overallMisses::total", 0) == 0)
    probe_miss = (probe.get("system.cache.overallHits::total", 0) == 0
                  and probe.get("system.cache.overallMisses::total", 0) == 1)

    checks = [
        ("victim protected miss observed",
         victim.get("system.cache.rfProtectedMisses", 0) == 1),
        ("exactly one demand-fill outcome recorded "
         "(no-allocate xor self-allocate)",
         (no_allocate, self_allocate) in ((1, 0), (0, 1))),
        ("exactly one window line resident (the draw's target)",
         window.get("system.cache.overallHits::total", 0) == 1),
    ]
    if no_allocate:
        checks += [
            ("draw picked a different line: exactly one random fill "
             "injected", injected == 1),
            ("draw picked a different line: probe of S misses",
             probe_miss),
        ]
    else:
        checks += [
            ("draw picked S itself: no separate random fill injected",
             injected == 0),
            ("draw picked S itself: probe of S hits", probe_hit),
        ]

    ok = True
    for name, passed in checks:
        print("  [%s] %s" % ("PASS" if passed else "FAIL", name))
        ok = ok and passed

    print("DIRECTED CHECK: %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
