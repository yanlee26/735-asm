# Scaling results

```
machine : Intel(R) Xeon(R) Gold 6242R CPU @ 3.10GHz, 8 logical CPUs (8 physical cores)
memory  : 31 GB
os      : Ubuntu 24.04.4 LTS (kernel 6.8.0-139-generic, x86_64)
compiler: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
openmp  : OMP_PROC_BIND=close OMP_PLACES=cores OMP_WAIT_POLICY=unset
sweep   : up to 8 threads, budget x1, 3 repeats (analysis uses the fastest)
started : Sun 13 Sep 2026 20:04:46 NZST
load at start: { 0.04 0.24 0.14 }
```

Measurement spread across repeats (median over fastest): median 1%, worst 56%. The fastest run of each configuration is used below.

## 1. The parallel version gives the same answer

Every run below was taken all the way to convergence. `iterations` and `image hash` (FNV-1a over the raw pixel bytes) are compared across the sequential program and the OpenMP program at every thread count and both partitions.

| npix | iterations | distinct iteration counts | distinct image hashes | runs compared |
|---:|---:|---:|---:|---:|
| 100 | 3,711 | 1 | 1 | 18 |
| 200 | 13,543 | 1 | 1 | 18 |
| 400 | 47,875 | 1 | 1 | 18 |
| 800 | 165,265 | 1 | 1 | 3 |

**Every configuration needed the identical number of iterations and produced a bit-identical image.**

## 2. Strong scaling (Amdahl's law)

Fixed plate, fixed number of iterations, varying thread count. Speedup is against the *sequential program*, not against the parallel program on one thread.

### 400x400, 10000 iterations -- sequential 2.967 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2.985 | 0.99 | 0.99 | - | 100% | 2.991 | 0.99 |
| 2 | 1.507 | 1.97 | 0.98 | 0.015 | 99% | 1.519 | 1.95 |
| 3 | 1.013 | 2.93 | 0.98 | 0.012 | 98% | 1.029 | 2.88 |
| 4 | 0.769 | 3.86 | 0.96 | 0.012 | 97% | 0.825 | 3.60 |
| 5 | 0.642 | 4.62 | 0.92 | 0.020 | 93% | 0.708 | 4.19 |
| 6 | 0.595 | 4.99 | 0.83 | 0.041 | 86% | 0.598 | 4.96 |
| 7 | 0.458 | 6.48 | 0.93 | 0.013 | 93% | 0.489 | 6.07 |
| 8 | 0.458 | 6.48 | 0.81 | 0.034 | 84% | 0.480 | 6.19 |

Best static speedup **6.48x at 7 threads**.
 Least squares Amdahl serial fraction **f = 0.027**, so the ceiling 1/f is about **37.5x**.

### 800x800, 2500 iterations -- sequential 3.074 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 3.061 | 1.00 | 1.00 | - | 100% | 3.095 | 0.99 |
| 2 | 1.550 | 1.98 | 0.99 | 0.008 | 99% | 1.563 | 1.97 |
| 3 | 1.045 | 2.94 | 0.98 | 0.010 | 98% | 1.040 | 2.96 |
| 4 | 0.802 | 3.83 | 0.96 | 0.015 | 95% | 0.787 | 3.91 |
| 5 | 0.645 | 4.77 | 0.95 | 0.012 | 96% | 0.653 | 4.71 |
| 6 | 0.546 | 5.63 | 0.94 | 0.013 | 94% | 0.541 | 5.69 |
| 7 | 0.464 | 6.62 | 0.95 | 0.010 | 94% | 0.477 | 6.44 |
| 8 | 0.410 | 7.50 | 0.94 | 0.009 | 93% | 0.458 | 6.72 |

Best static speedup **7.50x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.010**, so the ceiling 1/f is about **96.6x**.

### 1600x1600, 625 iterations -- sequential 4.592 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 4.628 | 0.99 | 0.99 | - | 100% | 4.605 | 1.00 |
| 2 | 2.359 | 1.95 | 0.97 | 0.027 | 97% | 2.359 | 1.95 |
| 3 | 1.704 | 2.70 | 0.90 | 0.056 | 90% | 1.803 | 2.55 |
| 4 | 1.379 | 3.33 | 0.83 | 0.067 | 83% | 1.347 | 3.41 |
| 5 | 1.161 | 3.96 | 0.79 | 0.066 | 78% | 1.137 | 4.04 |
| 6 | 0.964 | 4.76 | 0.79 | 0.052 | 79% | 1.001 | 4.59 |
| 7 | 0.798 | 5.75 | 0.82 | 0.036 | 81% | 0.870 | 5.28 |
| 8 | 0.785 | 5.85 | 0.73 | 0.052 | 72% | 0.807 | 5.69 |

Best static speedup **5.85x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.049**, so the ceiling 1/f is about **20.3x**.

