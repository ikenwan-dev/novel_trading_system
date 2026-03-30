#pragma once


#include <databento/historical.hpp>

namespace backtesting_engine::mbo {
class DataConsumer {
public:
  virtual bool try_poll(databento::MboMsg &out_msg) = 0;
  virtual ~DataConsumer() = default;

  DataConsumer(const DataConsumer &) = delete;
  DataConsumer &operator=(const DataConsumer &) = delete;
  DataConsumer(DataConsumer &&) = delete;
  DataConsumer &operator=(DataConsumer &&) = delete;

protected:
  DataConsumer() = default;
};
} // namespace backtesting_engine::mbo
