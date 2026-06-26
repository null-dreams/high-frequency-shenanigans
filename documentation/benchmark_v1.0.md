# Limit Order Book Benchmark Report (v1.0 - Optimized Pool & Intrusive Lists)

This report details the performance benchmark results for **High Frequency Shenanigans (HFS) v1.0**, featuring an optimized custom Object Pool and Intrusive Doubly-Linked Lists. The benchmark measures the matching engine's throughput and average processing latency against a packed binary stream of 2,000,000 market events.

---

## 1. Summary of Optimizations in v1.0

To address the performance bottlenecks identified in the v0.0 audit (specifically, OS heap allocations and cache misses), the following optimizations were implemented:

1.  **Intrusive Doubly-Linked Lists**:
    *   Replaced the standard `std::list<Order>` in `Limit` with an intrusive list layout.
    *   Moved the `next` and `prev` pointers directly inside the `Order` struct.
    *   This eliminates the wrapper node overhead of `std::list` and improves spatial locality.
2.  **Order Object Pool (Arena Allocator)**:
    *   Introduced an `OrderPool` class that pre-allocates a contiguous vector of `Order` objects.
    *   Resting orders are acquired from the pool in $O(1)$ and released back to a free-list upon execution/cancellation in $O(1)$.
    *   This completely bypasses dynamic heap allocation (`new`/`delete`) during the matching engine's critical path.
3.  **Registry Reservation**:
    *   The `order_registry_` (`std::unordered_map`) is pre-reserved to match the order pool capacity, minimizing rehashing latency spikes.

---

## 2. System Specifications

The benchmark was executed under the same environment as the baseline (v0.0):

*   **CPU**: 13th Gen Intel(R) Core(TM) i5-13420H (Hybrid architecture with Performance and Efficient cores)
*   **Memory**: 16 GB RAM
*   **Operating System**: Ubuntu 24.04 LTS (Linux)
*   **Compiler**: `g++` version 13.3.0
*   **Compilation Flags**: `-O3 -std=c++17 -Iinclude`

---

## 3. Benchmark Results (v1.0)

Below are the results from three consecutive trial runs of the optimized matching engine processing all 2,000,000 events:

| Metric | Run 1 | Run 2 | Run 3 | Average |
| :--- | :--- | :--- | :--- | :--- |
| **Total Events** | 2,000,000 | 2,000,000 | 2,000,000 | **2,000,000** |
| **Execution Time** | 204.235 ms | 204.547 ms | 200.941 ms | **203.241 ms** |
| **Throughput** | 9.793M ops/sec | 9.778M ops/sec | 9.953M ops/sec | **9.841M ops/sec** |
| **Avg. Latency** | 102.118 ns | 102.274 ns | 100.471 ns | **101.621 ns** |
| **Final Book Size** | 249,300 | 249,300 | 249,300 | **249,300** |

---

## 4. Performance Comparison: v0.0 vs v1.0

A comparison of the average metrics between the baseline (v0.0) and optimized (v1.0) implementations:

| Version | Avg. Execution Time | Avg. Throughput | Avg. Latency | Speedup | Latency Reduction |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **v0.0 (Baseline)** | 439.979 ms | 4.560M ops/sec | 219.990 ns | 1.00x (Ref) | 0.0% |
| **v1.0 (Optimized)** | **203.241 ms** | **9.841M ops/sec** | **101.621 ns** | **2.16x** | **53.8%** |

```
Latency Comparison (Lower is Better)
v0.0: ██████████████████████ 219.99 ns
v1.0: ██████████ 101.62 ns

Throughput Comparison (Higher is Better)
v0.0: ░░░░░░░░░ 4.56M ops/sec
v1.0: ░░░░░░░░░░░░░░░░░░░░ 9.84M ops/sec
```

---

## 5. Key Observations

1.  **Heap Allocations Eliminated**: By replacing standard `std::list` heap allocations with a custom pre-allocated block array (`OrderPool`), we eliminated OS-level dynamic memory lock overhead. This halved the execution time.
2.  **Improved Cache Locality**: Using intrusive pointers directly inside the `Order` struct allowed traversal of order queues with fewer cache misses compared to the standard doubly-linked list.
3.  **Tail Latency & Stability**: The system shows highly stable performance across multiple runs, with standard deviation in execution times falling below 2%.

---

## 6. Next Optimization Version (v2.0 Goals)

To break the 100ns latency barrier and reach 15M+ ops/sec:
1.  **Flat Map / Direct Array Indexing**: Replace the `std::map` red-black tree with a cache-friendly flat array or sorted contiguous vector for price levels.
2.  **Thread Affinity & Core Pinning**: Pin the matching engine thread to a performance CPU core to avoid context switching and OS-scheduler interrupts.
