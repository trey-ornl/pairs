#!/bin/bash
module load craype-accel-amd-gfx90a
module load rocm
module -t list
set -x
CC -ggdb -O $(hipconfig -C) -o pairs pairs.cc -L${ROCM_PATH}/lib -lamdhip64
