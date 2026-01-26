#pragma once
#include "DataProducers/DataProducer.h"
#include <string>
#include <vector>

class DataBentoProducer : public DataProducer {
public:
  DataBentoProducer(const std::string &shm_name,
                    const std::vector<std::string> filepaths);

  void run() override;

private:
  std::vector<std::string> mbo_filepaths;
  std::size_t file_index;

  // databento::DbnFileStore dbnFileStore;
};