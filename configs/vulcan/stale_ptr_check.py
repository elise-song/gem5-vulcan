#!/usr/bin/env python3
"""Detect the Cache Decay stale-pointer / TOCTOU bug from a stats file.

Run stale_ptr_reuse.py first to produce the stats:

    build/X86/gem5.opt --outdir=OUT configs/vulcan/stale_ptr_reuse.py
    python3 configs/vulcan/stale_ptr_check.py OUT/stats.txt

Reads the nd-touch / nd-probe stat blocks it dumps:

  * nd-touch validates the setup: B was legitimately resident and the touch
    was a genuine hit that rescheduled (renewed) its decay deadline
    (ndDecayReschedules >= 1, overallHits::total >= 1).
  * nd-probe is the actual check: with the correct mechanism, B's renewed
    deadline protects it and the probe HITS. With the stale-pointer bug, the
    pending-retry path never re-validates decayDeadline before evicting, so
    B gets wrongly decayed anyway and the probe MISSES despite the renewal.

Exit status is non-zero (and prints "VULNERABLE") if the bug reproduces.
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
    touch = next((s for lbl, s in blocks if lbl == "nd-touch"), None)
    probe = next((s for lbl, s in blocks if lbl == "nd-probe"), None)

    if touch is None or probe is None:
        print("FAIL: could not find nd-touch/nd-probe blocks "
              "(run stale_ptr_reuse.py first)")
        return 2

    setup_ok = (
        touch.get("system.cache.overallHits::total", 0) >= 1
        and touch.get("system.cache.ndDecayReschedules", 0) >= 1
    )
    print("  [%s] setup: B was resident and legitimately touched/renewed"
          % ("PASS" if setup_ok else "FAIL"))
    if not setup_ok:
        print("FAIL: setup invalid, timing calibration is off -- adjust "
              "--evict-delay-ns/--touch-delay-ns")
        return 2

    probe_hit = probe.get("system.cache.overallHits::total", 0) >= 1
    probe_miss = probe.get("system.cache.overallMisses::total", 0) >= 1
    decayed_anyway = probe.get("system.cache.ndDecayInvalidations", 0) >= 1

    if probe_hit and not decayed_anyway:
        print("  [SAFE] B's renewed deadline protected it (probe hit)")
        print("RESULT: SAFE (stale-pointer bug did not reproduce)")
        return 0

    if probe_miss and decayed_anyway:
        print("  [VULNERABLE] B was decay-invalidated despite its "
              "just-renewed, still-future deadline (probe miss)")
        print("RESULT: VULNERABLE (stale-pointer bug reproduced)")
        return 1

    print("  [UNCLEAR] probe result doesn't match either expected pattern "
          "(hits=%s misses=%s ndDecayInvalidations=%s)"
          % (probe.get("system.cache.overallHits::total", 0),
             probe.get("system.cache.overallMisses::total", 0),
             probe.get("system.cache.ndDecayInvalidations", 0)))
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
