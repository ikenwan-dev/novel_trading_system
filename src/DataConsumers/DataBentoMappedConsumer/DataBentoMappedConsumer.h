#pragma once
#include <databento/historical.hpp>
#include <string>
#include <vector>

namespace backtesting_engine::mbo {

class DataBentoMappedConsumer {
public:
  DataBentoMappedConsumer(const std::vector<std::string> &filepaths);
  ~DataBentoMappedConsumer();

  DataBentoMappedConsumer(const DataBentoMappedConsumer &) = delete;
  DataBentoMappedConsumer &operator=(const DataBentoMappedConsumer &) = delete;

  bool try_poll(databento::MboMsg &out_msg);

private:
  void open_next_file();
  void unmap_current();

  std::vector<std::string> filepaths_;
  size_t current_file_index_;
  int fd_;
  size_t mapped_size_;
  const char *mmap_base_;
  const char *current_ptr_;
};

} // namespace backtesting_engine::mbo
