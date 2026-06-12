/**
 * ========================================================================
 * ENGINE 1: EVOLVED BASE-32 CLUSTER-TREE SORT (JAVASCRIPT RUNTIME)
 * ========================================================================
 * * Architectural Overview:
 * This sorting engine optimizes performance by aligning nested bucketing passes 
 * with 32-way data structures, minimizing random branching. Instead of using 
 * traditional base-10 radix groupings, it splits values using a 5-bit stride (2^5 = 32),
 * mirroring the lane allocation of an NVIDIA GPU warp.
 * * Target Audience Notes:
 * - Excellent for compute-bound operations where cache-friendly data alignment is needed.
 * - Handles negative thresholds, uniform/skewed intervals, and tracks operations transparently.
 */
class Base32ClusterTreeEngine {
    /**
     * Initializes the Base-32 tree layout parameters.
     */
    constructor() {
        this.bucketCount = 32;       // Aligned directly to 32-lane GPU hardware footprints
        this.bitStride = 5;          // 5 bits required to index 32 distinct bucket positions (2^5)
        this.bitMask = 0x1F;         // Bitmask (binary: 0001 1111) to isolate the active 5-bit segment
    }

    /**
     * Public entry-point to execute the sorting pipeline.
     * * @param {Array<number>} dataArray - The raw unsorted sequence (integers or floating points).
     * @param {Object} externalTracker - Metrics container tracking simulated operational steps.
     * @returns {Array<number>} The fully sorted, reconstructed array.
     */
    executeSort(dataArray, externalTracker) {
        if (!dataArray || dataArray.length <= 1) return dataArray;

        // Establish boundaries to calculate structural offsets
        let minimumValue = dataArray[0];
        let maximumValue = dataArray[0];

        for (let i = 1; i < dataArray.length; i++) {
            if (dataArray[i] < minimumValue) minimumValue = dataArray[i];
            if (dataArray[i] > maximumValue) maximumValue = dataArray[i];
        }

        // Edge case: If all elements are equal, array is sorted.
        if (minimumValue === maximumValue) return dataArray;

        // Calculate the highest bit position required to represent the delta range
        const valueDelta = maximumValue - minimumValue;
        const highestBitOffset = Math.floor(Math.log2(valueDelta));
        
        // Align the starting bit shift to a multiple of our 5-bit stride
        const initialShiftValue = Math.floor(highestBitOffset / this.bitStride) * this.bitStride;

        // Initiate the recursive routing pass
        return this.processClusterPass(dataArray, minimumValue, initialShiftValue, externalTracker);
    }

    /**
     * Recursive routing loop processing bitmask distributions.
     * * @private
     */
    processClusterPass(currentBlock, baselineMin, activeShift, metricsTracker) {
        // Fallback safety boundary for atomic chunks
        if (currentBlock.length <= 8) {
            metricsTracker.bitshifts += currentBlock.length;
            return currentBlock.sort((a, b) => a - b);
        }

        // Initialize 32 local registers (one for each virtual warp lane)
        const registerBuckets = Array.from({ length: this.bucketCount }, () => []);
        metricsTracker.bitshifts += this.bucketCount;

        // Ingestion Stage: Shift values and mask bits to extract the bucket index (0-31)
        for (let i = 0; i < currentBlock.length; i++) {
            const dataPoint = currentBlock[i];
            // Normalize current point relative to the local baseline floor
            const alignmentOffset = Math.floor(dataPoint - baselineMin);
            
            // Core Bitwise Operation: Shift down to active bit window, mask away everything except lowest 5 bits
            const targetBucketIndex = (alignmentOffset >> activeShift) & this.bitMask;

            registerBuckets[targetBucketIndex].push(dataPoint);
            metricsTracker.bitshifts += 3; // Tracks metrics for normalizations, shifts, and masks
        }

        // Aggregation Stage: Re-traverse buckets and cascade further down or sort atomically
        let aggregatedOutput = [];
        for (let bucketIndex = 0; bucketIndex < this.bucketCount; bucketIndex++) {
            const targetBucket = registerBuckets[bucketIndex];
            if (targetBucket.length === 0) continue;

            let fullyResolvedSubBlock;
            // If there are remaining bit-strides left to evaluate, descend a tier recursively
            if (activeShift >= this.bitStride) {
                fullyResolvedSubBlock = this.processClusterPass(
                    targetBucket, 
                    baselineMin, 
                    activeShift - this.bitStride, 
                    metricsTracker
                );
            } else {
                // Leaf Node Resolution: Final fine-grained sorting at base layer
                fullyResolvedSubBlock = targetBucket.sort((a, b) => a - b);
                metricsTracker.bitshifts += targetBucket.length;
            }

            // Append the resolved sequence using a fast, sequentially localized traversal loop
            for (let k = 0; k < fullyResolvedSubBlock.length; k++) {
                aggregatedOutput.push(fullyResolvedSubBlock[k]);
            }
            metricsTracker.bitshifts += fullyResolvedSubBlock.length;
        }

        return aggregatedOutput;
    }
}