#include "orderbook/OrderBook.hpp"
#include "orderbook/Protocol.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

using namespace orderbook;
using namespace market::protocol;

// Safely loads the binary file into memory
std::vector<MarketEvent> load_events(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Could not open benchmark data file: " << filepath << "\n";
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size % sizeof(MarketEvent) != 0) {
        std::cerr << "Warning: File size is not a multiple of MarketEvent size. File may be corrupted.\n";
    }

    std::vector<MarketEvent> events(size / sizeof(MarketEvent));
    if (!file.read(reinterpret_cast<char*>(events.data()), size)) {
        std::cerr << "Error: Failed to read benchmark data file cleanly.\n";
        return {};
    }

    return events;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <benchmark_data.bin>\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::cout << "Loading benchmark events from " << filepath << "...\n";
    auto events = load_events(filepath);
    if (events.empty()) {
        return 1;
    }

    std::cout << "Successfully loaded " << events.size() << " events.\n";
    std::cout << "Running baseline LOB benchmark...\n";

    OrderBook ob;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& event : events) {
        if (event.type == static_cast<uint8_t>(ActionType::Add)) {
            ob.add_order(
                event.order_id,
                event.price,
                event.qty,
                static_cast<orderbook::Side>(event.side)
            );
        } else if (event.type == static_cast<uint8_t>(ActionType::Cancel)) {
            ob.cancel_order(event.order_id);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    double seconds = duration.count() / 1000.0;
    double ops_per_sec = static_cast<double>(events.size()) / seconds;
    double avg_latency_ns = (duration.count() * 1'000'000.0) / static_cast<double>(events.size());

    std::cout << "\n================ BENCHMARK RESULTS ================\n";
    std::cout << "Total processed events : " << events.size() << "\n";
    std::cout << "Execution time         : " << duration.count() << " ms\n";
    std::cout << "Throughput             : " << ops_per_sec << " ops/sec\n";
    std::cout << "Avg latency per event  : " << avg_latency_ns << " ns\n";
    std::cout << "Final book size        : " << ob.size() << " active orders\n";
    std::cout << "====================================================\n";

    return 0;
}