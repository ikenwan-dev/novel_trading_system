#pragma once
#include "SharedMemory/SharedMemory.h"

class DataProducer {
public:
  virtual void run() = 0;

protected:
  SharedMemory shm;
};