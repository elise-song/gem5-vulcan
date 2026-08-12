#!/bin/bash
#
# timing_histogram.sh
# My CHTC job
#
git clone https://github.com/elise-song/gem5-vulcan.git
cd gem5-vulcan
git checkout elise
scons build/ALL/gem5.opt -j$(nproc) 

cd configs/vulcan/amber_cache
gcc timing_histogram.c -std=gnu99 -DDO_READ=1 -o timing_histogram_read
gcc timing_histogram.c -std=gnu99 -DDO_WRITE=1 -o timing_histogram_write
gcc timing_histogram.c -std=gnu99 -DDO_FLUSH=1 -o timing_histogram_flush

cd ../../..
if [ "$1" == "read" ]; then
    build/ALL/gem5.opt configs/vulcan/amber_cache/timing_histogram.py $1
elif [ "$1" == "write" ]; then
    build/ALL/gem5.opt configs/vulcan/amber_cache/timing_histogram.py $1
elif [ "$1" == "flush" ]; then
    build/ALL/gem5.opt configs/vulcan/amber_cache/timing_histogram.py $1
else
    echo "Invalid argument. Please use 'read', 'write', or 'flush'."
fi
