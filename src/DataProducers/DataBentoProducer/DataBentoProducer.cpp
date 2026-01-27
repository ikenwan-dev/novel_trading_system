#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include <algorithm>

DataBentoProducer::DataBentoProducer(const std::string &shm_name,
                                     const std::vector<std::string> filepaths)
    : DataProducer(shm_name, sizeof(RingBuffer)), mbo_filepaths_(filepaths),
      ring_buffer_(new(shm_.get_ptr()) RingBuffer()) {
  std::sort(mbo_filepaths_.begin(), mbo_filepaths_.end());
}

DataBentoProducer::~DataBentoProducer() {
  if (ring_buffer_) {
    ring_buffer_->~RingBuffer();
  }
}
void DataBentoProducer::run() {

  for (const auto &filepath : mbo_filepaths_) {
    databento::DbnFileStore dbn_file_store(filepath);
    while (const databento::Record *record = dbn_file_store.NextRecord()) {
      const auto &mbo_msg = record->Get<databento::MboMsg>();
      while (!ring_buffer_->push(mbo_msg)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }
  ring_buffer_->push(databento::MboMsg{}); // signal end of data
}