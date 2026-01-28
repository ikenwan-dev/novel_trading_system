#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include <algorithm>

DataBentoProducer::DataBentoProducer(const std::vector<std::string> filepaths,
                                     Writer &writer)
    : mbo_filepaths_(filepaths), writer_(writer) {
  std::sort(mbo_filepaths_.begin(), mbo_filepaths_.end());
}

DataBentoProducer::~DataBentoProducer() {}

void DataBentoProducer::run() {
  for (const auto &filepath : mbo_filepaths_) {
    databento::DbnFileStore dbn_file_store(filepath);
    while (const databento::Record *record = dbn_file_store.NextRecord()) {
      const auto &mbo_msg = record->Get<databento::MboMsg>();
      while (!writer_.push(mbo_msg)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }
  writer_.push(databento::MboMsg{}); // signal end of data
}