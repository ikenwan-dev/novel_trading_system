#pragma once
#include "../../Events/Event.h"
#include "../../ThreadSafeQueue/ThreadSafeQueue.h"
#include "../DataHandler.h"
#include <map>
#include <vector>

struct Bar; // Forward declaration. TODO: Make sure to implement bar

class HistoricCSVDataHandler : public DataHandler {
public:
  HistoricCSVDataHandler(ThreadSafeQueue<std::shared_ptr<Event>> &queue,
                         const std::map<std::string, std::string> &csv_files);

  void update() override;
  bool is_running() const override;

private:
  void load_all_data();

  ThreadSafeQueue<std::shared_ptr<Event>> &event_queue_;

  // Map of ticker to vector of bar data. Sorted in ascending order by timestamp
  std::map<std::string, std::vector<Bar>> all_data_;

  // Map of ticker to current index in all_data_
  std::map<std::string, size_t> current_index_;

  bool is_running_ = true;
};