### 3200x3200, 156 iterations -- sequential 3.879 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 3.897 | 1.00 | 1.00 | - | 100% | 3.895 | 1.00 |
| 2 | 2.002 | 1.94 | 0.97 | 0.032 | 99% | 2.003 | 1.94 |
| 3 | 1.348 | 2.88 | 0.96 | 0.021 | 97% | 1.376 | 2.82 |
| 4 | 1.021 | 3.80 | 0.95 | 0.018 | 96% | 1.073 | 3.62 |
| 5 | 0.813 | 4.77 | 0.95 | 0.012 | 97% | 0.894 | 4.34 |
| 6 | 0.691 | 5.61 | 0.94 | 0.014 | 95% | 0.761 | 5.10 |
| 7 | 0.608 | 6.38 | 0.91 | 0.016 | 94% | 0.665 | 5.83 |
| 8 | 0.532 | 7.30 | 0.91 | 0.014 | 93% | 0.598 | 6.48 |

Best static speedup **7.30x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.015**, so the ceiling 1/f is about **69.0x**.

### The small end, from the runs to convergence

These are complete runs of the real program, so they show what a user would actually see.

| npix | iterations | sequential (s) | best parallel (s) | threads | speedup |
|---:|---:|---:|---:|---:|---:|
| 100 | 3,711 | 0.059 | 0.015 (static) | 8 | 3.79 |
| 200 | 13,543 | 0.992 | 0.158 (static) | 8 | 6.28 |
| 400 | 47,875 | 14.179 | 2.002 (static) | 8 | 7.08 |
| 800 | 165,265 | 197.337 | 27.602 (static) | 8 | 7.15 |

![strong scaling](strong_scaling.png)

## 3. Weak scaling (Gustafson's law)

The plate side grows as `400*sqrt(p)`, so the pixels per thread -- and so the arithmetic each thread does per iteration -- stay constant. Perfect scaling would keep the parallel time flat and give a scaled speedup of `p`.

| p | npix | pixels/thread | sequential (s) | static (s) | scaled speedup | dynamic (s) | scaled speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 400 | 160,000 | 0.441 | 0.446 | 0.99 | 0.446 | 0.99 |
| 2 | 566 | 160,178 | 0.898 | 0.456 | 1.97 | 0.457 | 1.96 |
| 4 | 800 | 160,000 | 1.837 | 0.485 | 3.79 | 0.477 | 3.85 |
| 8 | 1131 | 159,895 | 3.940 | 0.560 | 7.03 | 0.565 | 6.97 |

Fitting Gustafson's `S = p - a(p-1)`: **a = 0.080** for the static partition, **a = 0.077** for the dynamic one.

![weak scaling](weak_scaling.png)

## 4. Where the parallel time goes

### Synchronisation cost

A 64x64 plate for 1,672 iterations -- small enough that the arithmetic is cheap and the run is dominated by the two barriers and the reduction.

The overhead below is what the parallel run costs *above perfectly divided work*, `t(p) - t_seq/p`. Subtracting `t_seq/p` rather than `t_seq` is what makes this portable between machines: where a core is slow enough that even a 64x64 sweep is not free, splitting the arithmetic over `p` threads saves real time, and measuring against `t_seq` would net that saving off against the barrier cost and report a negative overhead.

| threads | time (s) | perfectly divided (s) | overhead (us/iteration) |
|---:|---:|---:|---:|
| 1 | 0.0115 | 0.0114 | 0.09 |
| 2 | 0.0071 | 0.0057 | 0.82 |
| 3 | 0.0055 | 0.0038 | 1.05 |
| 4 | 0.0049 | 0.0028 | 1.23 |
| 5 | 0.0048 | 0.0023 | 1.49 |
| 6 | 0.0049 | 0.0019 | 1.78 |
| 7 | 0.0051 | 0.0016 | 2.10 |
| 8 | 0.0052 | 0.0014 | 2.27 |

### Load balance: static vs dynamic partition

The mean fraction of the wall clock that a thread spent doing arithmetic rather than waiting at a barrier. Low numbers mean cores are sitting idle.

Plate 3200x3200:

| threads | static busy% | static busiest/idlest | dynamic busy% | dynamic busiest/idlest |
|---:|---:|---:|---:|---:|
| 1 | 100% | 100% / 100% | 100% | 100% / 100% |
| 2 | 99% | 99% / 98% | 98% | 98% / 98% |
| 3 | 97% | 98% / 95% | 95% | 95% / 94% |
| 4 | 96% | 99% / 93% | 92% | 94% / 91% |
| 5 | 97% | 99% / 95% | 88% | 89% / 88% |
| 6 | 95% | 98% / 91% | 86% | 87% / 85% |
| 7 | 94% | 96% / 90% | 85% | 85% / 84% |
| 8 | 93% | 98% / 86% | 83% | 84% / 82% |

![overhead and balance](overhead_balance.png)

