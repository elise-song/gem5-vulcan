#!/bin/bash

git clone https://github.com/elise-song/gem5-vulcan.git
cd gem5-vulcan
git checkout elise
scons build/X86/gem5.opt -j$(nproc) CPU_MODELS=AtomicSimpleCPU,TimingSimpleCPU,O3CPU,MinorCPU