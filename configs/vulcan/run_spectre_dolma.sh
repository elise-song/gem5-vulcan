#!/bin/bash
# Runs the Spectre v1 (bounds-check-bypass) PoC (spectre.c) under gem5 SE mode
# three times -- with no speculative-execution defense, with DOLMA's default
# mode, and with DOLMA's conservative mode -- and reports how many secret
# bytes each run recovers, so you can see the mitigation actually close the
# cache-timing covert channel rather than just compile and run.
#
# Usage: ./configs/vulcan/run_spectre_dolma.sh
# Expects: gem5.opt binary already built (build/X86/gem5.opt),
#           the spectre binary already built via the compile line in spectre.c
#           (gcc -O0 -static configs/vulcan/spectre.c -o configs/vulcan/spectre),
#           run from the gem5-vulcan repo root (or edit CONFIG/EXE below).

set -u

GEM5="${GEM5:-build/X86/gem5.opt}"
CONFIG="${CONFIG:-configs/deprecated/example/se.py}"
EXE="${EXE:-configs/vulcan/spectre}"
ARGS="${ARGS:-4 200}"
OUTDIR="${OUTDIR:-spectre_dolma_out}"

mkdir -p "$OUTDIR"

run() {
    local name="$1"; shift
    local outsub="$OUTDIR/$name"
    mkdir -p "$outsub"
    "$GEM5" --outdir="$outsub" "$CONFIG" \
        --num-cpus=1 --mem-size=512MB --caches --l2cache --cpu-type=DerivO3CPU \
        "$@" -c "$EXE" -o "$ARGS" \
        > "$outsub/stdout.log" 2> "$outsub/stderr.log"
    grep -m1 '^RESULT:' "$outsub/stdout.log" || echo "ERROR_NO_OUTPUT (check $outsub/stdout.log)"
}

echo "== Baseline (no defense) =="
run baseline --dolma-mode=0

echo "== DOLMA default mode =="
run dolma_default --dolma-mode=1

echo "== DOLMA conservative mode =="
run dolma_conservative --dolma-mode=2

echo
echo "Done. Full logs in $OUTDIR/{baseline,dolma_default,dolma_conservative}/stdout.log"
