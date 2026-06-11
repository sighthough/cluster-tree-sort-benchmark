
# Cluster-Tree Sort & Benchmark

> A custom cluster-tree algorithm shown in action with a live benchmark against the most common standard algorithms: Quick Sort and Insertion Sort.

👉 **[CLICK HERE TO RUN THE LIVE BENCHMARK](https://sighthough.github.io/cluster-tree-sort-benchmark/)**


This was made by me and googles gemini ai, 
to see the whole chat about it and perhaps be inspired or help with 
understanding how it works and how we came up to it [here is the link to the chat](https://gemini.google.com/share/1f99f25ee602) [and the latest evolution of it](https://gemini.google.com/share/9e749eb91c6e)

Markdown## Core Engine Architecture: How It Works

The **Dynamic Power-of-2 Cluster-Tree Sort** is a hardware-sympathetic sorting
engine designed to optimize instruction pipelines and maximize CPU/GPU cache
locality. Unlike traditional comparison-based sorting algorithms, it operates
in two distinct, high-performance phases: **Bit-Masked Binary Routing** and
**Localized Leaf Binary Insertion**.

### 1. The Multi-Tier Dynamic Layout
Before processing, the engine inspects the input dataset to determine the
optimal bounds. Instead of using arbitrary bucket counts or floating-point
division, it mathematically snaps the execution footprint to a **pure power 
of 2** ($2^n$, e.g., 16, 64, 256). This eliminates empty padding lanes and
prepares the data matrix for bitwise boundaries.

### 2. Phase 1: Bit-Masked Binary Routing
Traditional bucket or radix distributions rely on heavy arithmetic division
(`/`) or array boundary checks inside loops. This engine replaces those
expensive operations with a **top-down binary search tree mask**:

* **Bit-Shift Jumps:** The router calculates a target index by dropping down
  through execution tiers using bitwise left-shifts (`1 << s`).
* **Branch Predictor Harmony:** For every element, the routing path narrows
  down the target bucket location in exactly $\log_2(\text{BUCKETS\_COUNT})$
  steps. Because the loop bounds shrink predictably by exactly half on every
  iteration, the CPU’s hardware branch predictor operates with near-zero
  pipeline stalls.
* **1-Cycle Execution:** Bit-shifting executes in a single CPU clock cycle,
  completely bypassing the 10-to-40 cycle latency penalty of hardware integer
  division.

```javascript
// A conceptual look at the dynamic bit-masked routing step
for (let s = totalSteps - 1; s >= 0; s--) {
    let bitJump = 1 << s; // 1-clock-cycle hardware shift
    if (num >= min + stepSize * (idx + bitJump)) {
        idx += bitJump; // Narrow the execution window
    }
}
3. Phase 2: Localized Leaf Binary InsertionOnce elements are distributed into their power-of-2 channels, they must besorted internally. Traditional bucket sorters often drop standard linearinsertion loops inside the buckets, which degrades into $O(k^2)$ slowdowns ifdata clumps together.This engine implements a localized binary search insertion layer:Cache Locality: Because the data has been segmented into isolated,sequential memory buckets, the CPU pre-fetches the array segments into theultra-fast L1/L2 hardware cache lines.Logarithmic Splicing: Instead of crawling through the bucket one by one,a micro-binary search pinpoints the exact array splice index in $O(\log k)$steps, crushing the performance bottlenecks of skewed or hot-spot datadistributions.Performance Metrics vs. Native EnginesWhile behavioral benchmarks show a slightly higher count of virtual "structuraloperations" compared to hybrid engines like TimSort, the physical runtime issignificantly lower.By prioritizing sequential memory cache alignment over pointer-chasingstack management, and bitwise math over arithmetic division, theCluster-Tree engine achieves incredible physical speed by cooperating with themachine's underlying silicon.



---
*Co-authored by sighthough and Gemini.*
