#pragma once
#include "Constants/Constants.h"
#include "DataProducers/DataProducer.h"
#include "SharedMemory/SharedRingBuffer.h"
#include <databento/historical.hpp>
#include <string>
#include <vector>

using RingBuffer =
    SharedRingBuffer<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

class DataBentoProducer : public DataProducer {
public:
  DataBentoProducer(const std::string &shm_name,
                    const std::vector<std::string> filepaths);
  ~DataBentoProducer();

  DataBentoProducer(const DataBentoProducer &) = delete;
  DataBentoProducer &operator=(const DataBentoProducer &) = delete;
  DataBentoProducer(DataBentoProducer &&) = delete;
  DataBentoProducer &operator=(DataBentoProducer &&) = delete;

  void run() override;

private:
  std::vector<std::string> mbo_filepaths_;
  RingBuffer *ring_buffer_;
};