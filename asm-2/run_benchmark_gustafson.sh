#!/bin/bash
# ---------------------------------------------------------------
# 159.735 Assignment 2: Gustafson's Law benchmark for bucket_sort
#
# Unlike Amdahl's Law testing (fixed N, varying p), Gustafson's Law
# testing keeps the WORK PER PROCESS fixed and scales the problem
# size N proportionally with the number of processes p. If the
# program scales well, total_time should stay roughly CONSTANT as
# p increases (since each process always has the same amount of
# work to do) -- this is the essence of Gustafson's Law.
#
# Usage:
#   chmod +x run_benchmark_gustafson.sh
#   ./run_benchmark_gustafson.sh
# ---------------------------------------------------------------

set -e

EXECUTABLE=./bucket_sort
WORK_PER_PROC=5000000       # numbers per process (edit to fit your cluster/time budget)
PROCS=(1 2 4 8 16)          # numproc values to test (edit to match your cluster)
REPEATS=3                   # repeat each configuration for averaging
OUTFILE="gustafson_results.csv"

# Compile first (safe to re-run; skips if already built and up to date)
if [ ! -f "$EXECUTABLE" ] || [ "bucket_sort.cpp" -nt "$EXECUTABLE" ]; then
    echo "Compiling bucket_sort.cpp ..."
    mpic++ -O3 -o bucket_sort bucket_sort.cpp
fi

# CSV header
echo "numproc,run,N,sorted_ok,scatter_time,classify_time,alltoallv_time,sort_time,gatherv_time,total_time" > "$OUTFILE"

for p in "${PROCS[@]}"; do
    N=$(( WORK_PER_PROC * p ))   # scale problem size with p -- this is the key Gustafson step
    for r in $(seq 1 "$REPEATS"); do
        echo "Running numproc=$p, N=$N (run $r) ..."
        OUTPUT=$(mpirun -np "$p" "$EXECUTABLE" "$N")

        sorted_ok=$(echo "$OUTPUT"   | grep "Sorted correctly" | sed -E 's/.*: *([A-Z]+).*/\1/')
        scatter_t=$(echo "$OUTPUT"   | grep "Scatter time"     | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        classify_t=$(echo "$OUTPUT"  | grep "Classify time"    | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        alltoall_t=$(echo "$OUTPUT"  | grep "Alltoallv time"   | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        sort_t=$(echo "$OUTPUT"      | grep "Sort time"        | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        gather_t=$(echo "$OUTPUT"    | grep "Gatherv time"     | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')
        total_t=$(echo "$OUTPUT"     | grep "Total time"       | sed -E 's/.*= *([0-9.eE+-]+) s.*/\1/')

        echo "$p,$r,$N,$sorted_ok,$scatter_t,$classify_t,$alltoall_t,$sort_t,$gather_t,$total_t" >> "$OUTFILE"
    done
done

echo ""
echo "Done. Results written to $OUTFILE"
echo ""
echo "Preview:"
column -t -s, "$OUTFILE"
