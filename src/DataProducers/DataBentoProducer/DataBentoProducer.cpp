#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include "SharedMemory/SharedRingBuffer.h"
#include <algorithm>
#include <databento/historical.hpp>
#include <memory>

DataBentoProducer::DataBentoProducer(const std::string &shm_name,
                                     const std::vector<std::string> filepaths)
    : DataProducer(
          shm_name,
          sizeof(SharedRingBuffer<databento::MboMsg, RING_BUFFER_SIZE>)),
      mbo_filepaths(filepaths), file_index(0) {
  std::sort(mbo_filepaths.begin(), mbo_filepaths.end());
}

void DataBentoProducer::run() {
  using RingBuffer = SharedRingBuffer<databento::MboMsg, RING_BUFFER_SIZE>;
  auto deleter = [](RingBuffer *p) {
    if (p) {
      p->~RingBuffer(); // Explicitly call destructor
    }
  };

  std::unique_ptr<RingBuffer, decltype(deleter)> ring_buffer(
      new (shm.get_ptr()) RingBuffer(), deleter);

  for (const auto &filepath : mbo_filepaths) {
    databento::DbnFileStore dbn_file_store(filepath);
    while (const databento::Record *record = dbn_file_store.NextRecord()) {
      const auto &mbo_msg = record->Get<databento::MboMsg>();
      while (!ring_buffer->push(mbo_msg)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }
  ring_buffer->push(databento::MboMsg{}); // signal end of data
}