#pragma once
#include "SharedMemory/SharedMemory.h"

class DataProducer {
public:
  DataProducer(const std::string &shm_name, size_t shm_size)
      : shm(shm_name, shm_size) {}
  virtual void run() = 0;

protected:
  SharedMemory shm;
};