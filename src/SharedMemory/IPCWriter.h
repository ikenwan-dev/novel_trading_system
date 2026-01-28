#pragma once
#include "SharedMemory/SharedMemory.h"
#include "SharedMemory/SharedRingBuffer.h"
#include <iostream>
#include <string>

template <typename T, size_t Capacity> class IPCWriter {
  using RingBuffer = SharedRingBuffer<T, Capacity>;

public:
  IPCWriter(const std::string &shm_name, const bool create = true)
      : shm_(shm_name, sizeof(RingBuffer), create),
        ring_buffer_(new(shm_.get_ptr()) RingBuffer()) {}

  ~IPCWriter() {
    if (ring_buffer_) {
      ring_buffer_->~RingBuffer();
    }
  }

  void unlink() {
    std::cout << "Unlinking shared memory...\n";
    shm_.unlink();
  }

  // Delete copy/move as this owns a resource
  IPCWriter(const IPCWriter &) = delete;
  IPCWriter &operator=(const IPCWriter &) = delete;
  IPCWriter(IPCWriter &&) = delete;
  IPCWriter &operator=(IPCWriter &&) = delete;

  bool push(const T &item) { return ring_buffer_->push(item); }

private:
  SharedMemory shm_;
  RingBuffer *ring_buffer_;
};
