#pragma once

#include "Constants/Constants.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/historical.hpp>

namespace backtesting_engine::mbo {
class DataBentoIPCConsumer {
public:
  using Interactor =
      common::IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoIPCConsumer(Interactor &reader);
  ~DataBentoIPCConsumer() = default;

  DataBentoIPCConsumer(const DataBentoIPCConsumer &) = delete;
  DataBentoIPCConsumer &operator=(const DataBentoIPCConsumer &) = delete;
  DataBentoIPCConsumer(DataBentoIPCConsumer &&) = delete;
  DataBentoIPCConsumer &operator=(DataBentoIPCConsumer &&) = delete;

  bool try_poll(databento::MboMsg &out_msg);

private:
  Interactor &reader_;
  DataBentoLOB lob_;
};
} // namespace backtesting_engine::mbo
