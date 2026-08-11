# Random Fill cache — reuse-attack evaluation

Evaluation harness for the **Random Fill** secure cache (Liu & Lee, *Random
Fill Cache Architecture*, MICRO 2014) implemented in gem5's classic cache
(`src/mem/cache/`).

## What Random Fill does

Random Fill targets **reuse-based** attacks, where the victim's
secret-dependent access deterministically pulls a predictable line into the
cache and the attacker later detects that line's residency. On a *protected*
demand read (an access whose block address falls in a configured
`random_fill_ranges` region) the cache:

1. draws **one target line** uniformly from within
   `[demand - random_fill_window_before, demand + random_fill_window_after]`
   cache lines of the demanded address -- this draw includes the demanded
   line itself as one of the equally likely outcomes (Liu & Lee, Eq. 6), and
2. usually the draw differs from the demand: that different line is fetched
   and cached instead, and the demanded line's own fill is forced
   **no-allocate**. Occasionally the draw coincides with the demand itself,
   in which case its own fill is simply allowed to allocate normally, like an
   ordinary demand fetch.

This de-correlates the post-access cache state from the victim's actual
access: an attacker who checks whether a given line is resident afterwards
cannot tell "the victim reused this exact line" apart from "the victim
touched a different, unrelated in-window line" -- both are equally likely
outcomes of the same draw. Deliberately *excluding* the demanded line from
the draw would break this: the demanded line's residency probability would
drop to exactly 0 while every other line's stays positive, reopening a
distinguishable (and measurable) timing signal -- see
`CollisionTimingSignalVanishesAtFullWindow` in `random_fill.test.cc`.

Enable it on any classic cache with:

```python
cache.random_fill_ranges = [AddrRange(secret_lo, secret_hi)]
cache.random_fill_window_before = 8   # window radius before the demand (a)
cache.random_fill_window_after = 8    # window radius after the demand (b)
```

An empty `random_fill_ranges` (the default) disables the defense entirely, so
existing configurations are unaffected.

### Window shape: unclamped by design

The window's two radii mirror RR1/RR2 in the paper's Fig. 4 -- independently
sized, unlike a single `±` value. There is deliberately no knob to clamp the
window to the containing `random_fill_ranges` entry: real RR1/RR2 hardware
is just an adder and a mask, with no register that knows where a *table*
(as opposed to the register value) ends, so a software-only "clamp to the
table" step has no synthesizable hardware equivalent and isn't offered as a
choice. A candidate near a table edge can therefore legally land outside
every configured protected range, into whatever memory is physically
adjacent -- reproducing the paper's own "boundary effect" (Section V-B,
Fig. 5) rather than hiding it. `WindowSpillsPastRegionEdge` in
`random_fill.test.cc` demonstrates this directly: a demand at a table's
start draws candidates before it.

Callers are responsible for ensuring the memory surrounding each protected
range is itself legal, cacheable, and mapped out to at least
`random_fill_window_before`/`_after` lines past each edge -- e.g. by sizing
the backing memory well beyond the protected table, as `collision_attack.py`
and `random_fill_reuse.py` both do.

One caveat about what the boundary effect *doesn't* change:
`collision_attack.py`'s P1/P2 gap (below) stays ~0 even with the demand line
at a table edge -- a uniform draw gives every candidate the same
`1/(window_before + window_after + 1)` probability regardless of whether
some of those candidates happen to fall outside the table, so two *specific*
addresses are still equally likely to be the one cached. The boundary effect
is a *positional* leak (whether the fill could be "observed" landing
off-table at all, which can partially narrow down where in the table the
true access was), not a same-vs-different-address timing leak -- which is
why it shows up in the gtest above rather than in this benchmark's metric.

## The reuse attack (evaluation oracle)

`random_fill_reuse.py` drives a `PyTrafficGen -> classic cache -> memory`
system through three phases per trial:

| phase  | what it does                                                        |
|--------|---------------------------------------------------------------------|
| flush  | stream a buffer far larger than the cache, evicting the secret S     |
| victim | one read to the secret line S (a *protected* demand when defense on) |
| probe  | one read to S; a **hit** means the attacker recovered S's residency  |

Stats are reset immediately before the probe, so each dumped `rf-probe` block
reflects the probe alone: `system.cache.overallHits::total == 1` iff the probe
hit. The `success_rate` is the fraction of trials the probe hit.

- **Normal cache:** the victim access always caches S, so every probe hits →
  `success_rate ≈ 1.0`.
- **Random Fill:** the victim access caches S only when the random-fill draw
  happens to coincide with S itself, so `success_rate ≈ 1 / (2 * window + 1)`
  (the region here is far larger than the window, so the draw essentially
  never runs out of room) -- e.g. `≈ 0.059` at the default `window=8`, not
  exactly `0.0`. This matches the paper's own model (Eq. 6): the defense's
  guarantee is that this residual rate is *the same* whether or not the
  probed line is the one the victim actually touched, not that it is zero.
  See `collision_attack.py` below for a benchmark that measures that
  equality directly instead of just this single-address recovery rate.

## Running

From the gem5 repo root:

```bash
./configs/vulcan/run.sh              # gtest + before/after sweep + directed check
```

or manually:

