#!/bin/bash
module load rocm
export LD_LIBRARY_PATH="${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
set -x
export MPICH_GPU_SUPPORT_ENABLED=1
TASKS=$1
shift
NODES=$1
shift
THREADS=$(( 128 * NODES / TASKS ))
srun -n $TASKS -N $NODES -p bardpeak -c $THREADS -t 5:00 ./pairs $@ 
