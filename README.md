
# Cluster-Tree Sort & Benchmark

> A custom cluster-tree algorithm shown in action with a live benchmark against the most common standard algorithms: Quick Sort and Insertion Sort.

👉 **[CLICK HERE TO RUN THE LIVE BENCHMARK]([https://sighthough.github.io/cluster-tree-sort-benchmark/])**

---

## How It Works

Cluster-Tree Sort operates as a two-stage pipeline to achieve highly efficient sorting, approaching $O(n)$ linear time complexity under uniform data distribution:

1. **The Proactive Cluster Stage:** The algorithm analyzes the dataset size ($N$) and range ($Max - Min$). It instantly partitions the data space into $\sqrt{N}$ optimized buckets (clusters) using constant-time math.
2. **The Local Insertion Stage:** As items land in their respective clusters, they are sorted using a **Nearest Neighbor** approach. The algorithm calculates the absolute difference ($|a - b|$) against existing elements to find the closest numerical neighbor and inserts the item precisely to its left or right.

### The "Float" Advantage
Because random floating-point numbers distribute incredibly evenly across continuous space, our pre-calculated clusters remain small and perfectly balanced. This allows the algorithm to bypass the heavy comparison penalties of traditional algorithms—**effectively outperforming Quick Sort on large floating-point datasets.**

---

## Features Built Into This Benchmark

* **Dual Paradigms:** Test the algorithm in **Proactive Mode** (static datasets with known sizes) or **Reactive Mode** (dynamic streams that automatically adjust cluster scales when item thresholds are crossed).
* **Interactive Controls:** Adjust data sizes dynamically using a slider (up to 25,000 elements).
* **Data Flexibility:** Switch instantly between whole integers and floating-point decimals.
* **Real-time Performance Metrics:** Built-in high-precision timers show execution speeds down to the millisecond.

---
*Co-authored by sighthough and Gemini.*
