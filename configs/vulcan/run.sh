#!/bin/bash
mkdir configs/vulcan/data
for ((i = 0 ; i < 100 ; i++ )); do 
    build/ALL/gem5.opt --debug-flags=Cache  configs/vulcan/prime_probe.py 1 > configs/vulcan/data/$i.debug.txt
    python3 configs/vulcan/report.py configs/vulcan/data/$i.debug.txt configs/vulcan/data/$i.report.txt
done