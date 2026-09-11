# SafeSide under gem5 SE mode

This directory vendors the `demos/` and `benchmarks/` trees from
[google/safeside](https://github.com/google/safeside) (BSD-3-Clause /
GPL-2.0, see `LICENSE`/`LICENSE.GPL-2.0`), unmodified, so they can be built
as static x86-64 Linux binaries and run under this gem5 fork's O3 CPU in
syscall-emulation (SE) mode.

## What actually runs here

`build_demos.sh` builds four of SafeSide's demos — the ones that are
single-process, single-threaded, and need nothing beyond `rdtsc`,
`clflush`, `mfence`/`lfence`, and ordinary memory syscalls (all of which
gem5 SE mode supports):

- `spectre_v1_pht_sa` — classic Spectre v1: branch-predictor (PHT)
  bounds-check bypass, same address space.
- `spectre_v1_btb_sa` — Spectre v1 variant via indirect-call (BTB/vtable)
  mistraining, same address space.
- `spectre_v4` — speculative store bypass (stale pointer read races a
  store).
- `ret2spec_sa` — return-stack-buffer (RSB) overflow via recursion, same
  address space.

### Native (host CPU) results

All four were built and run natively as a sanity check before touching
gem5. `spectre_v1_pht_sa`, `spectre_v1_btb_sa`, and `spectre_v4` leak the
embedded secret string (`"It's a s3kr3t!!!"`) correctly. `ret2spec_sa`
doesn't converge on this particular (likely RSB-refill-mitigated) host CPU
— a known characteristic of that demo on patched modern hardware, not a
build problem.

### gem5 SE-mode results (`--cpu-type=DerivO3CPU --caches --l2cache`)

- **`spectre_v1_pht_sa`** — leaks the full secret correctly.
- **`ret2spec_sa`** — leaks the full secret correctly. Notably, this
  *succeeds* under gem5 where it failed to converge natively above:
  gem5's O3 CPU's return-address-stack predictor evidently doesn't
  implement the RSB-refill-style mitigation this host's real CPU has.
- **`spectre_v4`, `spectre_v1_btb_sa`** — run cleanly (no crash, no hang)
  but don't converge to a leak within a generous time budget (40+ minutes
  of simulated execution each). This was investigated, not just accepted:
  `system.cpu.iew.memOrderViolationEvents` confirms `spectre_v4`'s
  speculative store-bypass *is* firing (65 violations in a 90-second
  sample at the default `store_set_clear_period`); passing
  `-P 'system.cpu[0].store_set_clear_period=100'` raises that to 10503
  violations in the same window (gem5's store-set predictor normally
  re-learns the true dependency and stops speculating past it almost
  immediately), yet a 20-minute run with that override still didn't
  converge. Likewise, `system.cpu.branchPred.corrected_0::CallIndirect`
  confirms `spectre_v1_btb_sa`'s indirect-call mistraining *is* causing
  real BTB mispredictions (565 in a 90-second sample). Both demos exercise
  the right microarchitectural primitive but something further downstream
  (how the resulting speculative access correlates with the cache-timing
  oracle, in gem5's specific timing model) prevents it from actually
  converging — this would need real profiling/debugging inside gem5 to
  pin down further, not just a parameter tweak.

## What's excluded, and why

SafeSide has ~20 demos total; the rest need OS/hardware features gem5 SE
mode doesn't provide, or are for other ISAs:

- **`meltdown`** — requires loading an out-of-tree Linux kernel module and
  reading `/sys/kernel/debug/...`. SE mode has no kernel to load a module
  into.
- **`l1tf`, `meltdown_ac`, `meltdown_de`** — recover from a real hardware
  fault (`SIGSEGV`/`SIGBUS`/`SIGFPE`) by mutating `ucontext_t`'s saved
  `RIP` from inside a signal handler and resuming execution past the fault.
  This needs faithful synchronous-signal/`ucontext` support, which SE mode
  doesn't provide.
- **`speculation_over_read_hw_breakpoint`, `speculation_over_exec_hw_breakpoint`,
  `speculation_over_single_step_trap`** — need `fork()` + `ptrace()`
  (`PTRACE_POKEUSER`/`GETREGS`/`SETREGS`) and x86 hardware debug registers,
  or `EFLAGS.TF` single-stepping. `ptrace` and debug registers aren't
  modeled in SE mode.
- **`spectre_v1_btb_ca`, `ret2spec_ca`** — cross-process variants of demos
  already covered by their same-address-space (`_sa`) counterparts above;
  need `fork()` + `sched_setaffinity` and rely on two live OS processes
  actually aliasing the same physical core's BTB/RSB state, which doesn't
  map cleanly onto SE mode's process model anyway.
- **`meltdown_br`, `meltdown_ss`, `meltdown_of`** — i686-only (32-bit
  `BOUND`/segment-limit/`INTO` faults); this fork targets X86-64.
- **`meltdown_ud`, `eret_hvc_smc_wrapper`, `speculation_over_syscall`,
  `speculation_over_sw_breakpoint`** — aarch64-only.
- **`benchmarks/spectre_v1/microbenchmark.cc`** — not excluded for
  feasibility reasons (it's single-threaded, no signals), just out of scope
  for this initial import since it's a bandwidth benchmark rather than a
  discrete leak-the-secret demo. Worth building the same way as the four
  above if you want it.

## Usage

```
./configs/vulcan/safeside/build_demos.sh          # builds into ./bin/
./configs/vulcan/run_safeside.sh                  # runs all four under gem5.opt
```

`run_safeside.sh` accepts the same `GEM5`/`CONFIG`/`OUTDIR` environment
overrides as `run_spectre_dolma.sh`, plus `DOLMA_ARGS` to pass e.g.
`--dolma-mode=1` through to test these demos against this branch's DOLMA
port instead of the undefended baseline.
