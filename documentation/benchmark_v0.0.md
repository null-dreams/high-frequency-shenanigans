# Limit Order Book Benchmark Report (v0.0 - Baseline)

This report details the baseline performance benchmark results for the **High Frequency Shenanigans (HFS) v0.0** Limit Order Book (LOB) matching engine. The benchmark processes a packed binary stream of market events representing order additions and cancellations to evaluate throughput and average event-processing latency.

---

## 1. System Specifications

The benchmark was executed on the following hardware and software environment:

*   **CPU**: 13th Gen Intel(R) Core(TM) i5-13420H (Hybrid architecture with Performance and Efficient cores)
*   **Memory**: 16 GB RAM
*   **Operating System**: Ubuntu 24.04 LTS (Linux)
*   **Compiler**: `g++` version 13.3.0
*   **Compilation Flags**: `-O3 -std=c++17 -Iinclude`

---

## 2. Benchmark Methodology

The matching engine was benchmarked against a pre-generated, packed binary event stream file (`test_data.bin`) containing 2,000,000 (2 Million) serialized market events:

*   **Dataset Size**: 44,000,000 bytes (44 MB)
*   **Event Struct Alignment**: Packed 1-byte boundary struct containing:
    *   `type` (1 byte): `1` for Add, `2` for Cancel
    *   `order_id` (8 bytes): Unique identifier
    *   `price` (8 bytes): Limit price
    *   `qty` (4 bytes): Order quantity
    *   `side` (1 byte): `0` for Buy, `1` for Sell
*   **Operations**: Processes a sequence of orders, matching opposite sides or resting unmatched quantities in the book.

---

## 3. Benchmark Results (v0.0)

Below are the results from three consecutive trial runs of the baseline matching engine processing all 2,000,000 events:

| Metric | Run 1 | Run 2 | Run 3 | Average |
| :--- | :--- | :--- | :--- | :--- |
| **Total Events** | 2,000,000 | 2,000,000 | 2,000,000 | **2,000,000** |
| **Execution Time** | 425.073 ms | 475.430 ms | 419.434 ms | **439.979 ms** |
| **Throughput** | 4.705M ops/sec | 4.207M ops/sec | 4.768M ops/sec | **4.560M ops/sec** |
| **Avg. Latency** | 212.537 ns | 237.715 ns | 209.717 ns | **219.990 ns** |
| **Final Book Size** | 249,300 | 249,300 | 249,300 | **249,300** |

---

## 4. Key Observations

1.  **Throughput & Latency**: The baseline matching engine achieved an average throughput of **4.56M operations per second** with an average latency of **~220 ns per event**.
2.  **Order Matching Performance**: During the execution, a significant portion of incoming orders matched and cleared. The engine successfully handled the high density of events, ending with **249,300 resting orders** in the book.
3.  **Efficiency Profile**:
    *   **Low Latency**: Processing under 250 ns per event is highly performant for standard container usage.
    *   **Bottlenecks**: Analysis shows that dynamic allocations via `std::list` (for order lists) and tree node allocations via `std::map` (for price limits) remain the primary source of cache misses and CPU cycle consumption.
    *   **Rehashing Overhead**: The lookup registry (`std::unordered_map`) may trigger periodic rehashing as the registry expands to accommodate hundreds of thousands of active orders, causing latency tail spikes.

---

## 5. Next Optimization Version (v1.0 Goals)

To achieve sub-100ns latency (10M+ ops/sec), future versions will focus on:
1.  **Pre-allocated Memory Pools (Custom Allocators)** to eliminate heap-allocation locks.
2.  **Contiguous Flat Maps** to optimize cache line utilization.
3.  **Pre-reserved/Rehash-safe Order Registry** to minimize tail-latency spikes.
