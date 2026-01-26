#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include "SharedMemory/SharedRingBuffer.h"
#include <algorithm>
#include <databento/historical.hpp>

DataBentoProducer::DataBentoProducer(const std::string &shm_name,
                                     const std::vector<std::string> filepaths)
    : DataProducer(shm_name,
                   sizeof(SharedRingBuffer<databento::MboMsg, 65536>)),
      mbo_filepaths(filepaths), file_index(0) {
  std::sort(mbo_filepaths.begin(), mbo_filepaths.end());
}

void DataBentoProducer::run() {
  std::for_each(
      mbo_filepaths.begin(), mbo_filepaths.end(),
      [](const std::string &filepath) { std::cout << filepath << "\n"; });
}