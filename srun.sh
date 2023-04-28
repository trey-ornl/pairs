#!/bin/bash
module load rocm
module -t list
export LD_LIBRARY_PATH="${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
set -x
export MPICH_GPU_SUPPORT_ENABLED=1
export MPICH_OFI_NIC_POLICY=GPU
TASKS=$1
shift
NODES=$1
shift
srun -n $TASKS -N $NODES --exclusive --gpus-per-task=1 --gpu-bind=closest -t 5:00 ./pairs $@ 
