#include "DataHandlers/HistoricDataHandler/HistoricCSVDataHandler.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <iostream>
#include <map>

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
    if (!bars.empty()) {
      const auto &b = bars.front();
      std::time_t tt = std::chrono::system_clock::to_time_t(b.timestamp);
      std::cout << b.symbol << " first bar:\n";
      std::cout << "  time:  " << std::ctime(&tt); // ctime() adds newline
      std::cout << "  open:  " << b.open << "\n";
      std::cout << "  high:  " << b.high << "\n";
      std::cout << "  low:   " << b.low << "\n";
      std::cout << "  close: " << b.close << "\n";
      std::cout << "  vol:   " << b.volume << "\n";
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}