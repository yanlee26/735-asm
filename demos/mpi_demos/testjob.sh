#!/bin/bash

#SBATCH --partition=test 
#SBATCH --job-name=My_Job
#SBATCH --mem=120000
#SBATCH --nodes=1
#SBATCH --tasks-per-node=2
#SBATCH --cpus-per-task=1
#SBATCH --time=1:00:00
#SBATCH --mail-user=your.email@whatever.com
#SBATCH --mail-type=ALL

echo Job Started at: $(date)
echo
echo "hostname:" 
hostname
echo

# Load the required module and run the MPI program
# Make sure you use the same module you used to compile 
# the source code
module load openmpi4/intel-openapi/64/4.1.8-with-ucx 

echo Program output
mpiexec ./first 10000 
echo

echo "SLURM_JOBID="$SLURM_JOBID
echo "SLURM_NNODES"=$SLURM_NNODES
echo Job Finished at: $(date)
