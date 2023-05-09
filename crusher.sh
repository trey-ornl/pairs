#!/bin/bash
module load craype-accel-amd-gfx90a
module load rocm
module -t list
export LD_LIBRARY_PATH="${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
set -x
export MPICH_GPU_SUPPORT_ENABLED=1
export MPICH_OFI_NIC_POLICY=GPU

mkdir -p crusher

srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 m | tee crusher/2.1.0.m.txt
srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 h | tee crusher/2.1.0.h.txt
srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 d | tee crusher/2.1.0.d.txt

srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 m | tee crusher/8.1.0.m.txt
srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 h | tee crusher/8.1.0.h.txt
srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 d | tee crusher/8.1.0.d.txt

srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 m | tee crusher/2.2.0.m.txt
srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 h | tee crusher/2.2.0.h.txt
srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 d | tee crusher/2.2.0.d.txt

srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 m | tee crusher/16.2.0.m.txt
srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 h | tee crusher/16.2.0.h.txt
srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 0 d | tee crusher/16.2.0.d.txt

srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 m | tee crusher/2.1.1024.m.txt
srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 h | tee crusher/2.1.1024.h.txt
srun -n 2 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 d | tee crusher/2.1.1024.d.txt

srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 m | tee crusher/8.1.1024.m.txt
srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 h | tee crusher/8.1.1024.h.txt
srun -n 8 -N 1 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 d | tee crusher/8.1.1024.d.txt

srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 m | tee crusher/2.2.1024.m.txt
srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 h | tee crusher/2.2.1024.h.txt
srun -n 2 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 d | tee crusher/2.2.1024.d.txt

srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 m | tee crusher/16.2.1024.m.txt
srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 h | tee crusher/16.2.1024.h.txt
srun -n 16 -N 2 --exclusive --gpus-per-task=1 --gpu-bind=closest -t 1:00 ./pairs 1024 d | tee crusher/16.2.1024.d.txt

