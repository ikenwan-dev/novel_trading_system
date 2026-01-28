#pragma once
#include "Constants/Constants.h"
#include "DataProducers/DataProducer.h"
#include "SharedMemory/IPCWriter.h"
#include <databento/historical.hpp>
#include <string>
#include <vector>

class DataBentoProducer : public DataProducer {
public:
  using Writer = IPCWriter<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

  DataBentoProducer(const std::vector<std::string> filepaths, Writer &writer);
  ~DataBentoProducer();

  DataBentoProducer(const DataBentoProducer &) = delete;
  DataBentoProducer &operator=(const DataBentoProducer &) = delete;
  DataBentoProducer(DataBentoProducer &&) = delete;
  DataBentoProducer &operator=(DataBentoProducer &&) = delete;

  void run() override;

private:
  std::vector<std::string> mbo_filepaths_;
  Writer &writer_;
};