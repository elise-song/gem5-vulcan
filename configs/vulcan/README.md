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

1. delivers the demanded line's data to the core but **does not cache it**
   (no-allocate), and
2. fetches and caches **one random neighbour line** drawn from within
   `± random_fill_window` cache lines of the demanded address (clamped to the
   containing protected range, so the address is always legal and cacheable).

This de-correlates the post-access cache state from the victim's actual access.

Enable it on any classic cache with:

```python
cache.random_fill_ranges = [AddrRange(secret_lo, secret_hi)]
cache.random_fill_window = 8        # neighbourhood radius, in cache lines
```

An empty `random_fill_ranges` (the default) disables the defense entirely, so
existing configurations are unaffected.

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

- **Normal cache:** the victim access caches S, so every probe hits →
  `success_rate ≈ 1.0`.
- **Random Fill:** the victim access does not cache S (a random neighbour is
  cached instead), so every probe misses → `success_rate ≈ 0.0`.

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

Expected:

```
config        recovered  success_rate
normal-cache   200/200   success_rate = 200/200 = 1.0000
random-fill      0/200   success_rate = 0/200 = 0.0000
```

## Directed invariant check

```bash
build/X86/gem5.opt --outdir=m5out/directed configs/vulcan/random_fill_reuse.py \
    --defense on --directed
python3 configs/vulcan/directed_check.py m5out/directed/stats.txt
```

For a single protected access this asserts, from the stats, that: the demand is
observed as a protected miss, forced no-allocate, and injects exactly one random
fill (`rfProtectedMisses == rfDemandNoAllocate == rfRandomFillsInjected == 1`);
a scan of the window finds **exactly one resident line** (the random fill); and
a probe of S **misses** — S was left non-resident.

The pure neighbour-selection logic (block-aligned, in-region, in-window, never
the demanded line) is additionally covered by the gtest at
`src/mem/cache/random_fill.test.cc`
(`build/X86/mem/cache/random_fill.test.opt`).

## Files

- `random_fill_reuse.py` — reuse-attack harness (both `--defense off|on`, and
  `--directed` for the single-access invariant run).
- `report.py` — parses `rf-probe` stat blocks into a `success_rate`.
- `directed_check.py` — asserts the directed invariant from a `--directed` run.
- `run.sh` — runs the gtest, the before/after sweep, and the directed check.
