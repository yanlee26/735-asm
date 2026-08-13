/*
  159.735 Assignment 1: Parallel random number generation
  using the Leapfrog Method with MPI

  Sequential LCG (Linear Congruential Generator):
      x_{i+1} = (a * x_i + c) mod m

  Leapfrog property used here:
      x_{i+k} = (A * x_i + C) mod m
  where k is the "jump constant" (= number of processes, numproc),
      A = a^k mod m
      C = c * (a^(k-1) + a^(k-2) + ... + a^1 + a^0) mod m

  Each process:
    1. Computes its own A and C (they only depend on a, c, m, and k = numproc)
    2. Receives its own starting value x_i from the master (rank 0),
       where i = myid
    3. Independently (no further communication) generates its own
       subsequence x_i, x_{i+k}, x_{i+2k}, ... by leaping over the
       numbers used by the other processes
*/

#include <cstdlib>
#include <iostream>
#include "mpi.h"

typedef unsigned long ULONG; // use 64-bit unsigned integers throughout,
                              // as recommended in the assignment notes

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
  Computes (x^n) mod mod_val using the safe iterative method from the
  assignment notes (NOT pow(), which would overflow / lose precision).
  Since x < m (~2^32) and mod_val = m (~2^32), the intermediate product
  (y * x) is always < 2^64, which fits exactly in a 64-bit ULONG -- so no
  overflow occurs.
*/
ULONG mod_pow(ULONG x, ULONG n, ULONG mod_val)
{
  ULONG y = 1;
  for (ULONG e = 0; e < n; ++e) {
    y = (y * x) % mod_val;
  }
  return y;
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  if (argc < 2) {
    if (myid == 0) {
      std::cerr << "Usage: mpirun -np <p> " << argv[0] << " N" << std::endl;
    }
    MPI_Finalize();
    return 1;
  }

  ULONG N = std::atol(argv[1]); // total number of random samples wanted
                                 // (should not exceed m)
  int k = numproc;              // jump constant = number of processes

  MPI_Status stat;

  // ---------------------------------------------------------------
  // Step 1: each process computes its own jump constants A and C
  // ---------------------------------------------------------------
  ULONG A = mod_pow(a, k, m);

  // C = c * (a^(k-1) + a^(k-2) + ... + a^1 + a^0) mod m
  // Using distributivity: (sum_i a^i) mod m computed term by term
  ULONG sum_terms = 0;
  for (int e = 0; e < k; ++e) {
    ULONG term = mod_pow(a, e, m);
    sum_terms = (sum_terms + term) % m;
  }
  // c and sum_terms are both < m (~2^32), so their product fits in 64 bits
  ULONG C = (c * sum_terms) % m;

  // ---------------------------------------------------------------
  // Step 2: master (rank 0) generates the first k numbers using the
  // ordinary SEQUENTIAL recurrence and sends one to each process,
  // so that x_0 -> P0, x_1 -> P1, x_2 -> P2, ... x_{k-1} -> P_{k-1}
  // ---------------------------------------------------------------
  const ULONG seed = 12345UL; // same seed as the sequential version
  ULONG my_x;                 // this process's own starting point

  if (myid == 0) {
    ULONG n_prev = seed;
    for (int p = 0; p < numproc; ++p) {
      ULONG n_next = (a * n_prev + c) % m;
      if (p == 0) {
        my_x = n_next; // rank 0 keeps x_0 for itself
      } else {
        MPI_Send(&n_next, 1, MPI_UNSIGNED_LONG, p, 0, MPI_COMM_WORLD);
      }
      n_prev = n_next;
    }
    if (myid == 0) {
        std::cout << "k=" << k << " A=" << A << " C=" << C << std::endl;
    }  
} else {
    MPI_Recv(&my_x, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &stat);
  }

  // ---------------------------------------------------------------
  // Step 3: each process independently generates its own subsequence
  // by leaping over the numbers used by the other processes.
  // NO further communication is needed from this point on.
  // ---------------------------------------------------------------
  ULONG my_N = N / numproc; // number of samples this process must generate
  ULONG x_prev = my_x;

  double local_sum = 0.0; // example use of the random numbers

  for (ULONG i = 0; i < my_N; ++i) {
    ULONG x_next = (A * x_prev + C) % m;

    // Convert to a double in [0,1) if needed for your actual computation
    double r = static_cast<double>(x_next) / static_cast<double>(m);

    // ... do something with the random number r ...
    local_sum += r;

    x_prev = x_next;
  }

  std::cout << "Process " << myid << " generated " << my_N
            << " numbers, local sum = " << local_sum << std::endl;

  // ---------------------------------------------------------------
  // Step 4: combine results from all processes (example: MPI_Reduce)
  // ---------------------------------------------------------------
  double total_sum = 0.0;
  MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  if (myid == 0) {
    std::cout << "Total sum across all processes = " << total_sum << std::endl;
  }

  MPI_Finalize();
  return 0;
}