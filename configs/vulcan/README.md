# ScatterCache defense — cross-domain Prime+Probe evaluation

This directory holds the evaluation oracle for the **ScatterCache** secure-cache
defense (Werner et al., USENIX Security 2019) implemented in gem5's classic
cache as the `ScatterAssociative` indexing policy.

ScatterCache is a skewed-associative cache whose per-way set index is a *keyed*
function of the address, with the key selected by the **security domain**
(`req->contextId()`). Distinct domains get distinct keys, so the same address
maps to a different, unpredictable set in every way for every domain. Two
domains therefore almost never share a congruence class, which makes
cross-domain eviction-set construction infeasible and defeats eviction-based
attacks such as Prime+Probe.

## Files

- `prime_probe.py` — builds a single classic cache shared by an attacker
  (domain 0) and a victim (domain 1) and runs a per-secret Prime+Probe:
  *prime* the attacker's eviction set, let the *victim* touch the secret,
  then *probe*. The active security domain is switched per phase via the
  `_m5.sim.scatterSetDomain` hook. Selectable indexing policy:
  `baseline` (`TaggedSetAssociative`) vs `scatter` (`ScatterAssociative`).
- `report.py` — the oracle: computes `success_rate`, the fraction of secrets
  the attacker detected via a probe miss.
- `run.sh` — runs the identical attack against both caches and prints the
  before/after `success_rate`.

## Running

```
scons build/X86/gem5.opt -j$(nproc)
configs/vulcan/run.sh 1 4KiB 128 42
```

Representative result:

```
  baseline (plain set-associative) : success_rate = 128/128 = 1.0000
  scatter  (distinct domain keys)  : success_rate = 1/128   = 0.0078
```

The baseline channel is wide open (the attacker predicts the secret's set with
the conventional mapping and detects every access). Under ScatterCache the
attacker can no longer reproduce the victim's key-dependent mapping, so the
channel collapses to the ~`1/num_sets` noise floor. The same collapse holds for
higher associativities (e.g. `configs/vulcan/run.sh 4 8KiB 64` → `1.0` vs `0.0`).

## Unit test

The pure keyed-mapping logic is unit-tested independently of the simulator:

```
scons build/X86/mem/cache/tags/indexing_policies/scatter_hash.test.opt
./build/X86/mem/cache/tags/indexing_policies/scatter_hash.test.opt
```
