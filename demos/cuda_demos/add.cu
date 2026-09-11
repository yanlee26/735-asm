/*
 * add.cu
 *
 * The next step up from hello.cu: a kernel that actually does some work -
 * adding two integers together - but only a single value, on a single
 * thread. The point here is to see the full round trip of:
 *
 *   1. allocate memory on the device (cudaMalloc)
 *   2. copy input data from host to device (cudaMemcpy ... HostToDevice)
 *   3. run a kernel on the device
 *   4. copy the result back from device to host (cudaMemcpy ... DeviceToHost)
 *   5. free device memory (cudaFree)
 */

#include <cstdio>

// Kernel takes device pointers - a, b, and c all point to device memory.
// Since we're only using a single thread, there's no indexing to worry
// about yet (that comes in arradd.cu / vecadd*.cu).
__global__ void addKernel(int *a, int *b, int *c)
{
    *c = *a + *b;
}

int main()
{
    int a = 5, b = 7, c = 0;   // host copies
    int *d_a, *d_b, *d_c;      // device pointers (convention: d_ prefix)

    // Allocate space for a single int on the device for each variable.
    cudaMalloc((void **)&d_a, sizeof(int));
    cudaMalloc((void **)&d_b, sizeof(int));
    cudaMalloc((void **)&d_c, sizeof(int));

    // Copy the inputs from host memory to device memory.
    cudaMemcpy(d_a, &a, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, &b, sizeof(int), cudaMemcpyHostToDevice);

    // Launch with a single block of a single thread - only one addition
    // needs to happen.
    addKernel<<<1, 1>>>(d_a, d_b, d_c);
    cudaDeviceSynchronize();

    // Copy the result back from device memory to host memory.
    cudaMemcpy(&c, d_c, sizeof(int), cudaMemcpyDeviceToHost);

    printf("%d + %d = %d (computed on the device)\n", a, b, c);

    // Release device memory.
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);

    return 0;
}
