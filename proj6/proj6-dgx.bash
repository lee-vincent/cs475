#!/bin/bash
for s in 4096 16384 65536 262144 1048576 4194304
do
    for b in 8 32 64 128 256
    do
            g++ -DDATASIZE=$s -DLOCALSIZE=$b -o proj06 proj06.cpp /usr/local/apps/cuda/10.1/lib64/libOpenCL.so.1  -lm -fopenmp
            ./proj06
    done
done