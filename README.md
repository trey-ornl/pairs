# pairs
An MPI microbenchmark that uses strided parallel pairs of MPI ping-pong partners.
It currently supports HPE Cray EX supercomputers with AMD GPUs.

## Build
Run `make.sh` to compile the `pairs` executable. You may need to edit this script for the details of your environment.

## Run
The `pairs` executable takes 3 optional arguments.
1. Total message size per iteration, in number of `long` values. A non-positive value indicates the default of 128x1024x1024 `long`s (1 GiB).
2. Memory allocator. A value of "d" will use `hipMalloc`, "h" will use `hipHostMalloc`, and "a" will use `posix_memalign`. Any other value indicates the default `malloc`.
3. A misalignment offset value, in number of `long` values. The size of each buffer allocation will increase by this count, and the buffer pointer will be offset from the base allocation pointer by this count. A non-positive value indicates the default offset of zero.

The scripts `flux.sh` and `srun.sh` run `pairs` under the Flux and Slurm workload managers, respectively. They each require two arguments.
1. The total number of MPI tasks.
2. The total number of nodes.

All remaining arguments get passed to the `pairs` command. Thus, arugments 3-5 to these scripts become optional arguments 1-3 of `pairs`.

Runs to test bandwidth can use the default problem size of 128x1024x1024 `long`s.

Runs to test latency should use a smaller problem size, such as 1024 `long`s.

