#!/usr/bin/env bash
# Random Fill cache: end-to-end evaluation.
#
#   1. gtest of the pure random-fill draw logic (includes
#      CollisionTimingSignalVanishesAtFullWindow, which measures the P1-P2
#      collision-timing gap the paper's Eq. 6 says must vanish),
#   2. reuse attack against a NORMAL cache      -> success_rate ~ 1.0,
#   3. reuse attack against a RANDOM FILL cache -> success_rate ~ 1/(2W+1),
#      not 0 -- see configs/vulcan/README.md,
#   4. directed SE invariant check (internally consistent either way the
#      random-fill draw for S landed), and
#   5. collision_attack.py: the same P1 vs P2 measurement as the gtest, but
#      empirically through the real timing-cache model end to end.
#
# Run from the gem5 repo root:  ./configs/vulcan/run.sh
set -euo pipefail

GEM5=${GEM5:-build/X86/gem5.opt}
GTEST=${GTEST:-build/X86/mem/cache/random_fill.test.opt}
CFG=configs/vulcan/random_fill_reuse.py
COLLISION_CFG=configs/vulcan/collision_attack.py
OUT=${OUT:-m5out/vulcan_rf}
TRIALS=${TRIALS:-200}
WINDOW=${WINDOW:-8}
SEED=${SEED:-1}
COLLISION_TRIALS=${COLLISION_TRIALS:-800}

mkdir -p "$OUT"

echo "== [1/5] gtest of Random Fill helper logic =="
"$GTEST"

echo
echo "== [2/5] reuse attack vs NORMAL cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/off" "$CFG" \
    --defense off --trials "$TRIALS" --window "$WINDOW" --seed "$SEED"

echo
echo "== [3/5] reuse attack vs RANDOM FILL cache ($TRIALS trials) =="
"$GEM5" --outdir="$OUT/on" "$CFG" \
    --defense on --trials "$TRIALS" --window "$WINDOW" --seed "$SEED"

echo
echo "== [4/5] directed SE invariant check =="
"$GEM5" --outdir="$OUT/directed" "$CFG" --defense on --directed --window "$WINDOW"
python3 configs/vulcan/directed_check.py "$OUT/directed/stats.txt"

echo
echo "== reuse-attack success_rate (before vs after Random Fill) =="
python3 configs/vulcan/report.py \
    "normal-cache=$OUT/off/stats.txt" \
    "random-fill=$OUT/on/stats.txt"

echo
echo "== [5/5] collision-timing-attack benchmark (P1 vs P2, $COLLISION_TRIALS trials) =="
"$GEM5" --outdir="$OUT/collision" "$COLLISION_CFG" \
    --window 16 --trials "$COLLISION_TRIALS" --seed "$SEED"
python3 configs/vulcan/collision_report.py "$OUT/collision/stats.txt"
