# Scaling results

```
machine : Apple M5, 10 logical CPUs (10 physical cores)
memory  : 32 GB
os      : macOS 26.3 (arm64)
compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
openmp  : OMP_PROC_BIND=unset OMP_PLACES=unset OMP_WAIT_POLICY=unset
sweep   : up to 10 threads, budget x1, 5 repeats (analysis uses the fastest)
cores   : 4 performance + 6 efficiency
marker  : 4 performance cores
started : Sun Sep 20 17:31:49 NZST 2026
load at start: { 1.76 2.10 2.28 }
```

Measurement spread across repeats (median over fastest): median 1%, worst 8%. The fastest run of each configuration is used below.

## 1. The parallel version gives the same answer

Every run below was taken all the way to convergence. `iterations` and `image hash` (FNV-1a over the raw pixel bytes) are compared across the sequential program and the OpenMP program at every thread count and both partitions.

| npix | iterations | distinct iteration counts | distinct image hashes | runs compared |
|---:|---:|---:|---:|---:|
| 100 | 3,711 | 1 | 1 | 22 |
| 200 | 13,543 | 1 | 1 | 22 |
| 400 | 47,875 | 1 | 1 | 22 |
| 800 | 165,265 | 1 | 1 | 3 |

**Every configuration needed the identical number of iterations and produced a bit-identical image.**

## 2. Strong scaling (Amdahl's law)

Fixed plate, fixed number of iterations, varying thread count. Speedup is against the *sequential program*, not against the parallel program on one thread.

### 400x400, 10000 iterations -- sequential 0.784 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.767 | 1.02 | 1.02 | - | 100% | 0.769 | 1.02 |
| 2 | 0.520 | 1.51 | 0.75 | 0.327 | 80% | 0.524 | 1.50 |
| 3 | 0.407 | 1.92 | 0.64 | 0.279 | 64% | 0.420 | 1.87 |
| 4 | 0.386 | 2.03 | 0.51 | 0.323 | 54% | 0.385 | 2.04 |
| 5 | 0.539 | 1.45 | 0.29 | 0.610 | 35% | 0.472 | 1.66 |
| 6 | 0.560 | 1.40 | 0.23 | 0.658 | 29% | 0.522 | 1.50 |
| 7 | 0.574 | 1.37 | 0.20 | 0.688 | 24% | 0.585 | 1.34 |
| 8 | 0.577 | 1.36 | 0.17 | 0.699 | 21% | 0.605 | 1.29 |
| 9 | 0.627 | 1.25 | 0.14 | 0.775 | 18% | 0.653 | 1.20 |
| 10 | 0.700 | 1.12 | 0.11 | 0.881 | 17% | 0.730 | 1.07 |

Best static speedup **2.03x at 4 threads**.
 Least squares Amdahl serial fraction **f = 0.624**, so the ceiling 1/f is about **1.6x**.

### 800x800, 2500 iterations -- sequential 0.789 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.783 | 1.01 | 1.01 | - | 100% | 0.784 | 1.01 |
| 2 | 0.426 | 1.85 | 0.93 | 0.079 | 94% | 0.431 | 1.83 |
| 3 | 0.323 | 2.45 | 0.82 | 0.113 | 86% | 0.322 | 2.45 |
| 4 | 0.263 | 3.00 | 0.75 | 0.111 | 80% | 0.267 | 2.95 |
| 5 | 0.345 | 2.29 | 0.46 | 0.297 | 55% | 0.259 | 3.05 |
| 6 | 0.344 | 2.29 | 0.38 | 0.323 | 46% | 0.287 | 2.75 |
| 7 | 0.325 | 2.43 | 0.35 | 0.313 | 41% | 0.303 | 2.61 |
| 8 | 0.304 | 2.59 | 0.32 | 0.298 | 39% | 0.310 | 2.54 |
| 9 | 0.312 | 2.53 | 0.28 | 0.320 | 35% | 0.313 | 2.52 |
| 10 | 0.337 | 2.34 | 0.23 | 0.364 | 33% | 0.322 | 2.45 |

Best static speedup **3.00x at 4 threads**.
 Least squares Amdahl serial fraction **f = 0.296**, so the ceiling 1/f is about **3.4x**.

### 1600x1600, 625 iterations -- sequential 0.848 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.843 | 1.01 | 1.01 | - | 100% | 0.845 | 1.00 |
| 2 | 0.448 | 1.89 | 0.95 | 0.057 | 98% | 0.447 | 1.90 |
| 3 | 0.313 | 2.71 | 0.90 | 0.054 | 96% | 0.318 | 2.66 |
| 4 | 0.252 | 3.36 | 0.84 | 0.063 | 93% | 0.253 | 3.35 |
| 5 | 0.294 | 2.89 | 0.58 | 0.183 | 69% | 0.246 | 3.44 |
| 6 | 0.274 | 3.09 | 0.51 | 0.188 | 65% | 0.222 | 3.82 |
| 7 | 0.246 | 3.45 | 0.49 | 0.172 | 65% | 0.220 | 3.85 |
| 8 | 0.230 | 3.68 | 0.46 | 0.168 | 63% | 0.205 | 4.14 |
| 9 | 0.240 | 3.53 | 0.39 | 0.194 | 56% | 0.206 | 4.11 |
| 10 | 0.239 | 3.55 | 0.35 | 0.202 | 54% | 0.203 | 4.17 |

Best static speedup **3.68x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.177**, so the ceiling 1/f is about **5.6x**.

