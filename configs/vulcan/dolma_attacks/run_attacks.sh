#!/bin/bash
# Runs the ported DOLMA attack suite under gem5 SE mode and reports whether
# each one leaked its secret. See README-gem5.md for what's ported and why.
#
# Usage: ./run_attacks.sh
# Expects: gem5.opt already built (build/X86/gem5.opt), the attack binaries
#           already built (./build_attacks.sh), run from the gem5-vulcan
#           repo root (or edit CONFIG/BINDIR below).
#
# Set DOLMA_ARGS (e.g. DOLMA_ARGS=--dolma-mode=1) to test these attacks
# against this branch's DOLMA port instead of the undefended baseline.

set -u

GEM5="${GEM5:-build/X86/gem5.opt}"
CONFIG="${CONFIG:-configs/deprecated/example/se.py}"
BINDIR="${BINDIR:-configs/vulcan/dolma_attacks}"
OUTDIR="${OUTDIR:-dolma_attacks_out}"
DOLMA_ARGS="${DOLMA_ARGS:-}"
TIMEOUT_SECS="${TIMEOUT_SECS:-300}"

# Matches the DOLMA paper's own attack-suite config (attacks/main.py):
# a simple fixed-latency memory model, rather than this repo's other
# tests' default detailed DRAM timing, gives a much cleaner hit/miss signal
# for these single-trial attacks.
COMMON_ARGS=(
    --num-cpus=1 --mem-type=SimpleMemory --caches --l2cache --cpu-type=DerivO3CPU
    --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8
    --l2_size=2MB --l2_assoc=16
)

mkdir -p "$OUTDIR"

run_self_reporting() {
    local name="$1"
    local outsub="$OUTDIR/$name"
    mkdir -p "$outsub"
    timeout "$TIMEOUT_SECS" "$GEM5" --outdir="$outsub" "$CONFIG" \
        "${COMMON_ARGS[@]}" $DOLMA_ARGS -c "$BINDIR/$name" \
        > "$outsub/stdout.log" 2> "$outsub/stderr.log"
    local ec=$?
    if grep -q "Attack succeeded" "$outsub/stderr.log"; then
        echo "$name: LEAKED secret ($outsub/stderr.log)"
    elif grep -q "Attack failed" "$outsub/stderr.log"; then
        echo "$name: blocked ($outsub/stderr.log)"
    elif [ "$ec" = 124 ]; then
        echo "$name: TIMED OUT after ${TIMEOUT_SECS}s without a result"
    else
        echo "$name: NO RESULT (exit $ec) / check $outsub/stdout.log and $outsub/stderr.log"
    fi
}

run_dtlb() {
    local name="control_mem_dtlb_store"
    local outsub="$OUTDIR/$name"
    mkdir -p "$outsub"
    timeout "$TIMEOUT_SECS" "$GEM5" --debug-flags=TLB --outdir="$outsub" "$CONFIG" \
        "${COMMON_ARGS[@]}" $DOLMA_ARGS -c "$BINDIR/$name" \
        > "$outsub/stdout.log" 2> "$outsub/stderr.log"
    local ec=$?
    if [ "$ec" = 124 ]; then
        echo "$name: TIMED OUT after ${TIMEOUT_SECS}s"
        return
    fi
    "$(dirname "${BASH_SOURCE[0]}")/parse_tlb_log.py" "$outsub/stdout.log" \
        > "$outsub/verdict.log" 2>&1
    local verdict
    verdict="$(tail -1 "$outsub/verdict.log")"
    echo "$name: $verdict ($outsub/verdict.log)"
}

for demo in control_mem_dcache_load control_reg_dcache_load data_mem_dcache_load \
            control_mem_btb_branch control_mem_icache_branch; do
    run_self_reporting "$demo"
done
run_dtlb
