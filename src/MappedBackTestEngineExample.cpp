#include "Constants/Constants.h"
#include "DataConsumers/DataBentoMappedConsumer/DataBentoMappedConsumer.h"
#include "Events/Mbo/MBOSimulationEngine.h"
// #include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "LimitOrderBook/DataBentoLOB/DirectArrayLOB.h"
// #include "LimitOrderBook/DataBentoLOB/OptimizedDataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "Performance/LatencyProfiler.h"
#include "Performance/SimulationProfiler.h"
#include "Performance/TSC_Clock.h"
#include "RiskManager/Mbo/MBORiskManager.h"
#include "Strategies/MBOStrategies/MarketMaker/MarketMaker.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

std::vector<std::string> get_dbn_files(const std::string &directory) {
  std::vector<std::string> files;
  const std::string suffix = ".mbo.dbn";
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      std::string path = entry.path().string();
      if (path.size() >= suffix.size() &&
          path.compare(path.size() - suffix.size(), suffix.size(), suffix) ==
              0) {
        files.push_back(path);
      }
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

int main(int argc, char *argv[]) {
  try {
    // 1. Core Event Queue & Network Simulator
    EventQueue queue;
    NetworkSimulator simulator(queue, Constants::OUTBOUND_LATENCY,
                               Constants::INBOUND_LATENCY);

    // 2. Data Structures
    // Choose your LOB implementation here:
    // Example tick size for NAS/NQ/ES. Assuming .1 cent tick size = 0.001 * 1e9
    // = 1000000
    static constexpr int64_t assumed_tick_size = 100000;
    using LOBType = DirectArrayLOB<assumed_tick_size>;
    auto lob = std::make_unique<LOBType>();

    // 3. Risk & OMS
    MBORiskManager risk_manager(200, 1000);
    OrderManagementSystem oms(simulator, risk_manager, 20000000);

    // 4. Performance Profilers
    performance::TSC_Clock::calibrate();
    performance::LatencyProfiler profiler;
    performance::SimulationProfiler sim_profiler;

    // 5. Virtual Exchange
    VirtualExchange<LOBType> virtual_exchange(simulator, *lob, sim_profiler);

    // 6. Strategy (Market Maker)
    MarketMaker strategy(oms, 5, 10, &profiler);

    // 7. Data Consumer wrapper
    std::vector<std::string> filepaths;
    if (argc > 1) {
      if (std::filesystem::is_directory(argv[1])) {
        filepaths = get_dbn_files(argv[1]);
      } else {
        filepaths.push_back(argv[1]);
      }
    } else {
      std::string directory = std::string(PROJECT_ROOT) +
                              "/src/test_data/XNAS-20260120-7W93CD9NGT/";
      filepaths = get_dbn_files(directory);
    }

    if (filepaths.empty()) {
      std::cerr << "No .mbo.dbn files found to process." << std::endl;
      return 1;
    }

    DataBentoMappedConsumer data_consumer(filepaths);

    // 8. Simulation Engine
    MBOSimulationEngine<DataBentoMappedConsumer, LOBType, MarketMaker> engine(
        data_consumer, simulator, virtual_exchange, *lob, oms, strategy,
        profiler);

    std::cout << "Starting Simulation Engine (MMAP)..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    engine.run();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "\n============================================\n";
    std::cout << "           SIMULATION RUN SUMMARY           \n";
    std::cout << "============================================\n";
    std::cout << std::left << std::setw(25) << "  Duration:" << std::fixed
              << std::setprecision(4) << diff.count() << " seconds"
              << std::endl;

    std::pair<int64_t, int64_t> bbo = lob->get_bbo();
    std::cout << std::left << std::setw(25) << "  Final LOB BBO:"
              << " $" << std::fixed << std::setprecision(2)
              << static_cast<double>(bbo.first) / 1e9 << " @ $"
              << static_cast<double>(bbo.second) / 1e9 << std::endl;

    std::cout << std::left << std::setw(25)
              << "  Strategy Position:" << strategy.get_position() << " units"
              << std::endl;
    std::cout << "============================================\n" << std::endl;
    oms.print_summary(strategy.get_last_valid_mid_price());
    profiler.print_histograms(
        std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count());
    sim_profiler.print_histograms();

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
