#pragma once
#include "Common/Containers/FlatHashMap.h"
#include "Common/Containers/ObjectPool.h"
#include <array>
#include <cstdint>
#include <databento/enums.hpp>
#include <databento/record.hpp>
#include <utility>

namespace backtesting_engine::mbo {

/**
 * @brief Zero-Allocation, O(1) Limit Order Book using a Bitwise Circular Array.
 *
 * Employs a power-of-2 array mapped via (price / tick_size) & (CAPACITY - 1)
 * to guarantee strict O(1) lookups, insertions, and memory bounds safety
 * without branching or hardware modulo.
 */
class DirectArrayLOB {
public:
  static constexpr size_t MAX_ORDERS = 1000000;
  static constexpr size_t ID_MAP_CAPACITY = 2097152; // 2^21
  static constexpr size_t LOB_CAPACITY = 131072;     // Power of 2 for bitmask math

  /**
   * @param tick_size The fixed-point tick size (e.g., 0.25 * 1e9 = 250000000)
   */
  explicit DirectArrayLOB(int64_t tick_size);

  struct alignas(64) OrderNode {
    databento::MboMsg msg;
    int32_t next_idx = -1;
    int32_t prev_idx = -1;
  };

  struct PriceLevel {
    int64_t price = 0; // The actual price at this level (for BBO reporting and hash collision checks)
    uint64_t total_qty = 0;
    int32_t head_idx = -1; // FIFO head
    int32_t tail_idx = -1; // FIFO tail
  };

  void update_book(const databento::MboMsg &msg);
  std::pair<int64_t, int64_t> get_bbo() const;
  uint64_t get_level_qty(databento::Side side, int64_t price) const;
  uint64_t get_total_volume() const;

private:
  const int64_t tick_size_;

  // Cached BBO to avoid scanning when the book is stable
  mutable int64_t best_bid_ = databento::kUndefPrice;
  mutable int64_t best_ask_ = databento::kUndefPrice;

  // Pools
  common::ObjectPool<OrderNode, MAX_ORDERS> order_pool_;
  common::FlatHashMap<uint64_t, int32_t, ID_MAP_CAPACITY> id_map_;

  // O(1) Circular Arrays
  std::array<PriceLevel, LOB_CAPACITY> bids_{};
  std::array<PriceLevel, LOB_CAPACITY> asks_{};

  // Highly optimized bitwise modulo function. O(1) -> 1 Cycle.
  inline size_t price_to_index(int64_t price) const {
    return static_cast<size_t>((price / tick_size_) & (LOB_CAPACITY - 1));
  }

  void add_order(const databento::MboMsg &msg);
  void cancel_order(const databento::MboMsg &msg);
  void modify_order(const databento::MboMsg &msg);
  void clear_book();

  std::array<PriceLevel, LOB_CAPACITY> &get_side_array(databento::Side side);
  const std::array<PriceLevel, LOB_CAPACITY> &get_side_array(databento::Side side) const;

  void recompute_best_bid();
  void recompute_best_ask();
};

} // namespace backtesting_engine::mbo
