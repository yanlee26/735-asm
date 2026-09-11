/*
 * arradd.cu
 *
 * Extends add.cu from a single value to a small fixed-size array. This is
 * where per-thread indexing shows up for the first time: each thread
 * computes exactly one element of the output array, identified by
 * threadIdx.x.
 *
 * Because the whole array fits comfortably in one block (N is small),
 * we only need a single block of N threads - no blockIdx arithmetic yet.
 * That generalisation comes in vecadd1.cu / vecadd2.cu.
 */

#include <cstdio>

#define N 8

__global__ void arrAddKernel(int *a, int *b, int *c)
{
    // Each of the N threads in the block handles one array element.
    // threadIdx.x ranges from 0 to N-1 for a 1D block of N threads.
    int i = threadIdx.x;
    c[i] = a[i] + b[i];
}

int main()
{
    int a[N], b[N], c[N];
    int *d_a, *d_b, *d_c;
    size_t size = N * sizeof(int);

    // Fill the input arrays with some test data.
    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i * 10;
    }

    cudaMalloc((void **)&d_a, size);
    cudaMalloc((void **)&d_b, size);
    cudaMalloc((void **)&d_c, size);

    cudaMemcpy(d_a, a, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, size, cudaMemcpyHostToDevice);

    // One block, N threads - one thread per array element.
    arrAddKernel<<<1, N>>>(d_a, d_b, d_c);
    cudaDeviceSynchronize();

    cudaMemcpy(c, d_c, size, cudaMemcpyDeviceToHost);

    printf("Array addition (1 block, %d threads):\n", N);
    for (int i = 0; i < N; i++) {
        printf("  a[%d] + b[%d] = %d + %d = %d\n", i, i, a[i], b[i], c[i]);
    }

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);

    return 0;
}
