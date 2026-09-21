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
  misses during a bracketed "recovery" window. This repo's harness doesn't
  replicate that hook. Instead, `parse_tlb_log.py` here is a **from-scratch
  reimplementation** of upstream's `scripts/parse_tlb_logs.py` using a
  signal that doesn't need phase separation at all -- see the comment at
  the top of that file for the reasoning. It was cross-checked against the
  same ground truth (baseline leaks, DOLMA-Conservative doesn't) before
  being trusted.
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
| `data_mem_dcache_load` | leak | **leak** | blocked | **leak** | blocked |
| `control_mem_dtlb_store` | leak | **leak** | **leak** | **leak** | **leak** |
| `control_mem_icache_branch` | leak | blocked | blocked | blocked | blocked |
| `control_mem_btb_branch` | doesn't converge | doesn't converge | doesn't converge | doesn't converge | doesn't converge |

`control_mem_dcache_load` and `control_mem_icache_branch` match the
paper's own reference results
([efeslab/dolma/attacks/README.md](https://github.com/efeslab/dolma/blob/master/attacks/README.md))
exactly. The bolded results are where this repo's DOLMA port diverges from
what the paper's own reference implementation reports for the same named
attack, found only by actually running these specific tests:

- **`data_mem_dcache_load` leaks under DOLMA-Default (modes 1 and 3), but
  is correctly blocked under DOLMA-Conservative (modes 2 and 4).** The
  paper's own results say all four defended modes should block this. In
  `InstructionQueue::wakeDependents()`
  ([src/cpu/o3/inst_queue.cc](../../../src/cpu/o3/inst_queue.cc)), a
  non-Conservative load is only promoted to `DataInducer` once
  `isPendingMemOrder()` is true -- i.e. only *after* a memory-order
  violation has already been detected and confirmed. The paper's own text
  (§4.1) describes a *proactive* restriction ("DOLMA-Default must
  prevent leakages stemming from any load-dependent micro-ops, until all
  prior stores resolve") that should apply to any load that issued ahead
  of an unresolved elder store, not only to ones later confirmed to have
  guessed wrong. Conservative mode sidesteps this entirely (it makes
  *every* load a `DataInducer` unconditionally), which is exactly why it
  still blocks this attack while Default doesn't. This looks like a real
  gap in DOLMA-Default's SSB handling, not a convergence artifact --
  unlike `control_mem_btb_branch`, this one converges cleanly and
  consistently across every mode; it just doesn't get blocked in Default.

- **`control_mem_dtlb_store` leaks in every mode, including
  DOLMA-Conservative.** This is more serious: the paper's own headline
  attack is completely unprotected here, in every configuration. The
  reason is upstream, in the very first DOLMA commit, not something this
  session's earlier TLB fix (commit `f850279`, "Suppress TLB installation
  on a restricted miss") touches:
  `DynInst::writeMem()` ([src/cpu/o3/dyn_inst.cc](../../../src/cpu/o3/dyn_inst.cc))
  is the entry point `LSQUnit::executeStore()` calls at *execute* time
  (via `initiateAcc()`) to translate a store's address -- before the store
  commits -- and it deliberately never sets `Request::RESTRICTED`,
  reasoning that stores don't touch the cache until they commit, by which
  point they're safe. That reasoning is correct for the *data write*, but
  the *address translation* (the TLB lookup/walk/install) happens at this
  same speculative execute-time call, regardless of whether the write
  itself ever commits. So a restricted store's translation is never tagged
  restricted at all, meaning neither this session's TLB fix nor any other
  DOLMA protection ever engages for it. This is, concretely, the same
  category of mistake the DOLMA paper itself criticizes STT for making
  (§3.3): assuming a store is safe because its *data* write is
  deferred, without separately accounting for its *address*.

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

## Usage

```
./configs/vulcan/dolma_attacks/build_attacks.sh
./configs/vulcan/dolma_attacks/run_attacks.sh                       # baseline
DOLMA_ARGS=--dolma-mode=2 ./configs/vulcan/dolma_attacks/run_attacks.sh   # DOLMA-Conservative (M+R)
```
