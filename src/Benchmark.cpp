#include "orderbook/OrderBook.hpp"
#include "orderbook/Protocol.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <x86intrin.h>
#include <thread>

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

struct LatencyStats {
    uint64_t min;
    uint64_t p50;
    uint64_t p90;
    uint64_t p95;
    uint64_t p99;
    uint64_t p99_9;
    uint64_t p99_99;
    uint64_t max;
    double mean;
    double total_ms;
    double throughput;
};

struct LatencyStatsDouble {
    double min;
    double p50;
    double p90;
    double p95;
    double p99;
    double p99_9;
    double p99_99;
    double max;
    double mean;
    double total_ms;
    double throughput;
};

size_t get_percentile_index(size_t n, double percentile) {
    size_t idx = static_cast<size_t>(percentile / 100.0 * (n - 1));
    if (idx >= n) idx = n - 1;
    return idx;
}

LatencyStats calculate_stats(std::vector<uint64_t>& latencies, double total_ms) {
    std::sort(latencies.begin(), latencies.end());
    size_t n = latencies.size();
    
    uint64_t sum = 0;
    for (uint64_t lat : latencies) {
        sum += lat;
    }
    
    LatencyStats stats;
    stats.min = latencies.empty() ? 0 : latencies.front();
    stats.p50 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 50.0)];
    stats.p90 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 90.0)];
    stats.p95 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 95.0)];
    stats.p99 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 99.0)];
    stats.p99_9 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 99.9)];
    stats.p99_99 = latencies.empty() ? 0 : latencies[get_percentile_index(n, 99.99)];
    stats.max = latencies.empty() ? 0 : latencies.back();
    stats.mean = latencies.empty() ? 0.0 : static_cast<double>(sum) / n;
    stats.total_ms = total_ms;
    stats.throughput = latencies.empty() ? 0.0 : static_cast<double>(n) / (total_ms / 1000.0);
    
    return stats;
}

void print_header() {
    std::cout << "\n================================================= BENCHMARK LATENCY DISTRIBUTION (5 RUNS) =================================================\n";
    std::cout << " " << std::left << std::setw(6) << "Run"
              << std::right << std::setw(11) << "Time (ms)"
              << std::setw(19) << "Throughput (ops/s)"
              << std::setw(11) << "Min (ns)"
              << std::setw(11) << "P50 (ns)"
              << std::setw(11) << "P90 (ns)"
              << std::setw(11) << "P95 (ns)"
              << std::setw(11) << "P99 (ns)"
              << std::setw(12) << "P99.9 (ns)"
              << std::setw(13) << "P99.99 (ns)"
              << std::setw(12) << "Max (ns)"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------------\n";
}

void print_row(const std::string& run_label, const LatencyStats& stats) {
    std::cout << " " << std::left << std::setw(6) << run_label
              << std::right << std::setw(11) << std::fixed << std::setprecision(2) << stats.total_ms
              << std::setw(19) << std::fixed << std::setprecision(0) << stats.throughput
              << std::setw(11) << stats.min
              << std::setw(11) << stats.p50
              << std::setw(11) << stats.p90
              << std::setw(11) << stats.p95
              << std::setw(11) << stats.p99
              << std::setw(12) << stats.p99_9
              << std::setw(13) << stats.p99_99
              << std::setw(12) << stats.max
              << "\n";
}

void print_row_double(const std::string& run_label, const LatencyStatsDouble& stats) {
    std::cout << " " << std::left << std::setw(6) << run_label
              << std::right << std::setw(11) << std::fixed << std::setprecision(2) << stats.total_ms
              << std::setw(19) << std::fixed << std::setprecision(0) << stats.throughput
              << std::setw(11) << std::fixed << std::setprecision(1) << stats.min
              << std::setw(11) << stats.p50
              << std::setw(11) << stats.p90
              << std::setw(11) << stats.p95
              << std::setw(11) << stats.p99
              << std::setw(12) << stats.p99_9
              << std::setw(13) << stats.p99_99
              << std::setw(12) << stats.max
              << "\n";
}

double calibrate_tsc_cycles_per_ns() {
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t start_tsc = __rdtsc();
    
    // Sleep for 100 milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    uint64_t end_tsc = __rdtsc();
    
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    return static_cast<double>(end_tsc - start_tsc) / static_cast<double>(duration_ns);
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
    
    std::cout << "Calibrating TSC frequency... " << std::flush;
    double tsc_cycles_per_ns = calibrate_tsc_cycles_per_ns();
    std::cout << "Done. (1 ns = " << std::fixed << std::setprecision(4) << tsc_cycles_per_ns << " TSC cycles)\n";

    std::cout << "Running 5 benchmark trials using TSC timing...\n";

    std::vector<LatencyStats> run_stats;
    run_stats.reserve(5);

    for (int run = 1; run <= 50; ++run) {
        std::cout << "Executing Run " << run << "/5..." << std::flush;
        
        OrderBook ob;
        std::vector<uint64_t> latencies(events.size());

        auto run_start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < events.size(); ++i) {
            const auto& event = events[i];
            
            uint64_t op_start = __rdtsc();
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
            uint64_t op_end = __rdtsc();
            
            latencies[i] = op_end - op_start;
        }

        auto run_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> run_duration = run_end - run_start;

        // Convert raw tsc cycle differences to nanoseconds outside the critical loop
        for (size_t i = 0; i < latencies.size(); ++i) {
            latencies[i] = static_cast<uint64_t>(static_cast<double>(latencies[i]) / tsc_cycles_per_ns);
        }

        run_stats.push_back(calculate_stats(latencies, run_duration.count()));
        std::cout << " Done. (Throughput: " << std::fixed << std::setprecision(2) 
                  << (run_stats.back().throughput / 1'000'000.0) << "M ops/sec)\n";
    }

    // Print table of results
    print_header();
    for (size_t i = 0; i < run_stats.size(); ++i) {
        print_row(std::to_string(i + 1), run_stats[i]);
    }
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------------\n";

    // Calculate and print averages
    LatencyStatsDouble avg = {};
    for (const auto& stats : run_stats) {
        avg.min += stats.min;
        avg.p50 += stats.p50;
        avg.p90 += stats.p90;
        avg.p95 += stats.p95;
        avg.p99 += stats.p99;
        avg.p99_9 += stats.p99_9;
        avg.p99_99 += stats.p99_99;
        avg.max += stats.max;
        avg.mean += stats.mean;
        avg.total_ms += stats.total_ms;
        avg.throughput += stats.throughput;
    }

    double num_runs = static_cast<double>(run_stats.size());
    avg.min /= num_runs;
    avg.p50 /= num_runs;
    avg.p90 /= num_runs;
    avg.p95 /= num_runs;
    avg.p99 /= num_runs;
    avg.p99_9 /= num_runs;
    avg.p99_99 /= num_runs;
    avg.max /= num_runs;
    avg.mean /= num_runs;
    avg.total_ms /= num_runs;
    avg.throughput /= num_runs;

    print_row_double("Avg", avg);
    std::cout << "============================================================================================================================================\n";

    std::cout << "\nAverage Latency (Mean): " << std::fixed << std::setprecision(2) << avg.mean << " ns\n\n";

    return 0;
}