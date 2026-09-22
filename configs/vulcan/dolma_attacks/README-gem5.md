# DOLMA's own attack suite under this repo's gem5 SE mode

This directory ports the DOLMA paper's own canonical attack suite
(`attacks/src/*.c` from [efeslab/dolma](https://github.com/efeslab/dolma),
the paper's reference implementation) to build and run against this
branch's gem5 fork and its `--dolma-mode` port, instead of just against the
narrower [SafeSide](../safeside/) subset or the single hand-written
`../spectre.c` PoC.

Per `attacks/README.md` upstream, these are exactly the six scenarios shown
in the paper's own Fig. 7 (naming convention
`specType_secretLocation_channel_uop.c`):

- `control_mem_dcache_load` -- control speculation, secret in memory,
  D-cache channel, load uop. (Equivalent to `../spectre.c`.)
- `control_mem_dtlb_store` -- control speculation, secret in memory,
  **D-TLB** channel, store uop. This is the paper's own novel discovery
  (Listing 3 / Fig. 1).
- `control_mem_btb_branch` -- control speculation, secret in memory,
  **BTB** channel, branch uop.
- `control_mem_icache_branch` -- control speculation, secret in memory,
  **I-cache** channel, branch uop.
- `control_reg_dcache_load` -- control speculation, secret already in a
  **register** (not loaded from memory during speculation), D-cache
  channel, load uop.
- `data_mem_dcache_load` -- **data speculation (SSB)**, secret in memory,
  D-cache channel, load uop.

## What changed from upstream

- **Threading/orchestration dropped.** Upstream's `main.py` /
  `se_run_experiment.py` drive a custom SE-mode config script and (for
  `control_mem_dtlb_store` only) toggle the TLB debug flag on and off at
  runtime via `m5_work_begin()` exit-event hooks, so its log only captures
  misses during a bracketed probe window. This repo's harness doesn't
  replicate that hook, so `parse_tlb_log.py` here is a **from-scratch
  reimplementation** of upstream's `scripts/parse_tlb_logs.py` that gets
  the same phase separation a different way: TLB logging stays on
  throughout, `--debug-flags` also carries `PseudoInst` so the attack's own
  `m5_work_begin(42/43)` markers land in the log, and the parser brackets
  the probe with those. The verdict is then the attacker's own signal --
  whether the secret-indexed slot's probe took a d-TLB miss while the
  other 255 slots' probes did. See the comment at the top of that file.

  An earlier version of this parser tried to sidestep needing the markers
  by counting each slot's *total* misses across the whole run. That
  heuristic is unsound and has been discarded: the training-path slot
  dominates the counts, and "blocked, because the speculative store never
  executed" is indistinguishable from "leaked" under it. It reported a
  leak in modes 1 and 3 that the phase-separated signal shows is not
  there.
- **No assembly-level patching.** Upstream's Makefile runs
  `scripts/{control_mem_dtlb_store,control_reg_dcache_load,data_mem_dcache_load}.py`
  against gcc-9.3.0-era generated assembly to force a specific
  instruction-level pattern. This machine has gcc 13.3.0, and those patches
  do exact string matching against 9.3.0's output -- they don't apply
  as-is, and reverse-engineering new patches per architecture/compiler
  combination didn't turn out to be necessary: all three attacks leak
  correctly under plain `-O0`, unpatched, in this simulator. Verified
  empirically (unpatched baseline runs, checked against the paper's own
  expected `SUCCESS` outcome) before being trusted, not just assumed.
- **`CACHE_HIT_THRESHOLD` recalibrated** from upstream's `80` to `100` in
  the three attacks that use a fixed cutoff
  (`control_mem_dcache_load`, `control_reg_dcache_load`,
  `control_mem_dtlb_store`'s unused-for-pass/fail print). Measured directly
  from this simulator's own output: cache hits land around 45-50 cycles,
  misses around 150, in this config -- `80` sat in a dead zone that missed
  real hits. `data_mem_dcache_load` and `control_mem_btb_branch` use a
  find-the-minimum comparison instead of a fixed threshold and needed no
  change.
- **`--mem-type=SimpleMemory`, matching upstream's own config** (see
  `attacks/main.py`), rather than this repo's other tests' default
  detailed DRAM timing model. These are single-trial attacks (upstream
  runs each attack exactly once per gem5 invocation, unlike `../spectre.c`'s
  up-to-999-tries retry loop), so they need a cleaner, lower-noise timing
  signal than the default memory model gives. `run_attacks.sh` also passes
  the explicit `--l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB
  --l1i_assoc=8 --l2_size=2MB --l2_assoc=16` upstream uses, matching the
  paper's own Table 1 configuration (this repo's `--caches --l2cache`
  defaults happen to already match, but passing them explicitly keeps this
  independent of that default ever changing).
