#!/bin/bash
module load rocm
export LD_LIBRARY_PATH="${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
set -x
export MPICH_GPU_SUPPORT_ENABLED=1
TASKS=$1
shift
NODES=$1
shift
flux run -n $TASKS -N $NODES -x -g 1 --queue=hpe_only -t 5m ./pairs $@ 
