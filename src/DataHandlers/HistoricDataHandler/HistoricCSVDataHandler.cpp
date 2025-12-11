#include "HistoricCSVDataHandler.h"

HistoricCSVDataHandler::HistoricCSVDataHandler(
    ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
    const std::map<std::string, std::string> &csv_files)
    : event_queue_(event_queue) {
  load_all_data(csv_files);
}

void HistoricCSVDataHandler::load_all_data(
    const std::map<std::string, std::string> &csv_files) {}
