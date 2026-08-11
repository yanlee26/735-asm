/*
  159.735 Assignment 1: Parallel Monte Carlo evaluation of the Gamma
  function using the Leapfrog method for parallel random number generation.

  Gamma(x) = Integral_0^1  (log(1/t))^(x-1)  dt

  Monte Carlo estimator:
      I(N) = (t2 - t1) * (1/N) * sum_{n=0}^{N-1} f(t_n)
  Here t1 = 0, t2 = 1, so (t2 - t1) = 1.

  Random numbers t_n in (0,1) are generated using an LCG:
      x_{i+1} = (a*x_i + c) mod m
  parallelised with the "leapfrog" technique so that all processes draw
  from the SAME underlying sequence, with minimal communication overhead.

  Usage:
      mpirun -np <p> ./gamma_mc <x> <N>
  e.g.
      mpirun -np 4 ./gamma_mc 5.0 10000000
  (Gamma(5) = 4! = 24, useful for checking accuracy)
*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include "mpi.h"

typedef unsigned long ULONG;

// LCG constants (fixed, do not change) (线性同余生成器的固定常数)
// These come directly from the assignment sheet and must NOT be changed --
// they define exactly which pseudo-random sequence everyone generates.

const ULONG a = 1664525UL;// Multiplier — multiplies the previous value x_i
                                  // in the recurrence x_(i+1) = (a*x_i + c) mod m
const ULONG c = 1013904223UL; // Increment — added after multiplying by a
                                  // (this is the "c" in the LCG recurrence)
const ULONG m = 4294967296UL; // Modulus — equals 2^32
                                  // keeps every generated value within [0, m-1],
                                  // i.e. within the range of a 32-bit unsigned integer

/*
  Safe modular exponentiation: (x^n) mod mod_val
  All values involved are < m (~2^32), so intermediate products
  (< 2^64) fit exactly in a 64-bit ULONG -- no overflow.
  Do NOT use pow() here (loses precision for large integers).
*/
ULONG mod_pow(ULONG x, ULONG n, ULONG mod_val)
{
  ULONG y = 1;
  for (ULONG e = 0; e < n; ++e) {
    y = (y * x) % mod_val;
  }
  return y;
}

// The integrand f(t) = (log(1/t))^(x-1) = (-log(t))^(x-1)
inline double integrand(double t, double x)
{
  // Guard against t == 0 (log(1/0) is undefined). With our LCG, t is
  // (x_next)/m, so t = 0 can occur only if x_next = 0, which is
  // extremely rare; clamp to avoid a NaN poisoning the sum.
  if (t <= 0.0) t = 1e-300;
  return std::pow(-std::log(t), x - 1.0);
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  if (argc < 3) {
    if (myid == 0) {
      std::cerr << "Usage: mpirun -np <p> " << argv[0] << " x N" << std::endl;
    }
    MPI_Finalize();
    return 1;
  }

  double x = std::atof(argv[1]);      // argument of the Gamma function
  ULONG  N = std::atol(argv[2]);      // total number of Monte Carlo samples

  int k = numproc; // jump constant = number of processes

  // -----------------------------------------------------------------
  // Step 1: every process computes its own jump constants A and C
  // -----------------------------------------------------------------
  double t_start_setup = MPI_Wtime();

  ULONG A = mod_pow(a, k, m);

  ULONG sum_terms = 0;
  for (int e = 0; e < k; ++e) {
    ULONG term = mod_pow(a, e, m);
    sum_terms = (sum_terms + term) % m; // distributivity, avoids overflow
  }
  // c and sum_terms are both < m (~2^32), so their product fits in 64 bits
  ULONG C = (c * sum_terms) % m;

  // -----------------------------------------------------------------
  // Step 2: master (rank 0) generates the first k numbers sequentially
  // and sends x_1..x_{k-1} to the corresponding processes (x_0 stays
  // with rank 0). This is the only interprocess communication needed.
  // -----------------------------------------------------------------
  const ULONG seed = 12345UL;
  ULONG my_x;
  MPI_Status stat;

  double t_comm_start = MPI_Wtime();

  if (myid == 0) {
    ULONG n_prev = seed;
    for (int p = 0; p < numproc; ++p) {
      ULONG n_next = (a * n_prev + c) % m;
      if (p == 0) {
        my_x = n_next;
      } else {
        MPI_Send(&n_next, 1, MPI_UNSIGNED_LONG, p, 0, MPI_COMM_WORLD);
      }
      n_prev = n_next;
    }
  } else {
    MPI_Recv(&my_x, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &stat);
  }

  double t_comm_end = MPI_Wtime();
  double local_comm_time = t_comm_end - t_comm_start;

  double t_end_setup = MPI_Wtime();
  double local_setup_time = t_end_setup - t_start_setup;

  // -----------------------------------------------------------------
  // Step 3: each process independently generates its own subsequence
  // of random numbers (leaping over the ones used by other processes)
  // and accumulates its local partial sum of the integrand -- NO
  // further communication needed here.
  // -----------------------------------------------------------------
  ULONG my_N = N / numproc;
  // Give any leftover samples (N not exactly divisible by numproc) to rank 0
  if (myid == 0) {
    my_N += N % numproc;
  }

  double t_compute_start = MPI_Wtime();

  ULONG x_prev = my_x;
  double local_sum = 0.0;

  for (ULONG i = 0; i < my_N; ++i) {
    ULONG x_next = (A * x_prev + C) % m;
    double t = static_cast<double>(x_next) / static_cast<double>(m); // t in [0,1)
    local_sum += integrand(t, x);
    x_prev = x_next;
  }

  double t_compute_end = MPI_Wtime();
  double local_compute_time = t_compute_end - t_compute_start;

  // -----------------------------------------------------------------
  // Step 4: combine local partial sums into the final estimate
  // -----------------------------------------------------------------
  double t_reduce_start = MPI_Wtime();

  double total_sum = 0.0;
  MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  double t_reduce_end = MPI_Wtime();
  double local_reduce_time = t_reduce_end - t_reduce_start;

  // Gather timing info at rank 0 for benchmarking / Amdahl's Law analysis
  double max_comm_time, max_compute_time, max_reduce_time, max_setup_time;
  MPI_Reduce(&local_comm_time, &max_comm_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_compute_time, &max_compute_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_reduce_time, &max_reduce_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_setup_time, &max_setup_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if (myid == 0) {
    double gamma_estimate = total_sum / static_cast<double>(N);
    double total_time = max_setup_time + max_compute_time + max_reduce_time;

    std::cout << "===========================================" << std::endl;
    std::cout << "Gamma(" << x << ") estimate = " << gamma_estimate << std::endl;
    std::cout << "N (samples)         = " << N << std::endl;
    std::cout << "numproc             = " << numproc << std::endl;
    std::cout << "Setup time (max)    = " << max_setup_time   << " s" << std::endl;
    std::cout << "Comm time (max)     = " << max_comm_time    << " s ("
              << (max_comm_time / max_setup_time * 100.0)
              << "% of setup time)" << std::endl;
    std::cout << "Compute time (max)  = " << max_compute_time << " s" << std::endl;
    std::cout << "Reduce time (max)   = " << max_reduce_time  << " s" << std::endl;
    std::cout << "Total time          = " << total_time       << " s" << std::endl;
    std::cout << "===========================================" << std::endl;
  }

  MPI_Finalize();
  return 0;
}