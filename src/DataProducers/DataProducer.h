#pragma once

class DataProducer {
public:
  virtual void run() = 0;
  virtual ~DataProducer() = default;

  DataProducer(const DataProducer &) = delete;
  DataProducer &operator=(const DataProducer &) = delete;
  DataProducer(DataProducer &&) = delete;
  DataProducer &operator=(DataProducer &&) = delete;

protected:
  DataProducer() = default;
};