### 3200x3200, 156 iterations -- sequential 0.860 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.850 | 1.01 | 1.01 | - | 100% | 0.851 | 1.01 |
| 2 | 0.455 | 1.89 | 0.94 | 0.059 | 98% | 0.458 | 1.88 |
| 3 | 0.309 | 2.78 | 0.93 | 0.040 | 98% | 0.316 | 2.72 |
| 4 | 0.249 | 3.45 | 0.86 | 0.053 | 95% | 0.248 | 3.47 |
| 5 | 0.284 | 3.02 | 0.60 | 0.163 | 72% | 0.238 | 3.62 |
| 6 | 0.246 | 3.50 | 0.58 | 0.143 | 74% | 0.213 | 4.03 |
| 7 | 0.218 | 3.94 | 0.56 | 0.130 | 76% | 0.201 | 4.29 |
| 8 | 0.199 | 4.33 | 0.54 | 0.121 | 77% | 0.186 | 4.63 |
| 9 | 0.203 | 4.24 | 0.47 | 0.140 | 70% | 0.177 | 4.85 |
| 10 | 0.195 | 4.40 | 0.44 | 0.141 | 69% | 0.173 | 4.97 |

Best static speedup **4.40x at 10 threads**.
 Least squares Amdahl serial fraction **f = 0.132**, so the ceiling 1/f is about **7.5x**.

### The small end, from the runs to convergence

These are complete runs of the real program, so they show what a user would actually see.

| npix | iterations | sequential (s) | best parallel (s) | threads | speedup |
|---:|---:|---:|---:|---:|---:|
| 100 | 3,711 | 0.016 | 0.016 (static) | 1 | 1.02 |
| 200 | 13,543 | 0.251 | 0.249 (static) | 1 | 1.01 |
| 400 | 47,875 | 3.614 | 1.864 (static) | 4 | 1.94 |
| 800 | 165,265 | 51.929 | 21.261 (dynamic) | 10 | 2.44 |

![strong scaling](strong_scaling.png)

## 3. Weak scaling (Gustafson's law)

The plate side grows as `400*sqrt(p)`, so the pixels per thread -- and so the arithmetic each thread does per iteration -- stay constant. Perfect scaling would keep the parallel time flat and give a scaled speedup of `p`.

| p | npix | pixels/thread | sequential (s) | static (s) | scaled speedup | dynamic (s) | scaled speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 400 | 160,000 | 0.118 | 0.115 | 1.02 | 0.116 | 1.01 |
| 2 | 566 | 160,178 | 0.234 | 0.136 | 1.72 | 0.138 | 1.70 |
| 4 | 800 | 160,000 | 0.469 | 0.159 | 2.96 | 0.161 | 2.92 |
| 8 | 1131 | 159,895 | 0.974 | 0.310 | 3.15 | 0.277 | 3.51 |
| 10 | 1265 | 160,022 | 1.202 | 0.379 | 3.17 | 0.321 | 3.74 |

Fitting Gustafson's `S = p - a(p-1)`: **a = 0.521** for the static partition, **a = 0.500** for the dynamic one.

![weak scaling](weak_scaling.png)

## 4. Where the parallel time goes

### Synchronisation cost

A 64x64 plate for 1,672 iterations -- small enough that the arithmetic is cheap and the run is dominated by the two barriers and the reduction.

The overhead below is what the parallel run costs *above perfectly divided work*, `t(p) - t_seq/p`. Subtracting `t_seq/p` rather than `t_seq` is what makes this portable between machines: where a core is slow enough that even a 64x64 sweep is not free, splitting the arithmetic over `p` threads saves real time, and measuring against `t_seq` would net that saving off against the barrier cost and report a negative overhead.

| threads | time (s) | perfectly divided (s) | overhead (us/iteration) |
|---:|---:|---:|---:|
| 1 | 0.0033 | 0.0032 | 0.07 |
| 2 | 0.0159 | 0.0016 | 8.56 |
| 3 | 0.0225 | 0.0011 | 12.83 |
| 4 | 0.0279 | 0.0008 | 16.18 |
| 5 | 0.0349 | 0.0006 | 20.47 |
| 6 | 0.0519 | 0.0005 | 30.71 |
| 7 | 0.0624 | 0.0005 | 37.07 |
| 8 | 0.0683 | 0.0004 | 40.60 |
| 9 | 0.0717 | 0.0004 | 42.69 |
| 10 | 0.0815 | 0.0003 | 48.58 |

### Load balance: static vs dynamic partition

The mean fraction of the wall clock that a thread spent doing arithmetic rather than waiting at a barrier. Low numbers mean cores are sitting idle.

Plate 3200x3200:

| threads | static busy% | static busiest/idlest | dynamic busy% | dynamic busiest/idlest |
|---:|---:|---:|---:|---:|
| 1 | 100% | 100% / 100% | 100% | 100% / 100% |
| 2 | 98% | 100% / 97% | 98% | 99% / 98% |
| 3 | 98% | 99% / 97% | 97% | 98% / 96% |
| 4 | 95% | 97% / 92% | 96% | 97% / 95% |
| 5 | 72% | 97% / 64% | 89% | 96% / 84% |
| 6 | 74% | 97% / 61% | 90% | 93% / 87% |
| 7 | 76% | 95% / 61% | 87% | 95% / 83% |
| 8 | 77% | 94% / 59% | 88% | 94% / 80% |
| 9 | 70% | 86% / 50% | 87% | 92% / 80% |
| 10 | 69% | 85% / 47% | 86% | 91% / 80% |

![overhead and balance](overhead_balance.png)

