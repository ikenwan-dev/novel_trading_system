#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace backtesting_engine::common {

/**
 * @brief A high-performance, zero-allocation Linear Probing Hash Map.
 *
 * Optimized for uint64_t keys (like order_id). Uses Open Addressing with
 * Linear Probing and a power-of-two size to ensure O(1) performance and
 * perfect cache locality.
 *
 * @tparam K The key type (typically uint64_t).
 * @tparam V The value type.
 * @tparam Capacity MUST be a power of two.
 */
template <typename K, typename V, size_t Capacity> class FlatHashMap {
public:
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of two.");

  struct Entry {
    K key = 0;
    V value = -1; // Default value (e.g., -1 for index)
    bool occupied = false;
  };

  FlatHashMap() {
    // Zero-overhead initialization of the array.
  }

  /**
   * @brief Inserts or updates an entry.
   *
   * @return true if inserted, false if capacity reached.
   */
  bool insert(K key, V value) {
    size_t pos = hash(key);
    for (size_t i = 0; i < Capacity; ++i) {
      size_t idx = (pos + i) & mask_;
      if (!buckets_[idx].occupied) {
        buckets_[idx].key = key;
        buckets_[idx].value = value;
        buckets_[idx].occupied = true;
        size_++;
        return true;
      }
      if (buckets_[idx].key == key) {
        buckets_[idx].value = value;
        return true;
      }
    }
    return false; // Table full
  }

  /**
   * @brief Finds an entry by key.
   *
   * @return V the value if found, or -1 if not found.
   */
  V find(K key) const {
    size_t pos = hash(key);
    for (size_t i = 0; i < Capacity; ++i) {
      size_t idx = (pos + i) & mask_;
      if (!buckets_[idx].occupied) {
        return -1; // End of chain
      }
      if (buckets_[idx].key == key) {
        return buckets_[idx].value;
      }
    }
    return -1;
  }

  /**
   * @brief Removes an entry.
   *
   * NOTE: In a simple linear probing table, we use "Tombstones" to
   * handle deletions to prevent breaking the probe sequence.
   */
  void erase(K key) {
    size_t pos = hash(key);
    for (size_t i = 0; i < Capacity; ++i) {
      size_t idx = (pos + i) & mask_;
      if (!buckets_[idx].occupied)
        return;
      if (buckets_[idx].key == key) {
        // We'll mark it as unoccupied.
        // WARNING: Simple erasure without shifting or tombstones
        // can break linear probing. For HFT lookup maps (like order_id),
        // we often just use an "Active" flag or re-hash.
        // For simplicity and speed in this resume project,
        // we will use a "tombstone" approach.
        buckets_[idx].key = 0;
        buckets_[idx].occupied = false;

        // Re-hash the following cluster to avoid tombstones (Elite HFT trick)
        rehash_cluster(idx);
        size_--;
        return;
      }
    }
  }

  size_t size() const { return size_; }
  void clear() {
    for (auto &b : buckets_)
      b.occupied = false;
    size_ = 0;
  }

private:
  // Simple identity hash for uint64_t keys.
  // For most trading IDs, this is actually faster than more complex hashes.
  size_t hash(K key) const {
    return static_cast<size_t>(key ^ (key >> 33)); // Simple bit-mixer
  }

  // Elite HFT trick: When erasing an entry from a linear probing table,
  // you must re-insert all subsequent entries in the cluster to keep the
  // search path valid without using slow "Tombstone" markers.
  void rehash_cluster(size_t start_idx) {
    size_t i = (start_idx + 1) & mask_;
    while (buckets_[i].occupied) {
      K k = buckets_[i].key;
      V v = buckets_[i].value;
      buckets_[i].occupied = false;
      size_--; // insert will increment it back
      insert(k, v);
      i = (i + 1) & mask_;
    }
  }

  std::array<Entry, Capacity> buckets_;
  const size_t mask_ = Capacity - 1;
  size_t size_ = 0;
};

} // namespace backtesting_engine::common
