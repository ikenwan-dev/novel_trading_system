#pragma once


namespace backtesting_engine {
class DataProducer {
public:
  virtual void produce() = 0;
  virtual ~DataProducer() = default;

  DataProducer(const DataProducer &) = delete;
  DataProducer &operator=(const DataProducer &) = delete;
  DataProducer(DataProducer &&) = delete;
  DataProducer &operator=(DataProducer &&) = delete;

protected:
  DataProducer() = default;
};
} // namespace backtesting_engine
