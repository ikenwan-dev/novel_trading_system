#include "Constants/Constants.h"
#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"
#include "Events/Mbo/MBOSimulationEngine.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
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

    // 4. Virtual Exchange
    VirtualExchange virtual_exchange(simulator, lob);

    // 5. Strategy (Market Maker)
    // For example, half_spread_ticks = 5, order_qty = 10
    MarketMaker strategy(oms, 5, 10);

    // 6. Data Consumer wrapper
    DataBentoConsumer data_consumer(reader);

    // 7. Simulation Engine
    MBOSimulationEngine<MarketMaker> engine(
        data_consumer, simulator, virtual_exchange, lob, oms, strategy);

    std::cout << "Starting Simulation Engine..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    engine.run();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Simulation completed in " << diff.count() << " seconds."
              << std::endl;
    std::cout << "Final LOB BBO: " << lob.get_bbo().first << " @ "
              << lob.get_bbo().second << std::endl;
    std::cout << "Final Portfolio Holdings inside Strategy: "
              << strategy.get_position() << std::endl;
    std::cout << "OrderManagement System holdings: " << oms.get_holdings()
              << std::endl;
    std::cout << "OrderManagement System cash: $" << std::fixed
              << std::setprecision(2)
              << static_cast<double>(oms.get_cash()) / 1e9 << std::endl;
    
    // Print true Mark-To-Market Equity
    int64_t last_mid = strategy.get_last_valid_mid_price();
    if (last_mid != 0) {
      double mtm_equity = static_cast<double>(oms.get_mtm_equity(last_mid)) / 1e9;
      std::cout << "OrderManagement System MTM Equity: $" << std::fixed
                << std::setprecision(2) << mtm_equity << " (based on last valid mid: $"
                << static_cast<double>(last_mid) / 1e9 << ")" << std::endl;
    } else {
      std::cout << "OrderManagement System MTM Equity: N/A (no valid mid price recorded)" << std::endl;
    }

    oms.print_order_status_counts();

    reader.unlink();

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}