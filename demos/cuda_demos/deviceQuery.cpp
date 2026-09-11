/*
 * deviceQuery.cpp
 *
 * Queries the CUDA runtime for every GPU visible on this machine and
 * prints out its properties: name, compute capability, memory sizes,
 * core/clock information, and various architectural limits (max threads
 * per block, warp size, shared memory per block, etc).
 *
 * This is useful for two things:
 *   1. Confirming the CUDA runtime can actually see and talk to the GPU(s).
 *   2. Knowing the hardware limits you're programming against - e.g. the
 *      maximum number of threads per block affects how you choose launch
 *      configurations in kernels like vecadd1.cu / vecadd2.cu.
 *
 * Despite the .cpp extension this file uses the CUDA runtime API
 * (cuda_runtime.h), so it must be compiled with nvcc, not a plain C++
 * compiler - nvcc knows how to hand off host-only code like this to the
 * regular host compiler while still linking against the CUDA runtime.
 */

#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>

// Small helper to convert a cudaError_t into a fatal error message.
// Device queries are host-side API calls, so ordinary error checking
// (rather than kernel-launch error checking) is enough here.
static void checkCuda(cudaError_t err, const char *what)
{
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA error during %s: %s\n", what, cudaGetErrorString(err));
        exit(1);
    }
}

int main()
{
    int deviceCount = 0;
    checkCuda(cudaGetDeviceCount(&deviceCount), "cudaGetDeviceCount");

    printf("Detected %d CUDA capable device(s)\n\n", deviceCount);

    if (deviceCount == 0) {
        printf("No CUDA devices found - check drivers and that you are on a GPU node.\n");
        return 0;
    }

    // Also report the CUDA runtime and driver versions, since mismatches
    // between the two are a common source of confusion.
    int runtimeVersion = 0, driverVersion = 0;
    cudaRuntimeGetVersion(&runtimeVersion);
    cudaDriverGetVersion(&driverVersion);
    printf("CUDA Driver Version:  %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10);
    printf("CUDA Runtime Version: %d.%d\n\n", runtimeVersion / 1000, (runtimeVersion % 100) / 10);

    for (int dev = 0; dev < deviceCount; dev++) {
        cudaDeviceProp prop;
        checkCuda(cudaGetDeviceProperties(&prop, dev), "cudaGetDeviceProperties");

        printf("Device %d: \"%s\"\n", dev, prop.name);
        printf("  Compute capability:                          %d.%d\n",
               prop.major, prop.minor);
        printf("  Total global memory:                         %.2f GB (%zu bytes)\n",
               (double)prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0),
               prop.totalGlobalMem);
        printf("  Total constant memory:                       %zu bytes\n",
               prop.totalConstMem);
        printf("  Shared memory per block:                     %zu bytes\n",
               prop.sharedMemPerBlock);
        printf("  Registers available per block:                %d\n",
               prop.regsPerBlock);
        printf("  Warp size:                                   %d\n", prop.warpSize);
        printf("  Max threads per block:                       %d\n",
               prop.maxThreadsPerBlock);
        printf("  Max block dimensions:                        [%d, %d, %d]\n",
               prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
        printf("  Max grid dimensions:                         [%d, %d, %d]\n",
               prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
        printf("  Number of multiprocessors (SMs):             %d\n",
               prop.multiProcessorCount);
        printf("  Clock rate:                                  %.2f GHz\n",
               prop.clockRate / 1.0e6);
        printf("  Memory clock rate:                            %.2f GHz\n",
               prop.memoryClockRate / 1.0e6);
        printf("  Memory bus width:                             %d bits\n",
               prop.memoryBusWidth);
        printf("  L2 cache size:                                %d bytes\n",
               prop.l2CacheSize);
        printf("  ECC enabled:                                  %s\n",
               prop.ECCEnabled ? "yes" : "no");
        printf("  Unified addressing supported:                 %s\n",
               prop.unifiedAddressing ? "yes" : "no");
        printf("  Managed memory supported:                     %s\n",
               prop.managedMemory ? "yes" : "no");
        printf("  Concurrent kernels supported:                 %s\n",
               prop.concurrentKernels ? "yes" : "no");
        printf("  Async engine count (concurrent copy/exec):    %d\n",
               prop.asyncEngineCount);
        printf("  PCI bus / device ID:                          %d / %d\n",
               prop.pciBusID, prop.pciDeviceID);
        printf("\n");
    }

    return 0;
}