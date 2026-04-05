#include "Constants/Constants.h"
#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"
#include "Events/Mbo/MBOSimulationEngine.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "Performance/LatencyProfiler.h"
#include "Performance/SimulationProfiler.h"
#include "Performance/TSC_Clock.h"
#include "RiskManager/Mbo/MBORiskManager.h"
#include "Strategies/MBOStrategies/MarketMaker/MarketMaker.h"
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

int main() {
  try {
    std::cout << "Waiting for Producer to create SHM..." << std::endl;
    DataBentoConsumer::Interactor reader("test_shm", false);
    while (!reader.is_initialized()) {
      // spin while producer creates shm
    }
    std::cout << "Ring buffer initialized." << std::endl;

    // 1. Core Event Queue & Network Simulator
    EventQueue queue;
    NetworkSimulator simulator(queue, Constants::OUTBOUND_LATENCY,
                               Constants::INBOUND_LATENCY);

    // 2. Data Structures
    DataBentoLOB lob;

    // 3. Risk & OMS
    // Dummy max properties for testing
    MBORiskManager risk_manager(200, 1000);

    // We instantiate OMS with max_orders
    OrderManagementSystem oms(simulator, risk_manager, 20000000);

    // 4. Performance Profilers
    performance::TSC_Clock::calibrate();
    performance::LatencyProfiler profiler;
    performance::SimulationProfiler sim_profiler;

    // 5. Virtual Exchange
    VirtualExchange virtual_exchange(simulator, lob, sim_profiler);

    // 6. Strategy (Market Maker)
    // For example, half_spread_ticks = 5, order_qty = 10
    MarketMaker strategy(oms, 5, 10, &profiler);

    // 7. Data Consumer wrapper
    DataBentoConsumer data_consumer(reader);

    // 8. Simulation Engine
    MBOSimulationEngine<MarketMaker> engine(data_consumer, simulator,
                                            virtual_exchange, lob, oms,
                                            strategy, profiler);

    std::cout << "Starting Simulation Engine..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    engine.run();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Simulation completed in " << diff.count() << " seconds."
              << std::endl;
    std::pair<int64_t, int64_t> bbo = lob.get_bbo();
    std::cout << "Final LOB BBO: $" << std::fixed << std::setprecision(2)
              << static_cast<double>(bbo.first) / 1e9 << " @ $"
              << static_cast<double>(bbo.second) / 1e9 << std::endl;
    std::cout << "Final Portfolio Holdings inside Strategy: "
              << strategy.get_position() << std::endl;
    oms.print_summary(strategy.get_last_valid_mid_price());
    profiler.print_histograms(
        std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count());
    sim_profiler.print_histograms();

    reader.unlink();

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}