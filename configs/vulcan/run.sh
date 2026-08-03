#!/bin/bash
# Copyright (c) 2024 The Regents of the gem5-vulcan study.
# SPDX-License-Identifier: BSD-3-Clause
#
# Directed cross-domain Prime+Probe experiment for the ScatterCache defense.
# Runs the identical attack against a plain set-associative cache (baseline)
# and against a ScatterAssociative cache with DISTINCT per-domain keys, and
# prints the attacker success_rate for each -- demonstrating that the channel
# is wide open on the baseline and collapses to noise under ScatterCache.
#
# Usage: configs/vulcan/run.sh [assoc] [cache_size] [num_secrets] [seed]

set -e

GEM5=build/X86/gem5.opt
CFG=configs/vulcan/prime_probe.py
REPORT=configs/vulcan/report.py
OUT=configs/vulcan/data
mkdir -p "$OUT"

ASSOC=${1:-1}
SIZE=${2:-4KiB}
NSEC=${3:-128}
SEED=${4:-1}

run() {
    local mode=$1
    local log="$OUT/${mode}.assoc${ASSOC}.${SIZE}.seed${SEED}.log"
    local rpt="$OUT/${mode}.assoc${ASSOC}.${SIZE}.seed${SEED}.report.txt"
    "$GEM5" --debug-flags=Cache "$CFG" "$mode" "$ASSOC" "$SIZE" "$NSEC" "$SEED" \
        > "$log" 2>&1
    python3 "$REPORT" "$log" "$rpt" > /dev/null
    grep "success_rate" "$rpt"
}

echo "ScatterCache cross-domain Prime+Probe"
echo "  assoc=$ASSOC size=$SIZE num_secrets=$NSEC seed=$SEED"
echo
printf "  baseline (plain set-associative) : "
run baseline
printf "  scatter  (distinct domain keys)  : "
run scatter
echo
echo "Interpretation: a high baseline rate means the eviction-based channel is"
echo "open; the scatter rate should collapse to ~noise (~1/num_sets), showing"
echo "the attacker can no longer build a cross-domain eviction set."
