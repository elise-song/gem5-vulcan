#!/usr/bin/env bash
# Non-deterministic cache via Cache Decay: end-to-end evaluation.
#
#   1. directed gtest of the pure decay-lifetime logic,
#   2. reuse attack against a NORMAL cache       -> success_rate ~ 1.0,
#   3. reuse attack against a CACHE DECAY cache  -> success_rate ~ chance,
#   4. directed SE assertions (dirty writeback on decay, self-invalidation,
#      probe-miss-after-decay, touch reschedules the deadline).
#
# Run from the gem5 repo root:  ./configs/vulcan/run.sh
set -euo pipefail

GEM5=${GEM5:-build/X86/gem5.opt}
GTEST=${GTEST:-build/X86/mem/cache/cache_decay.test.opt}
CFG=configs/vulcan/decay_reuse.py
OUT=${OUT:-m5out/vulcan_decay}
TRIALS=${TRIALS:-200}
GAP=${GAP:-1000}         # prime->probe idle gap, in ns
# Decay lifetime distribution, centred on the prime->probe gap with a wide
# range so a primed line survives to the probe only about half the time --
# i.e. the channel is driven to chance. (Slightly below GAP to offset the
# fact that a line is installed a little before the idle gap begins.)
INTERVAL=${INTERVAL:-950} # base/mean decay lifetime, in ns
RANGE=${RANGE:-900}       # randomization half-width, in ns
SEED=${SEED:-1}

mkdir -p "$OUT"

echo "== [1/4] directed gtest of Cache Decay lifetime logic =="
"$GTEST"

echo
echo "== [2/4] reuse attack vs NORMAL cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/off" "$CFG" \
    --defense off --trials "$TRIALS" --gap-ns "$GAP" --seed "$SEED"

echo
echo "== [3/4] reuse attack vs CACHE DECAY cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/on" "$CFG" \
    --defense on --trials "$TRIALS" --gap-ns "$GAP" \
    --interval-ns "$INTERVAL" --range-ns "$RANGE" --seed "$SEED"

echo
echo "== [4/4] directed SE invariant check =="
"$GEM5" --outdir="$OUT/directed" "$CFG" --defense on --directed
python3 configs/vulcan/directed_check.py "$OUT/directed/stats.txt"

echo
echo "== reuse-attack success_rate (before vs after Cache Decay) =="
python3 configs/vulcan/report.py \
    "normal-cache=$OUT/off/stats.txt" \
    "cache-decay=$OUT/on/stats.txt"
