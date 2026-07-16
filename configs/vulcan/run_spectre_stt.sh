#!/bin/bash
# Runs the Spectre v1 (bounds-check-bypass) PoC (spectre.c) under gem5 SE mode
# twice -- once with no speculative-execution defense, once with STT plus the
# implicit-channel protection enabled -- and reports how many secret bytes
# each run recovers, so you can see the mitigation actually close the
# cache-timing covert channel rather than just compile and run.
#
# Usage: ./run_spectre_stt.sh
# Expects: gem5.opt binary on PATH (or edit GEM5 below),
#           the spectre binary already built via the compile line in spectre.c,
#           run from the gem5-vulcan repo root (or edit CONFIG below).

set -u

GEM5="${GEM5:-build/X86/gem5.opt}"
CONFIG="${CONFIG:-configs/deprecated/example/se.py}"
EXE="${EXE:-spectre}"
ARGS="${ARGS:-4 200}"
OUTDIR="${OUTDIR:-spectre_stt_out}"

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

echo "== UnsafeBaseline (no defense) =="
run baseline --threat_model=UnsafeBaseline --needsTSO=1

echo "== Spectre + STT + implicit_channel (defended) =="
run stt --threat_model=Spectre --needsTSO=1 --STT=1 --implicit_channel=1

echo
echo "Done. Full logs in $OUTDIR/{baseline,stt}/stdout.log"
