#!/bin/bash
# Builds the DOLMA paper's own attack suite (github.com/efeslab/dolma,
# attacks/src/) as static x86-64 Linux binaries for this repo's gem5 SE-mode
# harness. See README-gem5.md for what's ported, what changed, and why.
#
# Usage: ./build_attacks.sh [outdir]
# Uses the host g++ -- these are native x86-64 binaries, not cross-compiled.

set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/../../.." && pwd)"
SRC="$HERE/src"
OUTDIR="${1:-$HERE}"
mkdir -p "$OUTDIR"

CC="${CC:-gcc}"

build() {
    local name="$1"
    echo "== building $name =="
    "$CC" -O0 -static "$SRC/$name.c" -o "$OUTDIR/$name"
}

build control_mem_dcache_load
build control_mem_btb_branch
build control_mem_icache_branch
build control_reg_dcache_load
build data_mem_dcache_load

echo "== building control_mem_dtlb_store (needs gem5/m5ops.h) =="
"$CC" -O0 -static -I "$REPO_ROOT/include" \
    "$SRC/control_mem_dtlb_store.c" \
    "$REPO_ROOT/util/m5/src/abi/x86/m5op.S" \
    -o "$OUTDIR/control_mem_dtlb_store"

echo
echo "Built: $OUTDIR/{control_mem_dcache_load,control_mem_btb_branch,control_mem_icache_branch,control_reg_dcache_load,data_mem_dcache_load,control_mem_dtlb_store}"
