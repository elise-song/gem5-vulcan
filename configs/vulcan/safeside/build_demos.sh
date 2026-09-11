#!/bin/bash
# Builds the subset of Google SafeSide (github.com/google/safeside) demos
# that are feasible to run under gem5's classic (X86, SE-mode) simulation:
# single-process, single-threaded, no signal-handler-based fault recovery,
# no ptrace, no out-of-tree kernel module. See ../README-gem5.md for why the
# rest of the demos (meltdown, l1tf, the ptrace-based speculation_over_*
# demos, meltdown_ac/de, the i686-only and aarch64-only demos) are excluded.
#
# Usage: ./build_demos.sh [outdir]
# Produces static binaries (required for gem5 SE mode) directly next to the
# sources by default, or in $1 if given. Uses the host g++ -- these are
# native x86-64 binaries, not cross-compiled, since gem5's X86 SE mode runs
# ordinary x86-64 Linux binaries.

set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMOS="$HERE/demos"
OUTDIR="${1:-$HERE/bin}"
mkdir -p "$OUTDIR"

CXX="${CXX:-g++}"
COMMON_SOURCES=(
    "$DEMOS/cache_sidechannel.cc"
    "$DEMOS/instr.cc"
    "$DEMOS/timing_array.cc"
    "$DEMOS/utils.cc"
    "$DEMOS/asm/measurereadlatency_x86_64.S"
)

build() {
    local name="$1"; shift
    echo "== building $name =="
    "$CXX" -std=c++11 -O1 -static -I"$DEMOS" \
        -o "$OUTDIR/$name" \
        "$DEMOS/$name.cc" "${COMMON_SOURCES[@]}" "$@"
}

build spectre_v1_pht_sa
build spectre_v1_btb_sa
build spectre_v4
build ret2spec_sa "$DEMOS/ret2spec_common.cc"

echo
echo "Built: $OUTDIR/{spectre_v1_pht_sa,spectre_v1_btb_sa,spectre_v4,ret2spec_sa}"
