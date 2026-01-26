#pragma once
#include <string>
#include <vector>
#include "DataProducers/DataProducer.h"

class DataBentoProducer : public DataProducer {
public:
  DataBentoProducer(const std::string &shm_name, const std::vector<std::string> mbo_filepaths);

  void run() override;
};