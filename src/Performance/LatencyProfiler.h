#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace backtesting_engine::performance {

enum class Metric { TICK_TO_TRADE, LOB_ADD, LOB_CANCEL, LOB_MODIFY, COUNT };

class LatencyProfiler {
public:
  LatencyProfiler() : msg_throughput_count_(0) {
    // Pre-allocate 10,000,000 samples per metric type to avoid allocations
    // during runtime. 10M samples * 8 bytes = ~80 MB of RAM per Metric.
    for (int i = 0; i < static_cast<int>(Metric::COUNT); ++i) {
      latencies_[i].reserve(10'000'000);
    }
  }

  inline void record_latency(Metric type, uint64_t latency_ns) {
    auto &vec = latencies_[static_cast<int>(type)];
    if (vec.size() < vec.capacity()) {
      vec.push_back(latency_ns);
    }
    // Note: To remain absolutely allocation-free, we simply drop telemetry
    // if the massive pre-allocated bounds are exceeded rather than returning to
    // the OS to resize vectors in the middle of a hot-loop.
  }

  inline void increment_throughput_counter() {
    // Standard non-atomic increment for zero-overhead performance
    msg_throughput_count_++;
  }

  void print_histograms(uint64_t sim_wall_clock_time_ns) {
    std::cout << "\n============================================\n";
    std::cout << "         HFT PERFORMANCE METRICS            \n";
    std::cout << "============================================\n";

    double msg_per_sec =
        (static_cast<double>(msg_throughput_count_) / sim_wall_clock_time_ns) *
        1e9;
    std::cout << "System Throughput: " << std::fixed << std::setprecision(2)
              << msg_per_sec << " Msg/Sec\n";
    std::cout << "Messages Processed: " << msg_throughput_count_ << "\n\n";

    print_metric(Metric::LOB_ADD, "LOB Add Latency");
    print_metric(Metric::LOB_CANCEL, "LOB Cancel Latency");
    print_metric(Metric::LOB_MODIFY, "LOB Modify Latency");
    print_metric(Metric::TICK_TO_TRADE, "Tick-to-Trade Latency");
  }

private:
  void print_metric(Metric type, const std::string &name) {
    auto &vec = latencies_[static_cast<int>(type)];
    if (vec.empty()) {
      std::cout << "[" << name << "] No data recorded.\n\n";
      return;
    }

    std::sort(vec.begin(), vec.end());

    uint64_t mean = 0;
    for (auto val : vec)
      mean += val;
    mean /= vec.size();

    uint64_t p50 = vec[static_cast<size_t>(vec.size() * 0.50)];
    uint64_t p90 = vec[static_cast<size_t>(vec.size() * 0.90)];
    uint64_t p99 = vec[static_cast<size_t>(vec.size() * 0.99)];
    uint64_t p99_9 = vec[static_cast<size_t>(vec.size() * 0.999)];
    uint64_t max_val = vec.back();

    std::cout << "[" << name << "]\n";
    std::cout << "  Samples: " << vec.size() << "\n";
    std::cout << "  Mean:    " << mean << " ns\n";
    std::cout << "  P50:     " << p50 << " ns\n";
    std::cout << "  P90:     " << p90 << " ns\n";
    std::cout << "  P99:     " << p99 << " ns\n";
    std::cout << "  P99.9:   " << p99_9 << " ns\n";
    std::cout << "  Max:     " << max_val << " ns\n\n";
  }

  std::array<std::vector<uint64_t>, static_cast<int>(Metric::COUNT)> latencies_;
  uint64_t msg_throughput_count_;
};

} // namespace backtesting_engine::performance
