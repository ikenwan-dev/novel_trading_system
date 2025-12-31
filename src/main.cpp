#include "DataHandlers/HistoricDataHandler/HistoricCSVDataHandler.h"
#include "ExecutionHandlers/SimulatedExecutionHandler/SimulatedExecutionHandler.h"
#include "ExecutionHandlers/TransactionCostModel.h"
#include "Portfolio/Portfolio.h"
#include "RiskManager/RiskManager.h"
#include "Strategies/MovingAverageCrossover/MovingAverageCrossover.h"
#include "Strategies/Strategy.h"
#include "Strategies/StrategyConfig.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <glaze/json/read.hpp>
#include <iostream>
#include <memory>
#include <string>

StrategyConfig load_config_from_file(const std::string &filepath) {
  StrategyConfig sc;
  auto ec =
      glz::read_file_json(sc, "../src/Configs/mac_config.json", std::string{});

  if (ec) {
    // Glaze returns an error object that contains the error code and location
    throw std::runtime_error("Error parsing JSON: " + glz::format_error(ec));
  }
  return sc;
}

std::shared_ptr<Strategy> load_strategy_from_config(
    const StrategyConfig &sc,
    ThreadSafeQueue<std::shared_ptr<Event>> &event_queue) {
  if (sc.name == "MovingAverageCrossover") {
    return std::make_shared<MovingAverageCrossover>(
        event_queue, sc.tickers, sc.params.at("short_window"),
        sc.params.at("long_window"));
  } else {
    throw std::runtime_error("Unknown strategy name: " + sc.name);
  }
}

int main() {

  // Create event queue
  auto event_queue =
      std::make_shared<ThreadSafeQueue<std::shared_ptr<Event>>>();
  std::cout << "Event queue created" << std::endl;

  std::map<std::string, std::string> files{
      {"AAPL", "../src/test_data/aapl.us.txt"},
  };

  // Load data handler
  auto data_handler =
      std::make_shared<HistoricCSVDataHandler>(*event_queue, files);
  std::cout << "Historic CSV data handler created" << std::endl;

  // Load strategy from config
  StrategyConfig sc = load_config_from_file("../src/Configs/mac_config.json");
  auto strategy = std::dynamic_pointer_cast<MovingAverageCrossover>(
      load_strategy_from_config(sc, *event_queue));
  std::cout << "Strategy loaded: " << sc.name << std::endl;

  // Create portfolio
  auto portfolio = std::make_shared<Portfolio>(*data_handler, 100000.0);

  // Create risk manager
  auto risk_manager = std::make_shared<RiskManager>(*portfolio, *event_queue);

  // Create transaction cost model
  auto transaction_cost_model =
      std::make_unique<PershareCommissionModel>(0.005, 1.0);

  // Execution handler
  auto execution_handler = std::make_shared<SimulatedExecutionHandler>(
      *event_queue, *data_handler, std::move(transaction_cost_model));

  return 0;
}