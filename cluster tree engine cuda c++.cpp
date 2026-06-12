/**
 * ========================================================================
 * ENGINE 2: 32-CORE HARDWARE-ACCELERATED CUDA WARP ENGINE (NATIVE BINARY)
 * ========================================================================
 * * Architectural Overview:
 * This C++/CUDA driver represents how the Base-32 Cluster engine operates natively
 * inside hardware silicon. By matching the sub-group size to exactly 32 threads, 
 * execution maps directly onto an NVIDIA hardware warp, allowing threads to talk 
 * directly to each other via internal register registers using intrinsic voting macros.
 * * Dev-Team Performance Notes:
 * - Eliminates execution branching (Warp Divergence) because all lanes evaluate the matching mask in sync.
 * - Utilizes high-speed `__shared__` memory allocations rather than accessing global RAM channels.
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>

// Defined configuration constants matching hardware specifications
#define WARP_SIZE 32
#define BUCKET_COUNT 32
#define SHARED_MEM_LIMIT 2048

/**
 * CUDA Device Kernel: Executes a synchronized, branch-free array ingestion pass.
 * * @param d_input      - Global memory pointer referencing the raw unsorted device array.
 * @param d_output     - Global memory pointer designating where the re-ordered elements are placed.
 * @param totalRecords - Count of array elements currently undergoing hardware processing.
 * @param bitShift     - Active shifting delta specifying the current 5-bit evaluation window.
 * @param baselineFloor- Minimum offset boundary utilized to normalize float or integer sets.
 */
__global__ void cudaBase32WarpKernel(const float* d_input, float* d_output, int totalRecords, int bitShift, float baselineFloor) {
    // Determine the unique global execution lane for this thread
    int globalThreadId = blockIdx.x * blockDim.x + threadId.x;
    // Track localized lane indexing within the bounds of a specific 32-core warp
    int warpLaneId = threadIdx.x & 0x1F; 

    // Dynamically allocate high-speed shared memory storage local to the current block
    __shared__ float localWarpScratchpad[SHARED_MEM_LIMIT];
    // Dynamic execution tracking registry initialized for every thread lane
    __shared__ unsigned int globalLaneScatterIndices[BUCKET_COUNT];

    // Thread check against processing bounds
    if (globalThreadId >= totalRecords) return;

    // Phase 1: Coalesced memory data ingest from global DRAM into registers
    float localizedTargetElement = d_input[globalThreadId];
    
    // Phase 2: Compute target bucket index via fast hardware bit-manipulations
    int offsetIntRepresentation = __float2int_rd(localizedTargetElement - baselineFloor);
    // Extract exact 5-bit slot index via arithmetic shift right and a 0x1F mask (binary 11111)
    int targetBucketIndex = (offsetIntRepresentation >> bitShift) & 0x1F;

    // Phase 3: Hardware Ballot synchronization to map data distributions across lanes without branching
    // Every lane assesses its bucket index against the current loop tier.
    for (int currentBucketTier = 0; currentBucketTier < BUCKET_COUNT; currentBucketTier++) {
        // Evaluates which warp threads are targeting the active bucket slot simultaneously
        unsigned int dynamicWarpBallotMask = __ballot_sync(0xffffffff, targetBucketIndex == currentBucketTier);
        
        // If thread matches the active group, calculate its memory scatter displacement 
        if (targetBucketIndex == currentBucketTier) {
            // Count how many matching flags exist ahead of this thread in the warp register
            int precedingMatchCount = __popc(dynamicWarpBallotMask & ((1 << warpLaneId) - 1));
            
            // Warp Lane 0 coordinates base offsets and pushes data blocks into shared memory channels
            if (precedingMatchCount == 0) {
                // Atomically reserve space within shared scratchpad arrays
                globalLaneScatterIndices[currentBucketTier] = atomicAdd(&globalLaneScatterIndices[currentBucketTier], __popc(dynamicWarpBallotMask));
            }
            
            // Synchronize execution paths across all active threads in the block
            __syncthreads();

            // Direct register-to-shared memory routing loop
            int finalizedWriteOffset = globalLaneScatterIndices[currentBucketTier] + precedingMatchCount;
            if (finalizedWriteOffset < SHARED_MEM_LIMIT) {
                localWarpScratchpad[finalizedWriteOffset] = localizedTargetElement;
            }
        }
    }

    // Phase 4: Synchronize threads and flush localized cache blocks back out to system memory
    __syncthreads();
    d_output[globalThreadId] = localWarpScratchpad[threadIdx.x];
}

/**
 * Host-Side Execution Wrapper: Sets up grid sizing and schedules execution grids.
 */
extern "C" void launchCudaWarpEngine(const float* hostInput, float* hostOutput, int elementCount, int currentBitShift, float calculatedMin) {
    float *deviceInputPtr = nullptr;
    float *deviceOutputPtr = nullptr;
    size_t allocationSize = elementCount * sizeof(float);

    // Allocate isolated device memory slots
    cudaMalloc((void**)&deviceInputPtr, allocationSize);
    cudaMalloc((void**)&deviceOutputPtr, allocationSize);

    // Push source array from host RAM down to GPU device memory
    cudaMemcpy(deviceInputPtr, hostInput, allocationSize, cudaMemcpyHostToDevice);

    // Grid sizing calculation: round up blocks to fill discrete 32-thread hardware units
    int executionBlockSize = 256; // 8 active warps per block (8 * 32 = 256)
    int executionGridSize = (elementCount + executionBlockSize - 1) / executionBlockSize;

    // Execute the kernel on the GPU device
    cudaBase32WarpKernel<<<executionGridSize, executionBlockSize>>>(deviceInputPtr, deviceOutputPtr, elementCount, currentBitShift, calculatedMin);

    // Blocks execution flow until device threads complete processing loops
    cudaDeviceSynchronize();

    // Pull sorted results from GPU device memory back to host space
    cudaMemcpy(hostOutput, deviceOutputPtr, allocationSize, cudaMemcpyDeviceToHost);

    // Clean up device memory resources
    cudaFree(deviceInputPtr);
    cudaFree(deviceOutputPtr);
}