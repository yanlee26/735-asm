# MPI Demos — 159.735 Studies in Parallel and Distributed Systems

Small, self-contained MPI example programs used for teaching the basics of
message passing: point-to-point communication, collective communication
(broadcast, scatter, gather, all-to-all), and a sequential sort routine used
as the basis for a parallel bucket-sort exercise. Most demos ship in both C
and C++ versions with equivalent logic.

## Files

| File | Description |
|---|---|
| `first.c` / `first.cpp` | Simplest two-node example: master sends `N` to a slave node, each computes a partial sum, master combines the results with `MPI_Send`/`MPI_Recv`. |
| `second.c` / `second.cpp` | Extends `first` to an arbitrary number of nodes: master sends `N` to every slave process and gathers partial sums back. |
| `broadcast.c` / `broadcast.cpp` | Same partial-sum problem, using `MPI_Bcast` to distribute `N` and `MPI_Reduce` to combine results instead of manual send/receive loops. |
| `scatter.c` / `scatter.cpp` | Demonstrates `MPI_Scatter`: root splits an array and distributes equal chunks to every process. |
| `gather.c` / `gather.cpp` | Demonstrates `MPI_Gather`: every process contributes data that is collected back at the root. |
| `alltoall.c` / `alltoall.cpp` | Demonstrates `MPI_Alltoall`: every process exchanges distinct data with every other process. |
| `commscan.cpp` | Basic example of send/receive from process 0 to every other process in `MPI_COMM_WORLD`. |
| `bucket.c` | Sequential (non-MPI) bucket-sort reference implementation, useful as a baseline before parallelizing. |
| `hello.c` | Trivial "Hello, World!" C program (not MPI). |
| `testjob.sh` | Example SLURM batch script for submitting an MPI job (`first`) to a cluster. |

## Requirements

- An MPI implementation (e.g. OpenMPI or MPICH) providing `mpicc` / `mpicxx` and `mpirun`/`mpiexec`.
- A C/C++ compiler (`gcc`/`g++`) for the non-MPI programs (`bucket.c`, `hello.c`).

## Building

Compile any MPI demo with `mpicc` (C) or `mpicxx` (C++):

```bash
export MPICH_CXX=clang++ # note required
mpicc -o first first.c
mpicxx -o first first.cpp
mpicxx -o gamma_mc gamma_mc.cpp 
```

Repeat for the other `.c`/`.cpp` files, substituting the desired output name.

Non-MPI programs use a plain compiler:

```bash
gcc -o bucket -lm bucket.c
gcc -o hello hello.c

```

## Running

Run any compiled MPI demo with `mpirun`/`mpiexec`, specifying the number of
processes with `-n`. Several programs also take a numeric argument `N`:

```bash
mpirun -n 2 ./first 10000
mpirun -n 4 ./second 10000
mpirun -n 4 ./broadcast 10000
mpirun -n 4 ./scatter
mpirun -n 4 ./gather
mpirun -n 4 ./alltoall
mpirun -n 4 ./commscan
mpirun -n 4 ./gamma_mc 1000
```

## Running on a SLURM cluster

`testjob.sh` is a sample SLURM submission script that loads an MPI module and
launches the `first` executable. Adjust `--mail-user`, the `module load`
line, and the `mpiexec` command for your own build before submitting:

```bash
sbatch testjob.sh
```

## Credits

Demo source files are adapted from course material for 159.735 by Ian Bond.
