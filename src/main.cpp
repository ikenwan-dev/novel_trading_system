#include "DataHandlers/HistoricDataHandler/HistoricCSVDataHandler.h"
#include "ExecutionHandlers/SimulatedExecutionHandler/SimulatedExecutionHandler.h"
#include "ExecutionHandlers/TransactionCostModel.h"
#include "Portfolio/Portfolio.h"
#include "RiskManager/RiskManager.h"
#include "Statistics/Statistics.h"
#include "Strategies/BarStrategies/MovingAverageCrossover/MovingAverageCrossover.h"
#include "Strategies/BarStrategies/Strategy.h"
#include "Strategies/BarStrategies/StrategyConfig.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include "glaze/json/read.hpp"
#include <fstream>
#include <iomanip> // std::put_time
#include <iostream>
#include <memory>
#include <ranges>
#include <string>

using namespace backtesting_engine;
using namespace backtesting_engine::bar;

StrategyConfig load_config_from_file(const std::string &filepath) {
  StrategyConfig sc;
  auto ec = glz::read_file_json(sc, filepath, std::string{});

  if (ec) {
    // Glaze returns an error object that contains the error code and location
    throw std::runtime_error("Error parsing JSON: " + glz::format_error(ec));
  }
  return sc;
}

std::shared_ptr<Strategy> load_strategy_from_config(
    const StrategyConfig &sc,
    common::ThreadSafeQueue<std::shared_ptr<Event>> &event_queue) {
  if (sc.name == "MovingAverageCrossover") {
    auto keys_view = std::views::keys(sc.ticker_map);
    std::vector<std::string> tickers{keys_view.begin(), keys_view.end()};
    return std::make_shared<MovingAverageCrossover>(
        event_queue, tickers, sc.params.at("short_window"),
        sc.params.at("long_window"));
  } else {
    throw std::runtime_error("Unknown strategy name: " + sc.name);
  }
}

void write_equity_curve_to_csv(const std::vector<EquityDataPoint> &equity_curve,
                               const std::string &filepath) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filepath);
  }
  file << "timestamp,equity\n";
  for (const auto &point : equity_curve) {
    auto time = std::chrono::system_clock::to_time_t(point.timestamp);
    std::tm tm = *std::localtime(&time);
    file << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "," << point.value
         << "\n";
  }
  file.close();
}

int main() {
  try { // Create event queue
    std::cout << "Initializing components...\n" << std::endl;

    auto event_queue =
        std::make_shared<common::ThreadSafeQueue<std::shared_ptr<Event>>>();
    std::cout << "Event queue created" << std::endl;

    // Load strategy from config
    // TODO: make this configurable. create a factory pattern (or overload these
    // functions)
    StrategyConfig sc = load_config_from_file(std::string(PROJECT_ROOT) +
                                              "/src/Configs/mac_config.json");
    auto strategy = std::dynamic_pointer_cast<MovingAverageCrossover>(
        load_strategy_from_config(sc, *event_queue));
    std::cout << "Strategy loaded: " << sc.name << std::endl;

    std::unordered_map<std::string, std::string> files;
    for (const auto &[ticker, filepath] : sc.ticker_map) {
      std::cout << "Ticker: " << ticker << " Filepath: " << filepath
                << std::endl;
      files[ticker] = std::string(PROJECT_ROOT) + filepath;
    }
    std::cout << "Files: created" << std::endl;

    // Load data handler
    auto data_handler =
        std::make_shared<HistoricCSVDataHandler>(*event_queue, files);
    std::cout << "Historic CSV data handler created" << std::endl;

    // Create portfolio
    auto portfolio = std::make_shared<Portfolio>(*data_handler, 100000.0);
    std::cout << "Portfolio created with total value of: "
              << portfolio->get_total_value() << std::endl;

    // Create risk manager
    auto risk_manager = std::make_shared<RiskManager>(*portfolio, *event_queue);
    std::cout << "Risk manager created" << std::endl;

    // Create transaction cost model
    auto transaction_cost_model =
        std::make_unique<PershareCommissionModel>(0.005, 1.0);
    std::cout << "Transaction cost model created" << std::endl;

    // Execution handler
    auto execution_handler = std::make_shared<SimulatedExecutionHandler>(
        *event_queue, *data_handler, std::move(transaction_cost_model));
    std::cout << "Execution handler created" << std::endl;

    std::vector<EquityDataPoint> equity_curve;

    std::cout << "Initialization complete. Starting event loop...\n"
              << std::endl;

    while (data_handler->is_running()) {
      data_handler->update();

      std::shared_ptr<Event> event;
      while (event_queue->try_pop(event)) {
        switch (event->get_type()) {
        case EventType::Signal: {
          auto signal_event = std::dynamic_pointer_cast<SignalEvent>(event);
          risk_manager->on_signal(*signal_event);
          break;
        }
        case EventType::Market: {
          auto market_event = std::dynamic_pointer_cast<MarketEvent>(event);
          strategy->on_market_data(*market_event);
          portfolio->on_market_data(*market_event);
          equity_curve.push_back(
              {market_event->timestamp_, portfolio->get_total_value()});
          break;
        }
        case EventType::Order: {
          auto order_event = std::dynamic_pointer_cast<OrderEvent>(event);
          execution_handler->on_order(*order_event);
          break;
        }
        case EventType::Fill: {
          auto fill_event = std::dynamic_pointer_cast<FillEvent>(event);
          portfolio->on_fill(*fill_event);
          break;
        }
        default: {
          std::cout << "Unknown event type" << std::endl;
          break;
        }
        }
      }
    }
    std::cout << "Event loop finished. Portfolio contains a value of "
              << portfolio->get_total_value()
              << " and a cash amt of : " << portfolio->get_cash() << std::endl;
    std::cout << "unrealized pnl: " << portfolio->get_unrealized_pnl()
              << std::endl;

    for (const auto &[key, val] : portfolio->all_positions()) {
      std::cout << key << " : " << val.quantity << std::endl;
    }

    double cagr = Statistics::calculate_cagr(equity_curve);
    std::cout << "CAGR: " << cagr << std::endl;
    write_equity_curve_to_csv(equity_curve, "equity_curve.csv");

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}