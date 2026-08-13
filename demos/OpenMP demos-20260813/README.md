# OpenMP Demos

This repository contains simple OpenMP demo programs used for teaching and experimentation with parallel programming concepts.

Prerequisites
- A C++ compiler with OpenMP support (e.g. `g++` with `-fopenmp`).
- Optionally an MPI C++ wrapper (e.g. `mpic++`) if you prefer using `mpic++` for compilation.

Building

Build individual demos with either `g++` or `mpic++` as appropriate. Examples:

```
g++ -O3 -fopenmp -o omp_hello omp_hello.cpp
g++ -O3 -fopenmp -o omp_barrier omp_barrier.cpp

# or using mpic++ (as seen in the workspace):
mpic++ -O3 -o omp_hello omp_hello.cpp
```

Running

Set the number of OpenMP threads and run the binary:

```
# keep clang but install libomp and brew LLVM, then compile linking libomp
clang++ -Xpreprocessor -fopenmp -I$(brew --prefix libomp)/include \
  -L$(brew --prefix libomp)/lib -lomp -O3 -o omp_hello omp_hello.cpp
export OMP_NUM_THREADS=4
./omp_hello
```

If you compiled and plan to run under a job scheduler (example `SLURM`) or call with an argument (as `testjob.sh` does):

```
# example: run with 8 threads (some binaries accept a numeric arg to set threads)
./omp_hello 8

# with mpirun (only if you built with MPI wrapper and want to use mpirun)
mpirun -np 1 ./omp_hello
```

Files in this directory
- `omp_hello.cpp`: Hello-world style OpenMP example showing basic parallel region and thread IDs.
- `omp_barrier.cpp`: Demonstrates use of `#pragma omp barrier` to synchronize threads.
- `omp_critical.cpp`: Shows `#pragma omp critical` usage to protect shared updates.
- `omp_if.cpp`: Example using the `if` clause on OpenMP pragmas to conditionally run parallel regions.
- `omp_sharing1.cpp` and `omp_sharing2.cpp`: Illustrate variable sharing attributes (private, shared, firstprivate, etc.).
- `omp_vecadd.cpp`: Vector addition example demonstrating a simple data-parallel loop.
- `omp_workpool.cpp`: Work-pool (dynamic scheduling) style example using `schedule` clauses.
- `testjob.sh`: Example SLURM batch script that loads a GCC module and runs `./omp_hello 8` (adjust email/module/paths as needed).

Notes
- Some demos accept a runtime numeric argument to control thread count — check the source for exact behavior.
- If you encounter compilation errors, ensure your compiler supports OpenMP and that you pass the correct flags (`-fopenmp` for GCC/Clang).

License
- No license specified — add one if you plan to redistribute or publish these demos.

Enjoy experimenting with OpenMP!
