#pragma once

#include "Constants/Constants.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/historical.hpp>


namespace backtesting_engine::mbo {
class DataBentoConsumer {
public:
  using Interactor =
      common::IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoConsumer(Interactor &reader);
  ~DataBentoConsumer() = default;

  DataBentoConsumer(const DataBentoConsumer &) = delete;
  DataBentoConsumer &operator=(const DataBentoConsumer &) = delete;
  DataBentoConsumer(DataBentoConsumer &&) = delete;
  DataBentoConsumer &operator=(DataBentoConsumer &&) = delete;

  bool try_poll(databento::MboMsg &out_msg);

private:
  Interactor &reader_;
  DataBentoLOB lob_;
};
} // namespace backtesting_engine::mbo
