#include "DataHandlers/HistoricDataHandler/HistoricCSVDataHandler.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include "Strategies/StrategyConfig.h"
#include <glaze/json/read.hpp>
#include <iostream>


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
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}