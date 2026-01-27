#pragma once
#include "SharedMemory/SharedMemory.h"
#include <iostream>

class DataProducer {
public:
  DataProducer(const std::string &shm_name, size_t shm_size)
      : shm_(shm_name, shm_size, true) {}
  virtual void run() = 0;
  virtual ~DataProducer() {
    std::cout << "Unlinking shared memory...\n";
    shm_.unlink();
  }

  DataProducer(const DataProducer &) = delete;
  DataProducer &operator=(const DataProducer &) = delete;
  DataProducer(DataProducer &&) = delete;
  DataProducer &operator=(DataProducer &&) = delete;

protected:
  SharedMemory shm_;
};