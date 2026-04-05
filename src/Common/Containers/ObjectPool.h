#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace backtesting_engine::common {

/**
 * @brief A high-performance, zero-allocation Object Pool.
 * 
 * Pre-allocates a fixed number of objects on the heap (or as part of the class)
 * and manages them using a Free List. This eliminates heap allocations in the 
 * hot path, providing deterministic O(1) allocation and deallocation.
 * 
 * @tparam T The type of object to store.
 * @tparam Capacity The maximum number of objects in the pool.
 */
template <typename T, size_t Capacity>
class ObjectPool {
public:
  ObjectPool() {
    // Initialize the free list with all available indices.
    // We store indices in reverse order so that popping from the back 
    // gives us indices in increasing order (0, 1, 2...), which is slightly
    // better for initial cache locality.
    free_indices_.reserve(Capacity);
    for (int32_t i = static_cast<int32_t>(Capacity) - 1; i >= 0; --i) {
      free_indices_.push_back(i);
    }
  }

  /**
   * @brief Acquires a free object from the pool.
   * 
   * @return int32_t The index of the allocated object in the pool.
   * @throws std::runtime_error If the pool is exhausted.
   */
  int32_t allocate() {
    if (free_indices_.empty()) {
      throw std::runtime_error("ObjectPool exhausted: Capacity " + std::to_string(Capacity));
    }
    int32_t index = free_indices_.back();
    free_indices_.pop_back();
    return index;
  }

  /**
   * @brief Returns an object to the pool, making it available for reuse.
   * 
   * @param index The index of the object to deallocate.
   */
  void deallocate(int32_t index) {
    // In a production system, we might want to check if the index is already free
    // or out of bounds, but in ultra-low latency code, we skip these checks
    // if we trust the calling logic.
    free_indices_.push_back(index);
  }

  /**
   * @brief Direct access to an object by its index.
   */
  T& operator[](int32_t index) {
    return pool_[index];
  }

  /**
   * @brief Direct constant access to an object by its index.
   */
  const T& operator[](int32_t index) const {
    return pool_[index];
  }

  size_t available() const { return free_indices_.size(); }
  size_t capacity() const { return Capacity; }

private:
  std::array<T, Capacity> pool_;
  std::vector<int32_t> free_indices_;
};

} // namespace backtesting_engine::common
