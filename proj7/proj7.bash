#!/bin/bash

#SBATCH -J PROJ7-LEEVI
#SBATCH -A cs475-575
#SBATCH -p classmpitest
#SBATCH -N 16 # number of nodes
#SBATCH -n 16 # number of tasks
#SBATCH -o mpiproject.out
#SBATCH -e mpiproject.err
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=leevi@oregonstate.edu

for b in 1 2 4 6 8
    do
        module load openmpi
        mpic++ proj7.cpp -o proj7 -lm
        mpiexec -mca btl self,tcp -np $b ./proj7
    done
