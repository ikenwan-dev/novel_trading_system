#pragma once


namespace backtesting_engine {
class DataConsumer {
public:
  virtual void consume() = 0;
  virtual ~DataConsumer() = default;

  DataConsumer(const DataConsumer &) = delete;
  DataConsumer &operator=(const DataConsumer &) = delete;
  DataConsumer(DataConsumer &&) = delete;
  DataConsumer &operator=(DataConsumer &&) = delete;

protected:
  DataConsumer() = default;
};
} // namespace backtesting_engine
