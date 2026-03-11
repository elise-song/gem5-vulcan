#!/bin/bash
mkdir configs/vulcan/data
num_runs=$2
for ((i = 0 ; i < $1 ; i++ )); do 
    build/ALL/gem5.opt --debug-flags=Cache  configs/vulcan/prime_probe.py $num_runs 16KiB > configs/vulcan/data/$i.debug.txt
    python3 configs/vulcan/report.py configs/vulcan/data/$i.debug.txt configs/vulcan/data/$i.report.txt $num_runs 256
done