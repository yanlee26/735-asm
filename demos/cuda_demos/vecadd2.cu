/*
 * vecadd2.cu
 *
 * Same computation as vecadd1.cu (large vector add, multiple blocks,
 * global thread index formula), but using CUDA Unified Memory instead of
 * the explicit malloc / cudaMalloc / cudaMemcpy dance.
 *
 * cudaMallocManaged() allocates memory that is visible to BOTH the host
 * and the device, using a single pointer. The CUDA runtime automatically
 * migrates pages between host and device as needed - you no longer write
 * cudaMemcpy calls yourself. This is simpler to write and reason about,
 * though for performance-critical code explicit memory management (as in
 * vecadd1.cu) can sometimes give more control over when transfers happen.
 *
 * Compare the amount of boilerplate here to vecadd1.cu - this is the main
 * teaching point of this example.
 */

#include <cstdio>

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

    // A single set of pointers, usable directly on both host and device.
    float *a, *b, *c;
    cudaMallocManaged(&a, size);
    cudaMallocManaged(&b, size);
    cudaMallocManaged(&c, size);

    // Fill the inputs directly - no separate host arrays, no explicit
    // copy to the device required.
    for (int i = 0; i < N; i++) {
        a[i] = 1.0f;
        b[i] = 2.0f;
    }

    int numBlocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    printf("N = %d, threadsPerBlock = %d, numBlocks = %d, total threads = %d\n",
           N, THREADS_PER_BLOCK, numBlocks, numBlocks * THREADS_PER_BLOCK);

    vecAddKernel<<<numBlocks, THREADS_PER_BLOCK>>>(a, b, c, N);

    // With unified memory we still need to wait for the kernel to finish
    // before the host touches the result - cudaDeviceSynchronize() (or any
    // other synchronizing call) triggers the migration of pages back to
    // the host as they are accessed below.
    cudaDeviceSynchronize();

    // Verify: every element should be 1.0 + 2.0 = 3.0.
    bool ok = true;
    for (int i = 0; i < N; i++) {
        if (c[i] != 3.0f) {
            ok = false;
            printf("Mismatch at i=%d: got %f, expected 3.0\n", i, c[i]);
            break;
        }
    }
    printf(ok ? "Result verified: all elements correct.\n"
              : "Result INCORRECT.\n");

    // A single cudaFree per allocation - no separate host free() needed.
    cudaFree(a);
    cudaFree(b);
    cudaFree(c);

    return 0;
}
