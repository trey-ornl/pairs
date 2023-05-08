#!/bin/bash
module load rocm
module -t list
export LD_LIBRARY_PATH="${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
set -x
export MPICH_GPU_SUPPORT_ENABLED=1
export MPICH_OFI_NIC_POLICY=GPU

mkdir -p tioga

flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 m | tee tioga/2.1.0.m.txt
flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 h | tee tioga/2.1.0.h.txt
flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 d | tee tioga/2.1.0.d.txt

flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 m | tee tioga/8.1.0.m.txt
flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 h | tee tioga/8.1.0.h.txt
flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 d | tee tioga/8.1.0.d.txt

flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 m | tee tioga/2.2.0.m.txt
flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 h | tee tioga/2.2.0.h.txt
flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 d | tee tioga/2.2.0.d.txt

flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 m | tee tioga/16.2.0.m.txt
flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 h | tee tioga/16.2.0.h.txt
flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 0 d | tee tioga/16.2.0.d.txt

flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 m | tee tioga/2.1.1024.m.txt
flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 h | tee tioga/2.1.1024.h.txt
flux run -n 2 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 d | tee tioga/2.1.1024.d.txt

flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 m | tee tioga/8.1.1024.m.txt
flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 h | tee tioga/8.1.1024.h.txt
flux run -n 8 -N 1 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 d | tee tioga/8.1.1024.d.txt

flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 m | tee tioga/2.2.1024.m.txt
flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 h | tee tioga/2.2.1024.h.txt
flux run -n 2 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 d | tee tioga/2.2.1024.d.txt

flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 m | tee tioga/16.2.1024.m.txt
flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 h | tee tioga/16.2.1024.h.txt
flux run -n 16 -N 2 -x -g 1 --queue=hpe_only -t 1m ./pairs 1024 d | tee tioga/16.2.1024.d.txt