## Output
Output from `pairs` can serve as data files for [Gnuplot](https://gnuplot.sourceforge.net).

Output starts with a header of comment lines (prefixed with `#`).
```
# pairs: strided parallel pairs of MPI ping-pong partners
# rank: host cores device(closest cores)
```
A comment line for each MPI task follows, with the hostname, the CPU core IDs bound to the task, the PCIE ID for the default GPU device, and the nearest cores to that device.

Then come lines reporting the memory allocator, the total number of ping-pong task pairs (half the total number of tasks), and the timeout value.

The microbenchmark runs the following nested loops.
* The outer loop iterates over strides between pairs of tasks, starting with the largest possible stride, followed by every legal stride down to 1.
For example, a 16-task run uses pairs of tasks with ranks that are strided apart by 8, 4, 2, and 1.
  * A middle loop iterates over the number of seperate messages to partition the total buffer size into, starting with 1 large message, and ending with a message for each `long` value.
A timer measures each iteration of this middle loop.
If the iteration takes longer than the timeout (1 second), the middle loop exits, and the next iteration of the outer loop (the next stride between pairs) begins.
    * The innermost loop iterates over the paritions of the buffer, performing an MPI ping-pong for each partition.

Each iteration of the middle loop prints a line of space-separated Gnuplot data, as described by the following header comment.
```
# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)
```

Each iteration of the outer loop prints a comment header with the stride, along with two blank lines to delineate data "sets" that can be refenced using the Gnuplot `index` keyword.

Here is sample output from `./srun.sh 16 2 0 d` (16 MPI tasks, 2 nodes, default problem size, `hipMalloc`-ed buffers, no mis-alignment) on [OLCF's Crusher](https://docs.olcf.ornl.gov/systems/crusher_quick_start_guide.html#).
```
# ./pairs: strided parallel pairs of MPI ping-pong partners
# rank: host cores device(closest cores)
# 0: crusher141 1 209(0-15,64-79)
# 1: crusher141 9 214(0-15,64-79)
# 2: crusher141 17 201(16-31,80-95)
# 3: crusher141 25 206(16-31,80-95)
# 4: crusher141 33 217(32-47,96-111)
# 5: crusher141 41 222(32-47,96-111)
# 6: crusher141 49 193(48-63,112-127)
# 7: crusher141 57 198(48-63,112-127)
# 8: crusher142 1 209(0-15,64-79)
# 9: crusher142 9 214(0-15,64-79)
# 10: crusher142 17 201(16-31,80-95)
# 11: crusher142 25 206(16-31,80-95)
# 12: crusher142 33 217(32-47,96-111)
# 13: crusher142 41 222(32-47,96-111)
# 14: crusher142 49 193(48-63,112-127)
# 15: crusher142 57 198(48-63,112-127)
# allocator: hipMalloc
# pairs: 8
# total count: 134217728 longs (1024 MiB)
# timeout: 1s

# stride between partners: 8
# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)
1 134217728 68580.8 57342 45963.7 14.5813 17.4392 21.7563 14.5813 17.4392 21.7563
2 67108864 28989.6 26171.1 23354.9 17.2476 19.105 21.4088 34.4952 38.21 42.8176
4 33554432 13171.5 12462.2 11753.5 18.9804 20.0606 21.2703 75.9216 80.2424 85.0813
8 16777216 6258.83 6080.48 5898.41 19.9718 20.5576 21.1922 159.774 164.461 169.537
16 8388608 3043.77 2996.02 2950.55 20.5337 20.861 21.1825 328.54 333.776 338.92
32 4194304 1504.06 1491.98 1479.47 20.777 20.9453 21.1225 664.865 670.249 675.919
64 2097152 752.948 748.703 745.154 20.7518 20.8694 20.9688 1328.11 1335.64 1342.01
128 1048576 376.955 376.076 374.896 20.7253 20.7737 20.8391 2652.84 2659.04 2667.41
256 524288 193.939 193.675 193.459 20.1416 20.1691 20.1916 5156.26 5163.3 5169.05
512 262144 101.449 101.333 101.204 19.2524 19.2744 19.2989 9857.21 9868.47 9881.05
1024 131072 54.9621 54.8579 54.6105 17.7679 17.8017 17.8823 18194.4 18228.9 18311.5
2048 65536 31.7633 31.748 31.7143 15.3725 15.3799 15.3963 31482.8 31498.1 31531.5
4096 32768 20.1807 20.1504 20.1208 12.0977 12.1159 12.1338 49552.3 49626.8 49699.9
8192 16384 13.1108 13.1064 13.1032 9.31066 9.31377 9.31605 76272.9 76298.4 76317.1
16384 8192 10.1737 10.1351 10.0952 5.9993 6.02215 6.04598 98292.5 98666.9 99057.3
32768 4096 8.40085 8.38482 8.37518 3.63268 3.63962 3.64381 119036 119263 119400
65536 2048 4.79157 4.76711 4.74271 3.18451 3.20085 3.21731 208700 209771 210850
131072 1024 4.18725 4.16723 4.1418 1.82206 1.83081 1.84205 238820 239967 241441


# stride between partners: 4
# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)
1 134217728 25844.3 25829.6 25819.9 38.6932 38.7153 38.7298 38.6932 38.7153 38.7298
2 67108864 12976.1 12965.7 12956 38.5324 38.5634 38.5921 77.0647 77.1268 77.1843
4 33554432 6494.91 6487.26 6481.78 38.4917 38.5371 38.5697 153.967 154.148 154.279
8 16777216 3248.28 3246.03 3244.26 38.4819 38.5085 38.5296 307.855 308.068 308.237
16 8388608 1628.15 1627.26 1626.13 38.3871 38.4082 38.4348 614.193 614.531 614.957
32 4194304 817.87 817.307 816.87 38.209 38.2353 38.2558 1222.69 1223.53 1224.19
64 2097152 412.609 412.171 411.75 37.8688 37.909 37.9478 2423.6 2426.17 2428.66
128 1048576 210.25 209.699 209.265 37.1581 37.2557 37.3331 4756.24 4768.73 4778.64
256 524288 109.11 108.559 108.071 35.8011 35.9829 36.1454 9165.07 9211.61 9253.21
512 262144 58.1899 57.696 57.259 33.5647 33.852 34.1103 17185.1 17332.2 17464.5
1024 131072 32.5989 32.1189 31.6972 29.9569 30.4046 30.8091 30675.9 31134.3 31548.5
2048 65536 19.8538 19.3908 18.9625 24.5939 25.181 25.7498 50368.3 51570.8 52735.6
4096 32768 13.4946 12.967 12.5233 18.0917 18.8278 19.4948 74103.5 77118.7 79850.9
8192 16384 10.5041 9.93256 9.37969 11.6212 12.2899 13.0143 95200.9 100679 106613
16384 8192 8.78292 8.28476 7.83827 6.9493 7.36716 7.78681 113857 120704 127579
32768 4096 8.08748 7.51249 7.01186 3.77344 4.06225 4.35228 123648 133112 142615
65536 2048 7.83057 7.30996 6.84734 1.94862 2.0874 2.22842 127705 136800 146042


# stride between partners: 2
# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)
1 134217728 34355.1 27661.6 20987.9 29.1078 36.1512 47.6465 29.1078 36.1512 47.6465
2 67108864 17194.2 13872.8 10555.8 29.0796 36.0418 47.3674 58.1591 72.0836 94.7347
4 33554432 8618.66 6949.07 5281.95 29.0068 35.976 47.331 116.027 143.904 189.324
8 16777216 4316.77 3480.16 2645.56 28.9568 35.9179 47.2489 231.655 287.343 377.992
16 8388608 2160.99 1743.71 1326.81 28.9219 35.8431 47.1053 462.751 573.489 753.685
32 4194304 1083.69 875.405 667.38 28.8367 35.6978 46.8249 922.775 1142.33 1498.4
64 2097152 545.658 441.529 337.641 28.6351 35.3884 46.277 1832.65 2264.86 2961.73
128 1048576 276.661 224.525 172.573 28.2385 34.7956 45.2708 3614.53 4453.84 5794.66
256 524288 142.077 116.146 90.3314 27.4938 33.6323 43.2435 7038.43 8609.88 11070.3
512 262144 74.8506 61.7276 48.7117 26.0937 31.641 40.0956 13359.9 16200.2 20528.9
1024 131072 40.8691 34.3741 27.9498 23.8949 28.4098 34.9399 24468.4 29091.6 35778.5
2048 65536 23.8031 20.5659 17.3595 20.5134 23.7423 28.1275 42011.4 48624.3 57605.2
4096 32768 15.2648 13.7511 12.3124 15.9937 17.7542 19.8288 65510.3 72721.3 81218.7
8192 16384 11.1275 10.3496 9.63406 10.9702 11.7947 12.6707 89867.5 96622.2 103798
16384 8192 8.79537 8.5594 8.3901 6.93946 7.13078 7.27467 113696 116831 119188
32768 4096 7.90691 7.77984 7.51975 3.85961 3.92265 4.05832 126472 128537 132983
65536 2048 7.71692 7.52903 7.1725 1.97732 2.02666 2.1274 129585 132819 139421


# stride between partners: 1
# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)
1 134217728 21010.3 20996.7 20990.2 47.5958 47.6265 47.6413 47.5958 47.6265 47.6413
2 67108864 10554.1 10552 10551 47.3749 47.3842 47.389 94.7498 94.7684 94.7779
4 33554432 5280.84 5279.56 5278.93 47.3409 47.3524 47.3581 189.364 189.41 189.432
8 16777216 2645.18 2644.13 2643.68 47.2557 47.2746 47.2825 378.046 378.197 378.26
16 8388608 1327.25 1326.54 1325.99 47.0899 47.115 47.1346 753.438 753.841 754.153
32 4194304 667.227 667.087 666.964 46.8357 46.8455 46.8541 1498.74 1499.05 1499.33
64 2097152 337.427 337.359 337.307 46.3063 46.3157 46.3227 2963.6 2964.2 2964.65
128 1048576 172.48 172.349 172.268 45.2952 45.3294 45.351 5797.78 5802.16 5804.92
256 524288 90.2769 90.1612 90.0421 43.2697 43.3252 43.3825 11077 11091.2 11105.9
512 262144 48.6514 48.5406 48.3729 40.1453 40.237 40.3764 20554.4 20601.3 20672.7
1024 131072 27.7843 27.7101 27.5992 35.148 35.2421 35.3838 35991.5 36088 36233
2048 65536 17.2187 17.1262 17.0323 28.3577 28.5108 28.668 58076.5 58390.1 58712
4096 32768 12.1261 12.0377 11.9196 20.1335 20.2813 20.4823 82466.8 83072.2 83895.6
8192 16384 9.42301 9.33147 9.22386 12.9545 13.0816 13.2342 106123 107164 108414
16384 8192 8.17638 8.09268 8.00275 7.46482 7.54202 7.62677 122304 123569 124957
32768 4096 7.5555 7.46274 7.35938 4.03912 4.08933 4.14676 132354 133999 135881
65536 2048 7.42193 7.32676 7.23945 2.05591 2.08261 2.10773 134736 136486 138132
131072 1024 7.23816 7.13983 7.02552 1.05405 1.06857 1.08595 138157 140059 142338
```
