#pragma once
#include "SharedMemory/SharedMemory.h"
#include <iostream>

class DataProducer {
public:
  DataProducer(const std::string &shm_name, size_t shm_size)
      : shm(shm_name, shm_size, true) {}
  virtual void run() = 0;
  ~DataProducer() {
    std::cout << "Unlinking shared memory...\n";
    shm.unlink();
  }

protected:
  SharedMemory shm;
};