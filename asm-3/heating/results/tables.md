# Scaling results

```
machine : Apple M5, 10 cores (4 performance + 6 efficiency)
memory  : 32 GB
os      : macOS 26.3 (arm64)
compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
repeats : 5 (analysis uses the fastest)
load at start: { 3.04 5.63 5.94 }
```

Measurement spread across repeats (median over fastest): median 1%, worst 11%. The fastest run of each configuration is used below.

## 1. The parallel version gives the same answer

Every run below was taken all the way to convergence. `iterations` and `image hash` (FNV-1a over the raw pixel bytes) are compared across the sequential program and the OpenMP program at every thread count and both partitions.

| npix | iterations | distinct iteration counts | distinct image hashes | runs compared |
|---:|---:|---:|---:|---:|
| 100 | 3,711 | 1 | 1 | 30 |
| 200 | 13,543 | 1 | 1 | 30 |
| 400 | 47,875 | 1 | 1 | 30 |
| 800 | 165,265 | 1 | 1 | 13 |

**Every configuration needed the identical number of iterations and produced a bit-identical image.**

## 2. Strong scaling (Amdahl's law)

Fixed plate, fixed number of iterations, varying thread count. Speedup is against the *sequential program*, not against the parallel program on one thread.

### 400x400, 10000 iterations -- sequential 0.762 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.761 | 1.00 | 1.00 | - | 100% | 0.764 | 1.00 |
| 2 | 0.518 | 1.47 | 0.74 | 0.358 | 80% | 0.526 | 1.45 |
| 3 | 0.405 | 1.88 | 0.63 | 0.296 | 64% | 0.411 | 1.85 |
| 4 | 0.386 | 1.97 | 0.49 | 0.342 | 54% | 0.386 | 1.97 |
| 5 | 0.540 | 1.41 | 0.28 | 0.635 | 35% | 0.465 | 1.64 |
| 6 | 0.559 | 1.36 | 0.23 | 0.680 | 29% | 0.530 | 1.44 |
| 7 | 0.575 | 1.33 | 0.19 | 0.713 | 24% | 0.582 | 1.31 |
| 8 | 0.578 | 1.32 | 0.16 | 0.724 | 21% | 0.610 | 1.25 |
| 9 | 0.624 | 1.22 | 0.14 | 0.795 | 18% | 0.657 | 1.16 |
| 10 | 0.705 | 1.08 | 0.11 | 0.916 | 17% | 0.731 | 1.04 |

Best static speedup **1.97x at 4 threads**.
 Least squares Amdahl serial fraction **f = 0.646**, so the ceiling 1/f is about **1.5x**.

### 800x800, 2500 iterations -- sequential 0.780 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.781 | 1.00 | 1.00 | - | 100% | 0.784 | 1.00 |
| 2 | 0.425 | 1.84 | 0.92 | 0.088 | 94% | 0.431 | 1.81 |
| 3 | 0.314 | 2.49 | 0.83 | 0.103 | 87% | 0.312 | 2.50 |
| 4 | 0.263 | 2.96 | 0.74 | 0.116 | 80% | 0.261 | 2.99 |
| 5 | 0.344 | 2.27 | 0.45 | 0.301 | 55% | 0.261 | 2.99 |
| 6 | 0.344 | 2.27 | 0.38 | 0.328 | 46% | 0.287 | 2.72 |
| 7 | 0.324 | 2.41 | 0.34 | 0.318 | 41% | 0.303 | 2.57 |
| 8 | 0.305 | 2.56 | 0.32 | 0.303 | 39% | 0.310 | 2.52 |
| 9 | 0.316 | 2.47 | 0.27 | 0.331 | 35% | 0.306 | 2.55 |
| 10 | 0.316 | 2.47 | 0.25 | 0.339 | 34% | 0.310 | 2.52 |

Best static speedup **2.96x at 4 threads**.
 Least squares Amdahl serial fraction **f = 0.296**, so the ceiling 1/f is about **3.4x**.

### 1600x1600, 620 iterations -- sequential 0.833 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.833 | 1.00 | 1.00 | - | 100% | 0.837 | 0.99 |
| 2 | 0.435 | 1.92 | 0.96 | 0.044 | 98% | 0.437 | 1.90 |
| 3 | 0.309 | 2.70 | 0.90 | 0.056 | 95% | 0.313 | 2.66 |
| 4 | 0.244 | 3.42 | 0.85 | 0.057 | 93% | 0.243 | 3.42 |
| 5 | 0.291 | 2.86 | 0.57 | 0.187 | 69% | 0.236 | 3.52 |
| 6 | 0.270 | 3.08 | 0.51 | 0.189 | 65% | 0.217 | 3.84 |
| 7 | 0.243 | 3.43 | 0.49 | 0.174 | 64% | 0.215 | 3.87 |
| 8 | 0.224 | 3.71 | 0.46 | 0.165 | 62% | 0.200 | 4.16 |
| 9 | 0.237 | 3.51 | 0.39 | 0.196 | 55% | 0.197 | 4.23 |
| 10 | 0.233 | 3.58 | 0.36 | 0.200 | 54% | 0.197 | 4.22 |

Best static speedup **3.71x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.177**, so the ceiling 1/f is about **5.7x**.

### 3200x3200, 155 iterations -- sequential 0.843 s

