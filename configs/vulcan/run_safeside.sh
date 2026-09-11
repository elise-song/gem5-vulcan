#!/bin/bash
# Runs the gem5-SE-mode-feasible subset of Google SafeSide's demos (see
# configs/vulcan/safeside/README-gem5.md) under gem5 SE mode and reports
# whether each one leaked the embedded secret string.
#
# Usage: ./configs/vulcan/run_safeside.sh
# Expects: gem5.opt already built (build/X86/gem5.opt), the demo binaries
#           already built (./configs/vulcan/safeside/build_demos.sh),
#           run from the gem5-vulcan repo root (or edit CONFIG/BINDIR below).
#
# Set DOLMA_ARGS (e.g. DOLMA_ARGS=--dolma-mode=1) to test these demos
# against this branch's DOLMA port instead of the undefended baseline.
#
# spectre_v4 and spectre_v1_btb_sa don't reliably converge under gem5's
# default O3 config within any practical time budget (see README-gem5.md
# for the diagnosis -- the underlying speculation does happen, it just
# doesn't turn into a usable timing leak in gem5's model), and their own
# internal retry loops only give up after up to 100000 attempts per byte,
# which can take far longer than that to reach in simulation. Each demo
# below is therefore run under `timeout` so this script always terminates;
# override TIMEOUT_SECS (default 600) if you want to give the two
# non-convergent demos more of a chance.

set -u

GEM5="${GEM5:-build/X86/gem5.opt}"
CONFIG="${CONFIG:-configs/deprecated/example/se.py}"
BINDIR="${BINDIR:-configs/vulcan/safeside/bin}"
OUTDIR="${OUTDIR:-safeside_out}"
DOLMA_ARGS="${DOLMA_ARGS:-}"
TIMEOUT_SECS="${TIMEOUT_SECS:-600}"

mkdir -p "$OUTDIR"

run() {
    local name="$1"
    local outsub="$OUTDIR/$name"
    mkdir -p "$outsub"
    timeout "$TIMEOUT_SECS" "$GEM5" --outdir="$outsub" "$CONFIG" \
        --num-cpus=1 --mem-size=512MB --caches --l2cache --cpu-type=DerivO3CPU \
        $DOLMA_ARGS -c "$BINDIR/$name" \
        > "$outsub/stdout.log" 2> "$outsub/stderr.log"
    local ec=$?
    if grep -q "It's a s3kr3t" "$outsub/stdout.log"; then
        echo "$name: LEAKED secret ($outsub/stdout.log)"
    elif grep -q "Does not converge" "$outsub/stdout.log"; then
        echo "$name: did not converge ($outsub/stdout.log)"
    elif [ "$ec" = 124 ]; then
        echo "$name: TIMED OUT after ${TIMEOUT_SECS}s without leaking (not necessarily broken -- see README-gem5.md; retry with a larger TIMEOUT_SECS)"
    else
        echo "$name: NO LEAK / check $outsub/stdout.log and $outsub/stderr.log"
    fi
}

for demo in spectre_v1_pht_sa spectre_v1_btb_sa spectre_v4 ret2spec_sa; do
    echo "== $demo =="
    run "$demo"
done
