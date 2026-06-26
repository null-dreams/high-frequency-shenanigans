# HFS v0.0 (High Frequency Shenanigans) 

A high-performance C++ Limit Order Book (LOB) designed for simulated high-frequency trading (HFT) and low-latency matching. The implementation follows standard Price-Time Priority (FIFO matching).

---

## 1. Features & Architecture
- **Price-Time Priority (FIFO)**: Crossing orders match sequentially against resting orders at the best available price levels.
- **O(1) Cancellations**: Uses an internal registry (`std::unordered_map`) mapping `OrderId` to order list iterators, enabling fast, constant-time removals.
- **Header Search Safety**: Fixed uninitialized member bugs (`totalQuantity`) to eliminate undefined behavior in limit level calculations.

```
       [Incoming Order]
              │
              ▼
    [Match Engine (FIFO)] ──(Fill)──► [Trade Event]
              │
              ├──(Remaining Qty > 0)
              │
              ▼
   ┌──────────────────────┐
   │    Order Registry    │ (O(1) Cancellation lookup)
   └──────────┬───────────┘
              │ (Iterators)
              ▼
   ┌──────────────────────┐
   │   Bids / Asks Maps   │ (std::map of price -> Limit level)
   └──────────────────────┘
```

---

## 2. API Reference

The primary API is exposed through the `OrderBook` class under the `orderbook` namespace:

### `void add_order(OrderId id, Price price, Quantity qty, Side side)`
Processes an incoming order. It will attempt to match against resting orders on the opposite side of the book first. Any remaining unfilled quantity will rest in the book.
- **`id`**: Unique identifier for the order (duplicate IDs are ignored).
- **`price`**: Limit price.
- **`qty`**: Order quantity.
- **`side`**: `Side::Buy` or `Side::Sell`.

### `void cancel_order(OrderId id)`
Cancels a resting order using its ID in $O(1)$ lookup time and removes it from the book. If the ID is invalid, it results in a no-op.

### `Quantity get_volume_at_price(Price price, Side side) const`
Returns the total outstanding order volume resting at the given price level for the specified side.

### `std::size_t size() const noexcept`
Returns the total count of resting orders currently in the order book.

---

## 3. Quick Start

### Build Prerequisites
- CMake (version 3.15 or higher)
- C++17 compatible compiler (e.g., GCC 9+, Clang 9+, MSVC 2019+)

### Build Steps
```bash
# 1. Create and navigate to the build directory
mkdir -p build && cd build

# 2. Configure project and export compilation database (for IDE / LSP support)
cmake ..

# 3. Compile the library and test runner
make
```

### Running Tests
After compilation, execute the test runner to verify correctness:
```bash
./lob_runner
```

### Running Benchmarks
We also provide a baseline LOB benchmark utility (`lob_benchmark`) that processes packed binary market event streams to measure throughput and latency.

#### 1. Compile the Benchmark Target
Build the benchmark target from the build directory:
```bash
make lob_benchmark # or simply run: make
```

#### 2. Binary Event Stream Format
The benchmark utility reads event streams in a raw, packed binary format. Each event must strictly align to the following 1-byte packed struct:
```cpp
#pragma pack(push, 1)
struct MarketEvent {
    uint8_t type;       // 1 = Add Order, 2 = Cancel Order
    uint64_t order_id;  // Unique ID of the order
    uint64_t price;     // Limit Price
    uint32_t qty;       // Order Quantity
    uint8_t side;       // 0 = Buy, 1 = Sell
};
#pragma pack(pop)
```

#### 3. Execution
Run the benchmark by passing the path to the serialized binary event file:
```bash
./lob_benchmark <path_to_benchmark_data.bin>
```

#### 4. Outputs
Upon execution, the utility reports performance metrics:
- **Total processed events**: Count of processed operations.
- **Execution time**: Time taken to process the entire event loop (ms).
- **Throughput**: Transactions processed per second (ops/sec).
- **Average latency**: Mean processing latency per event (ns).
- **Final book size**: Remaining resting orders in the book.

---

## 5. Future Performance Optimizations (Next Updates)
To achieve sub-microsecond matching latency in real-world environments, the next phase of development will focus on the following optimizations:

1. **Pre-allocated Memory Pools (Custom Allocators)**:
   - *Issue*: Standard `std::list` and `std::map` perform dynamic memory allocations (`new`) on every new order or price level, incurring OS context-switch and mutex locks.
   - *Fix*: Replace standard allocators with a custom pool/arena allocator (e.g., `boost::container::flat_map` or block arenas) to achieve $O(1)$ allocations.

2. **Contiguous Storage & Flat Maps**:
   - *Issue*: `std::map` is structured as a Red-Black Tree, causing pointer chasing and cache misses during traversal.
   - *Fix*: Transition price level indexing to a cache-friendly Flat Map (e.g., a sorted vector of pairs) or direct-indexed arrays for true $O(1)$ price lookups.

3. **Rehash-Safe Order Registry**:
   - *Issue*: `std::unordered_map` triggers expensive rehashing operations when resizing, leading to severe tail-latency spikes.
   - *Fix*: Pre-reserve map buckets based on expected peak trading volume, or utilize closed-addressing flat hash maps (like `absl::flat_hash_map`).
