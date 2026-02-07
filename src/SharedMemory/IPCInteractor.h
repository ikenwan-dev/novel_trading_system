#pragma once
#include "SharedMemory/SharedMemory.h"
#include "SharedMemory/SharedRingBuffer.h"
#include <iostream>
#include <string>

template <typename T, size_t Capacity> class IPCInteractor {
  using RingBuffer = SharedRingBuffer<T, Capacity>;

public:
  IPCInteractor(const std::string &shm_name, const bool create = true)
      : shm_(shm_name, sizeof(RingBuffer), create) {
    if (create) {
      ring_buffer_ = new (shm_.get_ptr()) RingBuffer();
    } else {
      ring_buffer_ = static_cast<RingBuffer *>(shm_.get_ptr());
    }
  }

  ~IPCInteractor() {
    if (ring_buffer_) {
      ring_buffer_->~RingBuffer();
    }
  }

  bool is_initialized() const { return ring_buffer_->is_initialized(); }

  void unlink() {
    std::cout << "Unlinking shared memory...\n";
    shm_.unlink();
  }

  // Delete copy/move as this owns a resource
  IPCInteractor(const IPCInteractor &) = delete;
  IPCInteractor &operator=(const IPCInteractor &) = delete;
  IPCInteractor(IPCInteractor &&) = delete;
  IPCInteractor &operator=(IPCInteractor &&) = delete;

  // Push item into the ring buffer. Can only be called by the producer.
  bool push(const T &item) { return ring_buffer_->push(item); }

  // Pop item from the ring buffer. Can only be called by the consumer.
  bool pop(T &item) { return ring_buffer_->pop(item); }

private:
  SharedMemory shm_;
  RingBuffer *ring_buffer_;
};
