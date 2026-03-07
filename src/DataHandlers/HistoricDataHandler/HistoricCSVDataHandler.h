#pragma once
#include "DataHandlers/DataHandler.h"
#include "DataHandlers/DataTypes/DataTypes.h"
#include "Events/Event.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <optional>
#include <unordered_map>
#include <vector>


namespace backtesting_engine {
class HistoricCSVDataHandler : public DataHandler {
public:
  HistoricCSVDataHandler(
      ThreadSafeQueue<std::shared_ptr<Event>> &queue,
      const std::unordered_map<std::string, std::string> &csv_files);

  void update() override;
  bool is_running() const override;
  std::optional<Bar>
  get_latest_price_info(const std::string &ticker) const override;

  void
  load_all_data(const std::unordered_map<std::string, std::string> &csv_files);
  static std::vector<Bar> load_stooq_file(const std::string &filename);

  ThreadSafeQueue<std::shared_ptr<Event>> &event_queue_;

  // Map of ticker to vector of bar data. Sorted in ascending order by timestamp
  std::unordered_map<std::string, std::vector<Bar>> all_data_;

  // Map of ticker to current index in all_data_
  std::unordered_map<std::string, size_t> current_index_;

  bool is_running_ = true;

private:
  static std::chrono::system_clock::time_point
  parse_stooq_datetime(const std::string &dateStr, const std::string &timeStr);
};

} // namespace backtesting_engine
