/*
  159.735 Assignment 2: Parallel Bucket Sort using MPI collective
  communication routines with VARIABLE buffer sizes (Scatterv,
  Alltoallv, Gatherv).

  Strategy ("small bucket / large bucket" approach, as covered in the
  "Embarrassingly parallel, partitioning, and divide-and-conquer" slides):

    1. Master generates N random numbers in [0, MAXVAL).
    2. Master partitions the array into numproc (roughly) equal-sized
       chunks and distributes them with MPI_Scatterv. This first
       partition is by QUANTITY (not by value), so it is inherently
       load-balanced regardless of how the random numbers are
       distributed in value-space.
    3. Each process classifies its local chunk into numproc "small
       buckets" based on value range (bucket i covers
       [i*MAXVAL/numproc, (i+1)*MAXVAL/numproc) ).
    4. MPI_Alltoallv redistributes the small buckets so that process i
       ends up with the "large bucket" containing ALL values in its
       value range, gathered from every process.
    5. Each process sorts its own large bucket with qsort().
    6. MPI_Gatherv collects the sorted large buckets back at the
       master, in rank order, giving the final fully-sorted array.

  Usage:
      mpirun -np <p> ./bucket_sort <N>
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "mpi.h"

typedef long DATA_T;
const DATA_T MAXVAL = 1000000000L; // range of random numbers: [0, MAXVAL)

int compare_data(const void* a, const void* b)
{
    DATA_T x = *(const DATA_T*)a;
    DATA_T y = *(const DATA_T*)b;
    return (x > y) - (x < y);
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int numproc, myid;
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (argc < 2) {
        if (myid == 0) {
            fprintf(stderr, "Usage: mpirun -np <p> %s <N>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    long N = atol(argv[1]);

    double t0 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Step 1: master generates the full array of random numbers.
    // Only the master initializes the array, as required.
    // ------------------------------------------------------------------
    std::vector<DATA_T> full_array;
    if (myid == 0) {
        full_array.resize(N);
        srand(12345); // fixed seed for reproducibility when testing
        for (long i = 0; i < N; ++i) {
            full_array[i] = rand() % MAXVAL;
        }
    }

    // ------------------------------------------------------------------
    // Step 2: master partitions the array into numproc chunks (by
    // QUANTITY, not by value) and scatters them with MPI_Scatterv.
    // Using Scatterv (not Scatter) correctly handles N not being
    // exactly divisible by numproc.
    // ------------------------------------------------------------------
    std::vector<int> send_counts(numproc), displs(numproc);
    if (myid == 0) {
        long base = N / numproc;
        long remainder = N % numproc;
        long offset = 0;
        for (int p = 0; p < numproc; ++p) {
            long count = base + (p < remainder ? 1 : 0);
            send_counts[p] = (int)count;
            displs[p] = (int)offset;
            offset += count;
        }
    }

    int my_count;
    MPI_Scatter(send_counts.data(), 1, MPI_INT, &my_count, 1, MPI_INT,
                0, MPI_COMM_WORLD);

    std::vector<DATA_T> local_data(my_count);
    MPI_Scatterv(myid == 0 ? full_array.data() : nullptr,
                 myid == 0 ? send_counts.data() : nullptr,
                 myid == 0 ? displs.data() : nullptr,
                 MPI_LONG,
                 local_data.data(), my_count, MPI_LONG,
                 0, MPI_COMM_WORLD);

    double t1 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Step 3: classify local data into numproc "small buckets" based
    // on value range. Every process maintains buckets covering the
    // FULL range of possible numbers, as required.
    // ------------------------------------------------------------------
    DATA_T bucket_width = MAXVAL / numproc;
    std::vector<std::vector<DATA_T> > small_buckets(numproc);

    for (int i = 0; i < my_count; ++i) {
        int b = (int)(local_data[i] / bucket_width);
        if (b >= numproc) b = numproc - 1; // guard against the top edge value
        small_buckets[b].push_back(local_data[i]);
    }

    double t2 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Step 4: MPI_Alltoallv redistributes small buckets into large
    // buckets. Process i receives everyone's small_buckets[i], i.e.
    // ALL values across the whole array that fall in value-range i.
    // Alltoallv (not Alltoall) is required here because the number of
    // values going to each destination varies and is only known at
    // runtime.
    // ------------------------------------------------------------------
    std::vector<int> sendcounts(numproc), recvcounts(numproc);
    std::vector<int> sdispls(numproc), rdispls(numproc);

    for (int p = 0; p < numproc; ++p) {
        sendcounts[p] = (int)small_buckets[p].size();
    }

    // First exchange just the COUNTS, so every process knows how much
    // data it is about to receive from every other process.
    MPI_Alltoall(sendcounts.data(), 1, MPI_INT,
                 recvcounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

    // Flatten the small buckets into one contiguous send buffer, in
    // rank order, and compute the corresponding send displacements.
    std::vector<DATA_T> send_buffer;
    send_buffer.reserve(my_count);
    sdispls[0] = 0;
    for (int p = 0; p < numproc; ++p) {
        if (p > 0) sdispls[p] = sdispls[p - 1] + sendcounts[p - 1];
        send_buffer.insert(send_buffer.end(),
                            small_buckets[p].begin(), small_buckets[p].end());
    }

    // Compute receive displacements and the total size of this
    // process's "large bucket".
    rdispls[0] = 0;
    int total_recv = recvcounts[0];
    for (int p = 1; p < numproc; ++p) {
        rdispls[p] = rdispls[p - 1] + recvcounts[p - 1];
        total_recv += recvcounts[p];
    }

    std::vector<DATA_T> large_bucket(total_recv);
    MPI_Alltoallv(send_buffer.data(), sendcounts.data(), sdispls.data(), MPI_LONG,
                  large_bucket.data(), recvcounts.data(), rdispls.data(), MPI_LONG,
                  MPI_COMM_WORLD);

    double t3 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Step 5: sort the large bucket locally. As permitted by the
    // assignment sheet, we use the standard library qsort() rather
    // than implementing our own sequential sort.
    // ------------------------------------------------------------------
    qsort(large_bucket.data(), large_bucket.size(), sizeof(DATA_T), compare_data);

    double t4 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Step 6: gather the sorted large buckets back to the master, in
    // rank order. Because bucket sizes vary (depending on the value
    // distribution), MPI_Gatherv is required here rather than
    // MPI_Gather.
    // ------------------------------------------------------------------
    int my_sorted_count = (int)large_bucket.size();
    std::vector<int> gather_counts(numproc), gather_displs(numproc);
    MPI_Gather(&my_sorted_count, 1, MPI_INT,
               gather_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<DATA_T> sorted_array;
    if (myid == 0) {
        gather_displs[0] = 0;
        for (int p = 1; p < numproc; ++p) {
            gather_displs[p] = gather_displs[p - 1] + gather_counts[p - 1];
        }
        sorted_array.resize(N);
    }

    MPI_Gatherv(large_bucket.data(), my_sorted_count, MPI_LONG,
                myid == 0 ? sorted_array.data() : nullptr,
                myid == 0 ? gather_counts.data() : nullptr,
                myid == 0 ? gather_displs.data() : nullptr,
                MPI_LONG, 0, MPI_COMM_WORLD);

    double t5 = MPI_Wtime();

    // ------------------------------------------------------------------
    // Master: verify correctness and report timing breakdown for each
    // phase (useful for the performance report / Gustafson's Law
    // analysis required in the assignment).
    // ------------------------------------------------------------------
    if (myid == 0) {
        bool sorted_ok = true;
        for (long i = 1; i < N; ++i) {
            if (sorted_array[i - 1] > sorted_array[i]) { sorted_ok = false; break; }
        }

        double scatter_time   = t1 - t0;
        double classify_time  = t2 - t1;
        double alltoall_time  = t3 - t2;
        double sort_time      = t4 - t3;
        double gather_time    = t5 - t4;
        double total_time     = t5 - t0;

        printf("=====================================\n");
        printf("N = %ld, numproc = %d\n", N, numproc);
        printf("Sorted correctly : %s\n", sorted_ok ? "YES" : "NO");
        printf("Scatter time     = %f s\n", scatter_time);
        printf("Classify time    = %f s\n", classify_time);
        printf("Alltoallv time   = %f s\n", alltoall_time);
        printf("Sort time        = %f s\n", sort_time);
        printf("Gatherv time     = %f s\n", gather_time);
        printf("Total time       = %f s\n", total_time);
        printf("=====================================\n");
    }

    MPI_Finalize();
    return 0;
}
