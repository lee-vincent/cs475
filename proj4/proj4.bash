#!/bin/bash
for t in 1
do
  for n in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608
  do
     g++ all04.cpp -DNUMT=$t -DARRAYSIZE=$n -o proj04 -lm -fopenmp
    ./proj04
  done
done
