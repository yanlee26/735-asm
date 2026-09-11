/*
 * vecadd1.cu
 *
 * A "real" vector add: the vectors are large enough that they no longer
 * fit in a single block (a block is limited to at most 1024 threads on
 * current hardware), so we need multiple blocks. This introduces the
 * standard CUDA global thread index formula:
 *
 *     i = blockIdx.x * blockDim.x + threadIdx.x
 *
 * blockDim.x  - number of threads per block (fixed, chosen by us)
 * blockIdx.x  - which block this thread belongs to (0 .. numBlocks-1)
 * threadIdx.x - thread's index within its own block (0 .. blockDim.x-1)
 *
 * Because the total number of threads (numBlocks * threadsPerBlock) may
 * be slightly more than N, every thread must check "if (i < N)" before
 * touching the arrays, otherwise threads with i >= N would read/write
 * out of bounds.
 *
 * Memory management here is the classic explicit style: separate host
 * and device allocations, with manual cudaMemcpy in each direction.
 */

#include <cstdio>
#include <cstdlib>

#define N (1 << 20)          // ~1 million elements
#define THREADS_PER_BLOCK 256

__global__ void vecAddKernel(const float *a, const float *b, float *c, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main()
{
    size_t size = N * sizeof(float);

    // Host arrays.
    float *h_a = (float *)malloc(size);
    float *h_b = (float *)malloc(size);
    float *h_c = (float *)malloc(size);

    for (int i = 0; i < N; i++) {
        h_a[i] = 1.0f;
        h_b[i] = 2.0f;
    }

    // Device arrays.
    float *d_a, *d_b, *d_c;
    cudaMalloc((void **)&d_a, size);
    cudaMalloc((void **)&d_b, size);
    cudaMalloc((void **)&d_c, size);

    cudaMemcpy(d_a, h_a, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, size, cudaMemcpyHostToDevice);

    // Enough blocks to cover all N elements, rounding up.
    int numBlocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    printf("N = %d, threadsPerBlock = %d, numBlocks = %d, total threads = %d\n",
           N, THREADS_PER_BLOCK, numBlocks, numBlocks * THREADS_PER_BLOCK);

    vecAddKernel<<<numBlocks, THREADS_PER_BLOCK>>>(d_a, d_b, d_c, N);
    cudaDeviceSynchronize();

    cudaMemcpy(h_c, d_c, size, cudaMemcpyDeviceToHost);

    // Verify: every element should be 1.0 + 2.0 = 3.0.
    bool ok = true;
    for (int i = 0; i < N; i++) {
        if (h_c[i] != 3.0f) {
            ok = false;
            printf("Mismatch at i=%d: got %f, expected 3.0\n", i, h_c[i]);
            break;
        }
    }
    printf(ok ? "Result verified: all elements correct.\n"
              : "Result INCORRECT.\n");

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
    free(h_a);
    free(h_b);
    free(h_c);

    return 0;
}
