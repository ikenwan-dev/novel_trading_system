#pragma once
#include "Constants/Constants.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/historical.hpp>
#include <string>
#include <vector>


namespace backtesting_engine::mbo {
class DataBentoProducer {
public:
  using Interactor =
      common::IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoProducer(const std::vector<std::string> filepaths,
                    Interactor &writer);
  ~DataBentoProducer() = default;

  DataBentoProducer(const DataBentoProducer &) = delete;
  DataBentoProducer &operator=(const DataBentoProducer &) = delete;
  DataBentoProducer(DataBentoProducer &&) = delete;
  DataBentoProducer &operator=(DataBentoProducer &&) = delete;

  void produce();

private:
  std::vector<std::string> mbo_filepaths_;
  Interactor &writer_;
};
} // namespace backtesting_engine::mbo