| threads | static t (s) | speedup | efficiency | Karp-Flatt e | busy% | dynamic t (s) | speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0.843 | 1.00 | 1.00 | - | 100% | 0.846 | 1.00 |
| 2 | 0.439 | 1.92 | 0.96 | 0.042 | 98% | 0.433 | 1.94 |
| 3 | 0.311 | 2.71 | 0.90 | 0.053 | 98% | 0.310 | 2.72 |
| 4 | 0.241 | 3.50 | 0.88 | 0.047 | 96% | 0.243 | 3.47 |
| 5 | 0.280 | 3.01 | 0.60 | 0.165 | 73% | 0.233 | 3.61 |
| 6 | 0.244 | 3.46 | 0.58 | 0.147 | 75% | 0.205 | 4.12 |
| 7 | 0.214 | 3.95 | 0.56 | 0.129 | 76% | 0.197 | 4.27 |
| 8 | 0.194 | 4.35 | 0.54 | 0.120 | 77% | 0.182 | 4.62 |
| 9 | 0.200 | 4.22 | 0.47 | 0.142 | 69% | 0.175 | 4.82 |
| 10 | 0.194 | 4.35 | 0.43 | 0.145 | 69% | 0.174 | 4.85 |

Best static speedup **4.35x at 8 threads**.
 Least squares Amdahl serial fraction **f = 0.134**, so the ceiling 1/f is about **7.5x**.

### The small end, from the runs to convergence

These are complete runs of the real program, so they show what a user would actually see.

| npix | iterations | sequential (s) | best parallel (s) | threads | speedup |
|---:|---:|---:|---:|---:|---:|
| 100 | 3,711 | 0.016 | 0.016 (dynamic) | 1 | 1.00 |
| 200 | 13,543 | 0.243 | 0.242 (static) | 1 | 1.01 |
| 400 | 47,875 | 3.580 | 1.884 (dynamic) | 4 | 1.90 |
| 800 | 165,265 | 51.883 | 17.711 (static) | 4 | 2.93 |

![strong scaling](strong_scaling.png)

## 3. Weak scaling (Gustafson's law)

The plate side grows as `400*sqrt(p)`, so the pixels per thread -- and so the arithmetic each thread does per iteration -- stay constant. Perfect scaling would keep the parallel time flat and give a scaled speedup of `p`.

| p | npix | pixels/thread | sequential (s) | static (s) | scaled speedup | dynamic (s) | scaled speedup |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 400 | 160,000 | 0.121 | 0.118 | 1.03 | 0.116 | 1.05 |
| 2 | 566 | 160,178 | 0.230 | 0.135 | 1.70 | 0.138 | 1.67 |
| 3 | 693 | 160,083 | 0.348 | 0.147 | 2.36 | 0.149 | 2.35 |
| 4 | 800 | 160,000 | 0.476 | 0.159 | 3.00 | 0.157 | 3.04 |
| 6 | 980 | 160,066 | 0.710 | 0.266 | 2.66 | 0.217 | 3.27 |
| 8 | 1131 | 159,895 | 0.941 | 0.288 | 3.27 | 0.269 | 3.50 |
| 10 | 1265 | 160,022 | 1.174 | 0.371 | 3.16 | 0.329 | 3.57 |

Fitting Gustafson's `S = p - a(p-1)`: **a = 0.509** for the static partition, **a = 0.480** for the dynamic one.

![weak scaling](weak_scaling.png)

## 4. Where the parallel time goes

### Synchronisation cost

A 64x64 plate for 1,672 iterations. The arithmetic is almost free at this size, so whatever the parallel run costs above the sequential one is the price of the two barriers and the reduction.

| threads | time (s) | overhead over sequential (us/iteration) |
|---:|---:|---:|
| 1 | 0.0032 | 0.03 |
| 2 | 0.0167 | 8.12 |
| 3 | 0.0220 | 11.25 |
| 4 | 0.0277 | 14.66 |
| 5 | 0.0431 | 23.87 |
| 6 | 0.0577 | 32.65 |
| 7 | 0.0652 | 37.12 |
| 8 | 0.0728 | 41.67 |
| 9 | 0.0819 | 47.08 |
| 10 | 0.1017 | 58.95 |

### Load balance: static vs dynamic partition

The mean fraction of the wall clock that a thread spent doing arithmetic rather than waiting at a barrier. Low numbers mean cores are sitting idle.

Plate 3200x3200:

| threads | static busy% | static busiest/idlest | dynamic busy% | dynamic busiest/idlest |
|---:|---:|---:|---:|---:|
| 1 | 100% | 100% / 100% | 100% | 100% / 100% |
| 2 | 98% | 100% / 97% | 99% | 99% / 98% |
| 3 | 98% | 98% / 97% | 97% | 98% / 96% |
| 4 | 96% | 98% / 92% | 97% | 97% / 95% |
| 5 | 73% | 97% / 65% | 89% | 96% / 84% |
| 6 | 75% | 96% / 62% | 91% | 95% / 86% |
| 7 | 76% | 96% / 61% | 87% | 95% / 83% |
| 8 | 77% | 94% / 58% | 88% | 94% / 82% |
| 9 | 69% | 86% / 49% | 87% | 92% / 81% |
| 10 | 69% | 85% / 48% | 86% | 90% / 80% |

![overhead and balance](overhead_balance.png)

