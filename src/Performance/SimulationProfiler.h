#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace backtesting_engine::performance {

enum class SimMetric { VEX_ORDER_ADD, VEX_ORDER_CANCEL, VEX_MATCHING, COUNT };

class SimulationProfiler {
public:
  SimulationProfiler() {
    // Pre-allocate millions of samples to strictly avoid OS allocation
    // overheads during Hot Path simulation telemetry recording.
    for (int i = 0; i < static_cast<int>(SimMetric::COUNT); ++i) {
      latencies_[i].reserve(10'000'000);
    }
  }

  inline void record_latency(SimMetric type, uint64_t latency_ns) {
    auto &vec = latencies_[static_cast<int>(type)];
    if (vec.size() < vec.capacity()) {
      vec.push_back(latency_ns);
    }
  }

  void print_histograms() {
    std::cout << "\n============================================\n";
    std::cout << "         VIRTUAL EXCHANGE SIMULATION METRICS        \n";
    std::cout << "============================================\n";

    print_metric(SimMetric::VEX_ORDER_ADD,
                 "Virtual Exchange Add Order Overhead");
    print_metric(SimMetric::VEX_ORDER_CANCEL,
                 "Virtual Exchange Cancel Order Overhead");
    print_metric(SimMetric::VEX_MATCHING,
                 "Virtual Exchange Matching Engine Overhead");
  }

private:
  void print_metric(SimMetric type, const std::string &name) {
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

  std::array<std::vector<uint64_t>, static_cast<int>(SimMetric::COUNT)>
      latencies_;
};

} // namespace backtesting_engine::performance
