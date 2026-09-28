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
- **`spectre_v4`** — leaks correctly, but only after being ported off
  `CacheSideChannel` (see below). Budget generously: a byte costs roughly
  15 minutes of wall clock here.
- **`spectre_v1_btb_sa`** — still produces no signal, even with the same
  readout the three working demos use. See below.

### The `CacheSideChannel` readout does not work under gem5

`spectre_v4` and `spectre_v1_btb_sa` originally read their side channel
through `CacheSideChannel::RecomputeScores()`, unlike `spectre_v1_pht_sa`
and `ret2spec_sa`, which use `TimingArray`. That difference — not the
speculation, and not the cache model — is why the two never converged.

`RecomputeScores()` only scores a sample when *exactly one* oracle entry
reads as a cache hit, and discards the sample otherwise. Instrumenting it
(printing its own calibration and hit count every few calls) shows that
condition is never met under gem5:

    spectre_v4:        calls=1200 scored=0 avg_hitcount=16.00
                       median=201 safe_lat=55 hitmiss_diff=146 thresh=128
    spectre_v1_btb_sa: calls=1000 scored=0 avg_hitcount=21.00
                       median=202 safe_lat=55 hitmiss_diff=147 thresh=129

The calibration is healthy -- a hit reads 55 cycles, a miss 201, and the
threshold lands at 128, cleanly between them -- and all prefetchers are
`Null` in this configuration. But 16 (or 21) entries read fast on *every*
call, with no variance, so `hitcount` is never 1, **not one sample in over
a thousand is ever scored**, `scores_` never increments, and the
`> 2 * runner_up + 40` bar is unreachable. More runtime cannot help: two
100-minute runs (~2.35 billion simulated instructions each, versus the
12.2 million `spectre_v1_pht_sa` needs for the whole secret) left the
counters in exactly the state 200 runs did. The demos cannot converge, and
equally cannot reach their own 100,000-run "Does not converge" bound, which
is why they only ever appeared to hang.

Both are therefore ported to the `TimingArray` readout that upstream
already uses for `spectre_v1_pht_sa`, which has no such validity filter.
The change is mechanical (`FlushOracle()` becomes `FlushFromCache()`,
`ForceRead(oracle.data() + b)` becomes `ForceRead(&timing_array[b])`, and
the score accumulation becomes `FindFirstCachedElementIndexAfter()`), and
each file carries a comment recording why.

Outcome:

- **`spectre_v4` now works.** It recovers the first secret byte at
  baseline, where before the port it produced nothing at all. This is the
  suite's only working speculative-store-bypass test, and it confirms SSB
  genuinely occurs in this port -- something no other test demonstrated.
  Under DOLMA it leaks nothing, in both Default and Conservative, in a
  longer budget than the baseline needed.
- **`spectre_v1_btb_sa` still produces nothing**, now with a readout
  identical to three demos that work -- and the cause has since been
  pinned down in gem5 itself. `BPredUnit::updateBTB()` is called from
  exactly two places: `update()`, at commit, and `squash()`, for the
  branch that *resolved* as mispredicted. Wrong-path branches younger
  than it are discarded through `squashHistory()`, which never touches
  the BTB. A transient branch therefore cannot install a BTB entry, so a
  speculative indirect jump has nothing to transmit through. The
  mistraining does fire (565 `branchPred.corrected_0::CallIndirect` in a
  90-second sample); it simply cannot leave a trace.

  `../dolma_attacks/control_mem_btb_branch` fails identically, and its
  per-guess timings show the secret index sitting in the bulk with no
  signal at all. This is a property of gem5's branch predictor model, not
  of either attack or of DOLMA: the baseline has nothing to leak, so the
  BTB channel cannot be validated here in either direction.

Because a byte can take ~15 minutes, `run_safeside.sh` treats a correctly
recovered *prefix* of the secret as a leak rather than requiring all 16
characters -- one correct byte cannot be produced without the speculative
transmission having worked, and the all-or-nothing check would otherwise
report a working attack as a timeout.

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
