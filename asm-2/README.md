# 159.735 Assignment 2 — Parallel Bucket Sort

MPI implementation of parallel bucket sort using the "small bucket / large
bucket" approach, with variable-size collective communication routines
(Scatterv, Alltoallv, Gatherv).

## Files

| File | Purpose |
|---|---|
| `bucket_sort.cpp` | MPI program: master generates and distributes the unsorted array (Scatterv), each process classifies its chunk into small buckets by value range, small buckets are redistributed into large buckets (Alltoallv), each large bucket is sorted locally with `qsort()`, and results are collected back at the master (Gatherv). |
| `run_benchmark_gustafson.sh` | Automated benchmark script for testing **Gustafson's Law** — keeps work-per-process fixed and scales N proportionally with numproc (1, 2, 4, 8, 16 by default), 3 repeats each, writing results to `gustafson_results.csv`. |
| `analyze_gustafson.py` | Reads `gustafson_results.csv` and computes the scaled speedup S_scaled(p) = p·T(1)/T(p) and an estimated serial/overhead fraction (alpha); saves `gustafson_plot.png` if matplotlib is available. |

## Algorithm summary

1. **Master** generates N random numbers in `[0, MAXVAL)`.
2. **Scatterv**: array is split into numproc chunks by *quantity* (not
   value) and distributed — this first partition is inherently
   load-balanced regardless of the value distribution.
3. Each process classifies its local chunk into numproc **small buckets**
   by value range.
4. **Alltoallv**: small buckets are redistributed so process *i* receives
   everyone's small bucket *i*, forming its **large bucket**.
5. Each process sorts its own large bucket with `qsort()`.
6. **Gatherv**: sorted large buckets are collected back at the master, in
   rank order, giving the final sorted array.

## Building and running on CTCP

All computation must run as SLURM jobs on CTCP — do not run directly on
the login node.

```bash
# 1. Copy files to CTCP
scp bucket_sort.cpp run_benchmark_gustafson.sh analyze_gustafson.py \
    26023952@it096843.massey.ac.nz:~/assignment2/

# 2. SSH in
ssh 26023952@it096843.massey.ac.nz -p 2044
cd ~/assignment2
chmod +x run_benchmark_gustafson.sh analyze_gustafson.py

# 3. Compile (the benchmark script also does this automatically)
mpic++ -O3 -o bucket_sort bucket_sort.cpp

# 4. Manual single-run test, e.g. to check correctness/timing directly
mpirun -np 4 ./bucket_sort 20000000

# 5. Full Gustafson's Law benchmark sweep — wrap in a SLURM job (see below)
./run_benchmark_gustafson.sh

# 6. Analyze results
python3 analyze_gustafson.py
```

## Running the benchmark via SLURM

```bash
#!/bin/bash
#SBATCH --job-name=bucket_sort_bench
#SBATCH --nodes=1
#SBATCH --ntasks=16          # must be >= the largest numproc tested
#SBATCH --time=00:30:00
#SBATCH --output=slurm_%j.out

chmod +x run_benchmark_gustafson.sh
./run_benchmark_gustafson.sh
```

Save as `submit_gustafson.sh`, then:
```bash
sbatch submit_gustafson.sh
squeue -u 26023952        # check status
cat slurm_<jobid>.out     # view output once finished
```

**Important:** `--ntasks` must be at least as large as the biggest
`numproc` value in `run_benchmark_gustafson.sh`'s `PROCS=(...)` array (16
by default). CTCP has 32 cores available (`nproc`), so `--ntasks=16` fits
without oversubscription.

## Interpreting results for the report

- **Gustafson's Law expectation:** since work-per-process is held fixed
  and N scales with p, `total_time` should stay roughly *constant* as p
  increases if the program scales well. The scaled speedup
  `S_scaled(p) = p·T(1)/T(p)` should then track close to `p` (linear).
- **If `total_time` grows noticeably with p:** check the per-phase
  breakdown in `gustafson_results.csv` — the `alltoallv_time` and
  `gatherv_time` columns are the most likely source, since their
  communication volume grows with the number of processes.
- **Parts expected to closely follow Gustafson's Law:** `scatter_time` and
  `classify_time`, since their work scales linearly with N/p and does not
  depend on inter-process communication volume.
- Adjust `WORK_PER_PROC`, `PROCS=(...)`, and `REPEATS` at the top of
  `run_benchmark_gustafson.sh` to fit your time budget.
