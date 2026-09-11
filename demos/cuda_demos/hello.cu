/*
 * hello.cu
 *
 * The simplest possible CUDA program. The kernel itself does nothing useful -
 * the point of this example is just to see a kernel launch succeed and to
 * get comfortable with the <<<...>>> launch syntax, nvcc, and running a
 * CUDA binary on the GPU node.
 *
 * Terminology:
 *   host   - the CPU and its memory
 *   device - the GPU and its memory
 *   kernel - a function that runs on the device, launched from the host
 */

#include <cstdio>

// __global__ marks a function as a kernel: called from the host, runs on
// the device. This kernel takes no arguments and does no work - it exists
// purely so we have something to launch.
__global__ void helloKernel()
{
    // In a real kernel you'd normally do something per-thread here, e.g.
    // print the thread's index. printf from device code is supported on
    // modern GPUs (compute capability >= 2.0) and is handy for debugging.
    printf("Hello from the device! threadIdx.x = %d, blockIdx.x = %d\n",
           threadIdx.x, blockIdx.x);
}

int main()
{
    printf("Hello from the host - about to launch the kernel.\n");

    // Launch configuration <<<numBlocks, threadsPerBlock>>>.
    // Here: 1 block of 8 threads, so 8 threads total will each run the
    // kernel body and each print their own threadIdx.x.
    helloKernel<<<1, 8>>>();

    // Kernel launches are asynchronous - the host carries on immediately.
    // cudaDeviceSynchronize() blocks the host until the device has
    // finished, so that we actually see the printf output before main()
    // exits, and so we can catch any launch/runtime errors.
    cudaError_t err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err));
        return 1;
    }

    printf("Hello from the host - kernel finished.\n");
    return 0;
}
