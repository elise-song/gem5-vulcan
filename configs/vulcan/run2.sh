#!/bin/bash
num_runs=$1       # number of repeated runs per (size, assoc) combo
num_accesses=$2  # number of secrets per run

outdir="configs/vulcan/data/${num_runs}_${num_accesses}"
mkdir -p "$outdir"

declare -A sizes=(
    ["128B"]=2
    ["256B"]=4
    ["512B"]=8
    ["1KiB"]=16
    ["2KiB"]=32
    ["4KiB"]=64
    ["8KiB"]=128
    ["16KiB"]=256
    ["32KiB"]=512
    ["64KiB"]=1024
    ["128KiB"]=2048
    # ["256KiB"]=4096
    # ["512KiB"]=8192
    # ["1MiB"]=16384
)

assocs=(1 2 4 8)

for size in "${!sizes[@]}"; do
    total_blocks=${sizes[$size]}
    for assoc in "${assocs[@]}"; do
        if (( total_blocks % assoc != 0 )); then
            continue
        fi
        num_sets=$(( total_blocks / assoc ))

        for ((i = 0 ; i < num_runs ; i++ )); do
            outprefix="${outdir}/${size}.assoc${assoc}.${i}"
            build/X86/gem5.opt --debug-flags=Cache \
                configs/vulcan/prime_probe.py $num_accesses $size locked $assoc $i \
                > "${outprefix}.debug.txt" 2>&1
            python3 configs/vulcan/report2.py \
                "${outprefix}.debug.txt" "${outprefix}.report.txt" \
                $num_accesses $num_sets
        done
    done
done 

python3 configs/vulcan/all_reports.py \
    "$outdir" \
    "configs/vulcan/summary_${num_runs}_${num_accesses}"