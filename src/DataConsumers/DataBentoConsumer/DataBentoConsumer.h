#pragma once

#include "Constants/Constants.h"
#include "DataConsumers/DataConsumer.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/historical.hpp>

class DataBentoConsumer : public DataConsumer {
public:
  using Interactor =
      IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoConsumer(Interactor &reader);
  ~DataBentoConsumer() = default;

  DataBentoConsumer(const DataBentoConsumer &) = delete;
  DataBentoConsumer &operator=(const DataBentoConsumer &) = delete;
  DataBentoConsumer(DataBentoConsumer &&) = delete;
  DataBentoConsumer &operator=(DataBentoConsumer &&) = delete;

  void consume() override;

private:
  Interactor &reader_;
};