- **`complex_btb`/`basic_btb` (upstream's per-attack toggle of
  `BranchPredictor.py`'s `useIndirect` param) dropped** -- this repo's
  branch predictor config has no such param.
- Ruby/`X86_MESI_Two_Level` dropped in favor of the classic memory system
  (`--caches --l2cache`) that every other DOLMA-mode test in this repo
  uses. This matters, not just for consistency: **DOLMA's cache-side
  protections (fill-suppression, no-LRU-update-on-hit) are only
  implemented for the classic memory system in this repo** --
  `grep -r isRestricted src/mem/ruby/` turns up nothing. Running these
  under Ruby, as upstream's `Makefile` does, would silently test an
  unprotected code path and misreport DOLMA as broken.

## Results

Run via `DOLMA_ARGS=--dolma-mode=N ./run_attacks.sh` for N in 0-4:

| Attack | mode 0 | mode 1 (Default M+R) | mode 2 (Conservative M+R) | mode 3 (Default M) | mode 4 (Conservative M) |
|---|---|---|---|---|---|
| `control_mem_dcache_load` | leak | blocked | blocked | blocked | blocked |
| `control_reg_dcache_load` | leak | blocked | blocked | blocked | blocked |
| `data_mem_dcache_load` | leak | blocked | blocked | blocked | blocked |
| `control_mem_dtlb_store` | leak | blocked | blocked | blocked | blocked |
| `control_mem_icache_branch` | leak | blocked | blocked | blocked | blocked |
| `control_mem_btb_branch` | doesn't converge | doesn't converge | doesn't converge | doesn't converge | doesn't converge |

Every attack that produces a usable signal now matches the paper's own
reference results
([efeslab/dolma/attacks/README.md](https://github.com/efeslab/dolma/blob/master/attacks/README.md)),
apart from the one fidelity gap noted last. Two of these rows needed a fix
to this repo's DOLMA port before they did, both found only by actually
running these tests:

- **`data_mem_dcache_load` originally leaked under DOLMA-Default (modes 1
  and 3)**, while being correctly blocked under DOLMA-Conservative. In
  `InstructionQueue::wakeDependents()`
  ([src/cpu/o3/inst_queue.cc](../../../src/cpu/o3/inst_queue.cc)), a
  non-Conservative load was only promoted to `DataInducer` once
  `isPendingMemOrder()` was true -- i.e. only *after* a memory-order
  violation had already been detected and confirmed. The paper's own text
  (§4.1) describes a *proactive* restriction ("DOLMA-Default must
  prevent leakages stemming from any load-dependent micro-ops, until all
  prior stores resolve") that applies to any load that issued ahead of an
  unresolved elder store, not only to ones later confirmed to have guessed
  wrong. Conservative mode sidestepped this entirely (it makes *every*
  load a `DataInducer` unconditionally), which is exactly why it still
  blocked this attack while Default didn't. Fixed in commit `aa39fadf`,
  which adds `ROB::hasUnresolvedElderStore()` and consults it at promotion
  time.

- **`control_mem_dtlb_store` originally leaked in every mode, including
  DOLMA-Conservative** -- the paper's own headline attack, unprotected in
  every configuration. The cause was upstream, in the very first DOLMA
  commit, not anything the TLB fix (commit `f850279`, "Suppress TLB
  installation on a restricted miss") touched:
  `DynInst::writeMem()` ([src/cpu/o3/dyn_inst.cc](../../../src/cpu/o3/dyn_inst.cc))
  is the entry point `LSQUnit::executeStore()` calls at *execute* time
  (via `initiateAcc()`) to translate a store's address -- before the store
  commits -- and it never set `Request::RESTRICTED`, on the reasoning that
  stores don't touch the cache until they commit, by which point they're
  safe. That reasoning holds for the *data write*, but the *address
  translation* (the TLB lookup/walk/install) happens at this same
  speculative execute-time call regardless of whether the write ever
  commits, so a restricted store's translation was never tagged restricted
  at all and no DOLMA protection ever engaged for it. This is concretely
  the same category of mistake the DOLMA paper itself criticizes STT for
  making (§3.3): assuming a store is safe because its *data* write is
  deferred, without separately accounting for its *address*. Fixed in
  commit `32ce960c`, which tags the translation and has
  `LSQUnit::writebackStores()` clear the flag again for the real,
  now-safe committed write.

- **`control_reg_dcache_load` is blocked in mem-only mode 3, where the
  paper's own reference says it should succeed.** Mem-only mode is
  supposed to relax protection specifically for register-resident
  secrets, regardless of which channel eventually transmits them; this
  repo's mode 3 protects it anyway. This is a side effect of this
  session's earlier mem-only-mode fix (commit `9b61b55`, "Fix DOLMA
  mem-only mode skipping control-restriction"), which
  correctly closes a real security hole for `control_mem_dcache_load` but,
  as a side effect, also removes the paper's intended M vs. M+R
  distinction for this specific scenario. Unlike the two findings above,
  this direction of drift makes the port *more* conservative than the
  paper's spec, not less -- a fidelity gap, not a new leak.

- **`control_mem_btb_branch` doesn't converge in any mode, including
  baseline (mode 0).** Investigated, not just accepted: the per-guess
  timing distribution shows almost every one of the 256 candidate targets
  landing at the same cycle count, with no signal at the secret index --
  consistent with this CPU config's BTB (4096 entries, per this repo's O3
  CPU defaults) being far larger than the 256 distinct targets this attack
  trains, so none of them ever evict each other. This matches the same
  class of non-convergence already documented for SafeSide's
  `spectre_v1_btb_sa` (see [../safeside/README-gem5.md](../safeside/README-gem5.md))
  and doesn't indicate anything about DOLMA's own correctness, since the
  underlying channel never produces a usable signal even without any
  defense enabled.

## A residual gap these six attacks do not catch

Passing all six is necessary, not sufficient. Instrumenting `ROB::doSquash()`
to count, over a whole `control_mem_dtlb_store` run, every memory access that
(a) was `ControlRestricted` when it entered the ROB, (b) executed anyway, and
(c) was then squashed -- i.e. every wrong-path access that DOLMA had tainted
and then untainted before it ran:

| mode | wrong-path mem refs tainted at insert | of which the access itself ran **unrestricted** |
|---|---|---|
| 1 (Default M+R) | 7,864,064 | 887,038 (11.3%) |
| 2 (Conservative M+R) | 12,707,844 | 894 (0.007%) |

DOLMA-Conservative is effectively airtight by this measure; DOLMA-Default
lets better than one in ten wrong-path accesses run with its restriction
already cleared. None of the six attacks exercises that gap -- they all
transmit through a chain that Default happens to still cover -- so this is
recorded here as a measured, open finding rather than a fixed one. The
1000x mode-1-vs-mode-2 ratio points at `ControlRestricted`'s clearing rule
rather than at any single transmitter: Conservative's blanket per-load
`DataInducer` tainting keeps these same instructions covered by
`DataRestricted` long after `ControlRestricted` has gone.

One contributor is fixed, and is worth naming because it is *not* the
dominant one: `IEW::executeInsts()` used to clear a branch's
`ControlInducer` the moment the branch executed, including when it
mispredicted -- even though every younger instruction was then wrong-path
and still several cycles away from being squashed. Holding the inducer
until `ROB::doSquash()` has actually drained the squash closes that window
at no measured cost (identical `simTicks` in mode 1, +0.0006% in mode 2),
but it only accounts for 2,140 of mode 1's 887,038 escapes. The rest have
a different cause, not yet identified.

## Usage

```
./configs/vulcan/dolma_attacks/build_attacks.sh
./configs/vulcan/dolma_attacks/run_attacks.sh                       # baseline
DOLMA_ARGS=--dolma-mode=2 ./configs/vulcan/dolma_attacks/run_attacks.sh   # DOLMA-Conservative (M+R)
```
