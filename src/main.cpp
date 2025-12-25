#include "DataHandlers/HistoricDataHandler/HistoricCSVDataHandler.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include "Strategies/StrategyConfig.h"
#include "Strategies/Strategy.h" 
#include "Strategies/MovingAverageCrossover/MovingAverageCrossover.h"
#include <glaze/json/read.hpp>
#include <iostream>
#include <memory>


StrategyConfig load_config_from_file(const std::string& filepath){
  StrategyConfig sc;
  auto ec = glz::read_file_json(sc, "../src/Configs/mac_config.json", std::string{});

  if (ec) {
    // Glaze returns an error object that contains the error code and location
    throw std::runtime_error("Error parsing JSON: " + glz::format_error(ec));
  }
  std::cout << "Loaded strategy with name: " << sc.name << std::endl;
  return sc;
}

std::shared_ptr<Strategy> load_strategy_from_config(const StrategyConfig& sc, ThreadSafeQueue<std::shared_ptr<Event>>& event_queue){
  if (sc.name == "MovingAverageCrossover") {
    std::cout << "Loaded MovingAverageCrossover strategy" << std::endl;
    return std::make_shared<MovingAverageCrossover>(event_queue, sc.tickers,sc.params.at("short_window"), sc.params.at("long_window"));
  } else {
    throw std::runtime_error("Unknown strategy name: " + sc.name);
  }
}
  

int main() {
  try {

    ThreadSafeQueue<std::shared_ptr<Event>> event_queue{};
    
    std::map<std::string, std::string> files{
        {"AAPL", "../src/test_data/aapl.us.txt"},
    };
    auto historic_csv_data_handler =
        std::make_shared<HistoricCSVDataHandler>(event_queue, files);
    auto bars = historic_csv_data_handler->all_data_.at("AAPL");
    std::cout << "Loaded " << bars.size() << " bars\n";

    StrategyConfig sc = load_config_from_file("../src/Configs/mac_config.json");
    std::shared_ptr<Strategy> strategy = load_strategy_from_config(sc, event_queue);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}