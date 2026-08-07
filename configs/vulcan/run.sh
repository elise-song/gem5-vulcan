#!/bin/bash
mkdir configs/vulcan/data
build/ALL/gem5.opt --debug-flags=Cache  configs/vulcan/vipt.py 16KiB > configs/vulcan/data/debug.txt
