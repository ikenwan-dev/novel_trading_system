#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T> class ThreadSafeQueue {
public:
  // Pushes a value T onto the queue
  void push(T value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(value);
    cond_var_.notify_one();
  }

  // Trys to pop a value from the queue.
  // Returns True if sucessfull, False otherwise
  bool try_pop(T &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    value = std::move(queue_.front());
    queue_.pop();
    return true;
  }

  // Blocks until a value is available and then pops it from the queue
  void wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this] { return !queue_.empty(); });
    value = std::move(queue_.front());
    queue_.pop();
  }

private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
};