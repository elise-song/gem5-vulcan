#!/bin/bash
#
# timing_histogram.sh
# My CHTC job: runs timing_histogram for one operation (read, write, or
# flush) in gem5 FS mode
#
set -e

case "$1" in
    read)  op_flag=-DDO_READ=1 ;;
    write) op_flag=-DDO_WRITE=1 ;;
    flush) op_flag=-DDO_FLUSH=1 ;;
    *) echo "Invalid argument. Please use 'read', 'write', or 'flush'."; exit 1 ;;
esac

git clone https://github.com/elise-song/gem5-vulcan.git
cd gem5-vulcan
git checkout elise
scons build/ALL/gem5.opt -j$(nproc)

# Keep the downloaded kernel and disk image in the job's scratch space
export GEM5_RESOURCE_DIR=$(pwd)/gem5-resources

AMBER=configs/vulcan/amber/cache
mkdir -p $AMBER/bin
# Static so it runs regardless of the guest's libraries; fall back to dynamic
# (the Ubuntu 24.04 guest's glibc is new enough) if libc.a is unavailable
gcc -std=gnu99 -O0 -static $op_flag $AMBER/src/timing_histogram.c \
    $AMBER/src/cpu_info.c -o $AMBER/bin/timing_histogram_$1 ||
gcc -std=gnu99 -O0 $op_flag $AMBER/src/timing_histogram.c \
    $AMBER/src/cpu_info.c -o $AMBER/bin/timing_histogram_$1

build/ALL/gem5.opt -d m5out_$1 $AMBER/chtc_files/timing_histogram.py \
    --binary $AMBER/bin/timing_histogram_$1

cp m5out_$1/histogram.out histogram_$1.out
cp m5out_$1/coarse_histogram.out coarse_histogram_$1.out
