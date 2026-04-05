#pragma once
#include "Common/Containers/FlatHashMap.h"
#include "Common/Containers/ObjectPool.h"
#include <cstdint>
#include <databento/enums.hpp>
#include <databento/record.hpp>
#include <vector>
#include <utility>

namespace backtesting_engine::mbo {

/**
 * @brief An Optimized, Zero-Allocation Limit Order Book for Databento MBO data.
 * 
 * Uses an Intrusive OrderPool for FIFO order management and a custom 
 * FlatHashMap for fast Order ID lookups. This implementation avoids all 
 * heap allocations in the hot path.
 */
class OptimizedDataBentoLOB {
public:
  // --- Constants for pre-allocation ---
  static constexpr size_t MAX_ORDERS = 1000000;
  static constexpr size_t ID_MAP_CAPACITY = 2097152; // Power of 2 (2^21)

  struct OrderNode {
    databento::MboMsg msg;
    int32_t next_idx = -1;
    int32_t prev_idx = -1;
  };

  struct PriceLevel {
    int64_t price;
    uint64_t total_qty = 0;
    int32_t head_idx = -1; // FIFO head
    int32_t tail_idx = -1; // FIFO tail
  };

  void update_book(const databento::MboMsg &msg);
  std::pair<int64_t, int64_t> get_bbo() const;
  uint64_t get_level_qty(databento::Side side, int64_t price) const;
  uint64_t get_total_volume() const;

private:
  // Core Containers (Pre-allocated)
  common::ObjectPool<OrderNode, MAX_ORDERS> order_pool_;
  common::FlatHashMap<uint64_t, int32_t, ID_MAP_CAPACITY> id_map_;

  // Price levels kept sorted in contiguous vectors (simulating std::flat_map)
  std::vector<PriceLevel> bids_; // Sorted DESCENDING
  std::vector<PriceLevel> asks_; // Sorted ASCENDING

  void add_order(const databento::MboMsg &msg);
  void cancel_order(const databento::MboMsg &msg);
  void modify_order(const databento::MboMsg &msg);
  void clear_book();

  std::vector<PriceLevel>& get_side(databento::Side side);
  const std::vector<PriceLevel>& get_side(databento::Side side) const;
  
  // Helper for O(log N) price level lookup
  PriceLevel* find_price_level(std::vector<PriceLevel>& side, int64_t price, bool is_bid);
};

} // namespace backtesting_engine::mbo
