#!/bin/bash

#SBATCH --partition=test 
#SBATCH --job-name=My_Job
#SBATCH --mem=120000
#SBATCH --nodes=1
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --time=1:00:00
#SBATCH --mail-user=your.email@whatever.com
#SBATCH --mail-type=ALL

echo Job Started at: $(date)
echo
echo "\$ hostname:" 
hostname
echo

# Load the required module and run the MPI program
# Make sure you use the same module you used to compile
# the source code. If `module` is not available (e.g. on a
# local macOS machine), fall back and continue.
if command -v module >/dev/null 2>&1; then
	module load gcc/64/14.2.0
else
	echo "module command not found; proceeding without module."
	echo "If you're running locally, ensure you have a compatible compiler/libomp installed (brew install gcc libomp)."
fi

# Run the program (some demos accept a numeric arg to set threads)
./omp_hello 8

echo "SLURM_JOBID="$SLURM_JOBID
echo "SLURM_NNODES"=$SLURM_NNODES

echo Job Finished at: $(date)
