# Non-deterministic cache (Cache Decay) — reuse-attack evaluation

Evaluation harness for the **Non-deterministic cache via Cache Decay**
(Keramidas, Antonopoulos, Serpanos & Kaxiras, *"Non deterministic caches: a
simple and effective defense against side channel attacks"*, DAES 2008)
implemented in gem5's classic cache (`src/mem/cache/`).

## What Cache Decay does

Contention/reuse timing attacks (Prime+Probe, Evict+Time, reuse) rely on the
cache being **deterministic**: a line the victim installs stays installed until
the attacker deliberately evicts it, so the attacker's later probe faithfully
reflects the victim's secret-dependent access.

Cache Decay destroys that determinism. When enabled, **every** cache line is
given a *randomized* decay lifetime when it is installed; when that lifetime
elapses the line **self-invalidates** (it is written back first if dirty, then
dropped). A line that is used again has its lifetime re-drawn (extended). Because
lifetimes are random and per-line, cache occupancy between the victim's access
and the attacker's probe is noisy — a primed or victim line may have randomly
vanished — so the attacker's measurement no longer reliably distinguishes the
secret.

Enable it on any classic cache with:

```python
cache.decay_enabled = True
cache.decay_interval = "500ns"   # base/mean per-line lifetime
cache.decay_range    = "250ns"   # uniform randomization half-width
```

`decay_enabled = False` (the default) disables the defense entirely, so existing
configurations are unaffected. Each line's lifetime is drawn uniformly from
`[decay_interval - decay_range, decay_interval + decay_range]` (clamped `>= 1`
tick); a non-zero `decay_range` is what makes the defense genuinely
non-deterministic.

### Mechanism (in the classic cache)

- A per-line `decayDeadline` tick lives on `CacheBlk`, drawn from the
  params-provided RNG (`rng->random<Tick>(lo, hi)`, the same engine idiom as
  `RandomRP`) on fill, and re-drawn on a hit/touch.
- A single event (`decayEvent`) drives all decays: it is always scheduled for
  the earliest live deadline, and on firing it scans the live tag array (by
  address, never a stale block pointer) and self-invalidates every line whose
  deadline has elapsed, then reschedules itself for the next-earliest deadline.
- Correctness guards: a line with an outstanding MSHR / write-buffer entry, or
  whose fill has not yet landed, is **deferred** (never dropped mid-flight); a
  **dirty** line is **written back** before it is invalidated (no silent data
  loss); invalidation reuses the cache's normal `evictBlock()` eviction path so
  coherence stays correct. Stats: `ndDecayInvalidations`,
  `ndDirtyDecayWritebacks`, `ndDecayReschedules`.

## The reuse attack (evaluation oracle)

`decay_reuse.py` drives a `PyTrafficGen -> classic cache -> memory` system
through phases per trial:

| phase  | what it does                                                        |
|--------|---------------------------------------------------------------------|
| flush  | stream a buffer far larger than the cache, evicting the secret S     |
| victim | one read to the secret line S (installs S in the cache)             |
| gap    | idle for a fixed interval, during which S may randomly decay        |
| probe  | one read to S; a **hit** means the attacker recovered S's residency  |

Stats are reset immediately before the probe, so each dumped `nd-probe` block
reflects the probe alone: `system.cache.overallHits::total == 1` iff the probe
hit. The `success_rate` is the fraction of trials the probe hit.

- **Normal cache:** the victim access installs S and it never decays, so every
  probe hits → `success_rate ≈ 1.0` (a perfect, deterministic channel).
- **Cache Decay:** the gap is chosen to straddle the randomized lifetime
  distribution, so S survives to the probe only about half the time →
  `success_rate` collapses toward chance (~0.5): the attacker can no longer
  distinguish the victim's access from noise.

## Running

From the gem5 repo root:

```bash
./configs/vulcan/run.sh              # gtest + before/after sweep + directed check
```

or manually:

```bash
build/X86/gem5.opt --outdir=m5out/off configs/vulcan/decay_reuse.py \
    --defense off --trials 200
build/X86/gem5.opt --outdir=m5out/on  configs/vulcan/decay_reuse.py \
    --defense on  --trials 200 --gap-ns 1000 --interval-ns 950 --range-ns 900
python3 configs/vulcan/report.py \
    normal-cache=m5out/off/stats.txt cache-decay=m5out/on/stats.txt
```

Expected (representative):

```
config        recovered  success_rate
normal-cache   200/200   success_rate = 200/200 = 1.0000
cache-decay     ~90/200   success_rate ≈ 0.45   (≈ chance)
```

The 200 independent trials (each a different, randomly chosen secret line that
decays independently) are the sample over which the randomized behaviour is
observed; the reported `success_rate` is the mean over that sample.

## Directed invariant check

```bash
build/X86/gem5.opt --outdir=m5out/directed configs/vulcan/decay_reuse.py \
    --defense on --directed
python3 configs/vulcan/directed_check.py m5out/directed/stats.txt
```

Using a lifetime whose maximum is far shorter than a long idle (so decay is
guaranteed to fire), this asserts, from the stats, that:

- a line's decay event fires and self-invalidates it
  (`ndDecayInvalidations >= 1`);
- a **dirty** decayed line is **written back** before invalidation
  (`ndDirtyDecayWritebacks >= 1`);
- once decayed, a probe of the line **misses** (it was left non-resident); and
- **touching** a resident line **reschedules** its decay deadline
  (`ndDecayReschedules >= 1`, and the touching access was a hit).

The pure decay-lifetime arithmetic (uniform within the randomized window,
positive, deterministic when the range is 0, `deadline = now + lifetime`) is
additionally covered by the gtest at `src/mem/cache/cache_decay.test.cc`
(`build/X86/mem/cache/cache_decay.test.opt`).

## Files

- `decay_reuse.py` — reuse-attack harness (`--defense off|on`, `--directed`).
- `report.py` — parses `nd-probe` stat blocks into a `success_rate`.
- `directed_check.py` — asserts the decay invariants from a `--directed` run.
- `run.sh` — runs the gtest, the before/after sweep, and the directed check.
