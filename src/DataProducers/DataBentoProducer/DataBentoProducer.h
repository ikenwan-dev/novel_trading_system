#pragma once
#include "Constants/Constants.h"
#include "DataProducers/DataProducer.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/historical.hpp>
#include <string>
#include <vector>


namespace backtesting_engine {
class DataBentoProducer : public DataProducer {
public:
  using Interactor =
      IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoProducer(const std::vector<std::string> filepaths,
                    Interactor &writer);
  ~DataBentoProducer() = default;

  DataBentoProducer(const DataBentoProducer &) = delete;
  DataBentoProducer &operator=(const DataBentoProducer &) = delete;
  DataBentoProducer(DataBentoProducer &&) = delete;
  DataBentoProducer &operator=(DataBentoProducer &&) = delete;

  void produce() override;

private:
  std::vector<std::string> mbo_filepaths_;
  Interactor &writer_;
};
} // namespace backtesting_engine
