# asm-1

This directory contains a parallel Monte Carlo MPI implementation of the Gamma function for assignment 1 of 159.735.

## Contents

- `gamma_mc.cpp` — MPI C++ program that estimates `Gamma(x)` using a parallel leapfrog linear congruential generator (LCG) and Monte Carlo sampling.
- `gamma_mc` — compiled executable (if built).
- `run_benchmark.sh` — benchmark driver that runs `gamma_mc` over multiple process counts and writes timing results to `benchmark_results.csv`.
- `benchmark_results.csv` — example benchmark output with measured times for each process count and repeat.
- `analyze_results.py` — Python analysis script that averages repeated runs, computes speedup and efficiency, estimates an Amdahl serial fraction, and optionally generates `speedup_plot.png` if `matplotlib` is installed.
- `speedup_plot.png` — saved runtime speedup plot (optional output from `analyze_results.py`).

## Build

Compile the MPI program with `mpic++`:

```bash
mpic++ -O3 -o gamma_mc gamma_mc.cpp
```

If your MPI wrapper requires a specific C++ compiler on macOS, set `MPICH_CXX` first:

```bash
export MPICH_CXX=/usr/bin/clang++
mpic++ -O3 -o gamma_mc gamma_mc.cpp
```

## Run

Run the program with `mpirun` and specify the number of processes, the Gamma argument `x`, and the number of Monte Carlo samples `N`:

```bash
#                          x        N     
#             arg[0]    arg[1].  arg[2]
mpirun -np 4 ./gamma_mc 5.0 10000000

# log
===========================================
Gamma(5) estimate = 23.9929
N (samples)         = 10000000
numproc             = 4
Setup time (max)    = 0.000193 s
Comm time (max)     = 0.000192 s (99.4819% of setup time)
Compute time (max)  = 0.037868 s
Reduce time (max)   = 0.000355 s
Total time          = 0.038416 s
===========================================
```

A useful check is `Gamma(5.0) = 24`.

## Benchmark

The benchmark script runs `gamma_mc` over several process counts and repeats each configuration:

```bash
chmod +x run_benchmark.sh
./run_benchmark.sh
```

This produces `benchmark_results.csv` with columns:

- `numproc`
- `run`
- `N`
- `x`
- `gamma_estimate`
- `setup_time`
- `comm_time`
- `compute_time`
- `reduce_time`
- `total_time`

## Analyze

Use `analyze_results.py` to average repeated runs, compute measured speedup, compare against Amdahl's Law, and optionally generate a plot:

```bash
python analyze_results.py benchmark_results.csv
```

If `matplotlib` is installed, the script also saves `speedup_plot.png`.

## Notes

- `gamma_mc.cpp` uses MPI for parallel process coordination and a leapfrog RNG strategy so each rank draws from the same LCG sequence with minimal communication.
- The `run_benchmark.sh` script is intended for single-node MPI benchmarking and may need adjustments to `PROCS`, `SAMPLES`, and `REPEATS` for your local system.
