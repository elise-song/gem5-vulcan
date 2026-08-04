#!/usr/bin/env bash
# Random Fill cache: end-to-end evaluation.
#
#   1. directed gtest of the pure helper logic,
#   2. reuse attack against a NORMAL cache          -> success_rate ~ 1.0,
#   3. reuse attack against a RANDOM FILL cache      -> success_rate ~ 0.0,
#   4. directed SE assertion (S non-resident, one random fill injected).
#
# Run from the gem5 repo root:  ./configs/vulcan/run.sh
set -euo pipefail

GEM5=${GEM5:-build/X86/gem5.opt}
GTEST=${GTEST:-build/X86/mem/cache/random_fill.test.opt}
CFG=configs/vulcan/random_fill_reuse.py
OUT=${OUT:-m5out/vulcan_rf}
TRIALS=${TRIALS:-200}
WINDOW=${WINDOW:-8}
SEED=${SEED:-1}

mkdir -p "$OUT"

echo "== [1/4] directed gtest of Random Fill helper logic =="
"$GTEST"

echo
echo "== [2/4] reuse attack vs NORMAL cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/off" "$CFG" \
    --defense off --trials "$TRIALS" --window "$WINDOW" --seed "$SEED"

echo
echo "== [3/4] reuse attack vs RANDOM FILL cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/on" "$CFG" \
    --defense on --trials "$TRIALS" --window "$WINDOW" --seed "$SEED"

echo
echo "== [4/4] directed SE invariant check =="
"$GEM5" --outdir="$OUT/directed" "$CFG" --defense on --directed --window "$WINDOW"
python3 configs/vulcan/directed_check.py "$OUT/directed/stats.txt"

echo
echo "== reuse-attack success_rate (before vs after Random Fill) =="
python3 configs/vulcan/report.py \
    "normal-cache=$OUT/off/stats.txt" \
    "random-fill=$OUT/on/stats.txt"
