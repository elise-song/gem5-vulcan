#!/bin/bash
mkdir configs/vulcan/data

declare -A sizes=(
    ["128B"]=2 #good 
    ["256B"]=4
    ["512B"]=8 #bad
    ["1KiB"]=16
    ["2KiB"]=32
    ["4KiB"]=64
    ["8KiB"]=128
    ["16KiB"]=256 #good 
    ["32KiB"]=512
    ["64KiB"]=1024 #good 
    ["128KiB"]=2048
    # ["256KiB"]=4096
    # ["512KiB"]=8192
    # ["1MiB"]=16384
)
num_runs=$2
for size in "${!sizes[@]}"; do
    num_sets=${sizes[$size]}
    for ((i = 0 ; i < $1 ; i++ )); do 
        build/ALL/gem5.opt --debug-flags=Cache  configs/vulcan/prime_probe.py $num_runs $size > configs/vulcan/data/$num_sets.$i.debug.txt
        python3 configs/vulcan/report.py configs/vulcan/data/$num_sets.$i.debug.txt configs/vulcan/data/$num_sets.$i.report.txt $num_runs $num_sets
    done
done