```bash
build/X86/gem5.opt --outdir=m5out/off configs/vulcan/random_fill_reuse.py \
    --defense off --trials 200
build/X86/gem5.opt --outdir=m5out/on  configs/vulcan/random_fill_reuse.py \
    --defense on  --trials 200 --window 8
python3 configs/vulcan/report.py \
    normal-cache=m5out/off/stats.txt random-fill=m5out/on/stats.txt
```

Expected (the random-fill row's exact count will vary trial-to-trial with
`--seed`, but should land near `success_rate ≈ 1/17 ≈ 0.059` at the default
`window=8`):

```
config        recovered  success_rate
normal-cache   200/200   success_rate = 200/200 = 1.0000
random-fill      9/200   success_rate = 9/200 = 0.0450
```

## Directed invariant check

```bash
build/X86/gem5.opt --outdir=m5out/directed configs/vulcan/random_fill_reuse.py \
    --defense on --directed
python3 configs/vulcan/directed_check.py m5out/directed/stats.txt
```

For a single protected access this asserts, from the stats, that the demand is
observed as exactly one protected miss with exactly one of
`rfDemandNoAllocate` / `rfDemandSelfAllocate` set (the draw either picked a
different line or picked the demand itself -- both are valid outcomes, see
above); that a scan of the window finds **exactly one resident line** (the
draw's target) either way; and that the probe of S is consistent with which
outcome occurred (misses if the draw picked a different line and S's own
fill was no-allocated; hits if the draw picked S itself and its fill was
allowed to allocate). Because either outcome is legitimate, this check does
**not** assert that S is always left non-resident -- that was true of an
earlier, less faithful version of this defense that excluded the demanded
line from the draw, which is exactly what
`CollisionTimingSignalVanishesAtFullWindow` in `random_fill.test.cc` (and
`collision_attack.py` below) are designed to catch.

The pure random-fill draw logic (block-aligned, in-window, and statistically
indistinguishable between landing on the demanded line versus any other
in-window line) is covered by the gtest at
`src/mem/cache/random_fill.test.cc`
(`build/X86/mem/cache/random_fill.test.opt`).

## Collision-timing-attack benchmark

`random_fill_reuse.py` only recovers one *known* address, so it cannot tell
"the demanded line is never cached" apart from "the demanded line is cached
with the paper-accurate probability" -- both give a small, nonzero
`success_rate`. `collision_attack.py` measures the quantity the security
proof is actually about (Eq. 4/6): the gap between

- **P1**: hit rate when the probe targets the *same* line as the preceding
  protected demand (a "collision"), and
- **P2**: hit rate when the probe targets a *different*, fixed, in-window
  line (a "no collision"),

for a small (default 16-line, matching the paper's own AES-table case study)
protected table. Run it with:

```bash
build/X86/gem5.opt --outdir=m5out/collision configs/vulcan/collision_attack.py \
    --window 16 --trials 800
python3 configs/vulcan/collision_report.py m5out/collision/stats.txt
```

With `--window 16` on each side, the candidate count is always
`2*16 + 1 = 33` (never clamped to the 16-line table -- see "Window shape"
above), so both P1 and P2 should land near `1/33 ≈ 0.0303`, and the check
passes:

```
P1 (collision, probe reused the demand line)     = 15/400 = 0.0375
P2 (no collision, probe hit a different in-window line) = 9/400 = 0.0225
gap = P1 - P2 = +0.0150  (SE = 0.0121, |gap|/SE = 1.24)
COLLISION-SIGNAL CHECK: PASSED (want |gap|/SE <= 3.0)
```

Against a build that excludes the demanded line from the random-fill draw,
this instead reports `P1 = 0.0000`, `P2 ≈ 1/(2*window) ≈ 0.0275`, and
`|gap|/SE ≈ 3.4` -- a `FAILED` verdict, since P1 is then a hard 0 regardless
of window size while P2 stays positive. That is exactly the regression this
benchmark (and the faster, pure-function gtest
`CollisionTimingSignalVanishesAtFullWindow`) exists to catch.

Note: the probe address is itself inside the protected region, so a probe
*miss* is also a fresh protected demand that draws (and, on a differing
draw, injects) its own random fill. `collision_attack.py` settles with an
idle period after the probe for exactly this reason -- without it, that
injection can still be draining when the next trial's flush starts and leak
a spurious resident line into a later trial's measurement.

`--window-before`/`--window-after` (default to `--window`) expose the
corresponding cache params, and `--demand-line` moves the attack's fixed
address (e.g. `--demand-line 0` puts it at a table edge) -- see "Window
shape" above for what that does and doesn't change: this benchmark's P1/P2
gap stays ~0 regardless of where the demand line sits.

## Files

- `random_fill_reuse.py` — single-address Flush-Reload harness (both
  `--defense off|on`, and `--directed` for the single-access invariant run).
- `report.py` — parses `rf-probe` stat blocks into a `success_rate`.
- `directed_check.py` — asserts the directed invariant from a `--directed` run.
- `collision_attack.py` — cache-collision timing-attack benchmark; measures
  P1 vs P2 (Eq. 4/6) through the real cache timing model.
- `collision_report.py` — parses `rf-collision-probe` /
  `rf-nocollision-probe` blocks into P1, P2, and a pass/fail gap check.
- `run.sh` — runs the gtest, the before/after sweep, the directed check, and
  the collision-timing-attack benchmark.
