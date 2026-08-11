#!/bin/bash
# ---------------------------------------------------------------
# 159.735 Assignment 1: Automated benchmark script for gamma_mc
#
# Runs the compiled gamma_mc program across a range of process
# counts (numproc) and sample sizes (N), parses the timing output,
# and writes everything into a single CSV file for easy plotting
# and Amdahl's Law analysis in the report.
#
# Usage:
#   chmod +x run_benchmark.sh
#   ./run_benchmark.sh
#
# Adjust PROCS, SAMPLES, X_VALUE, and REPEATS below as needed.
# ---------------------------------------------------------------

set -e

EXECUTABLE=./gamma_mc
X_VALUE=5.0                 # Gamma(5) = 24, known value for accuracy checking
SAMPLES=100000000           # N: number of Monte Carlo samples per run
PROCS=(1 2 4 8 16)          # numproc values to test (edit to match your cluster)
REPEATS=3                   # repeat each configuration and keep results for averaging
OUTFILE="benchmark_results.csv"

# Compile first (safe to re-run; skips if already built and up to date)
if [ ! -f "$EXECUTABLE" ] || [ "gamma_mc.cpp" -nt "$EXECUTABLE" ]; then
    echo "Compiling gamma_mc.cpp ..."
    mpic++ -O3 -o gamma_mc gamma_mc.cpp
fi

# CSV header
echo "numproc,run,N,x,gamma_estimate,setup_time,comm_time,compute_time,reduce_time,total_time" > "$OUTFILE"

for p in "${PROCS[@]}"; do
    for r in $(seq 1 "$REPEATS"); do
        echo "Running numproc=$p, run=$r ..."
        OUTPUT=$(mpirun -np "$p" "$EXECUTABLE" "$X_VALUE" "$SAMPLES")

        gamma_est=$(echo "$OUTPUT"   | grep "Gamma("        | sed -E 's/.*= *([0-9.eE+-]+).*/\1/')
        setup_t=$(echo "$OUTPUT"     | grep "Setup time"    | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        comm_t=$(echo "$OUTPUT"      | grep "Comm time"     | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        compute_t=$(echo "$OUTPUT"   | grep "Compute time"  | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        reduce_t=$(echo "$OUTPUT"    | grep "Reduce time"   | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        total_t=$(echo "$OUTPUT"     | grep "Total time"    | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')

        echo "$p,$r,$SAMPLES,$X_VALUE,$gamma_est,$setup_t,$comm_t,$compute_t,$reduce_t,$total_t" >> "$OUTFILE"
    done
done

echo ""
echo "Done. Results written to $OUTFILE"
echo ""
echo "Preview:"
column -t -s, "$OUTFILE"