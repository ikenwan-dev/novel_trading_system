#pragma once
#include "../LimitOrderBookConcept.h"
#include "Common/Containers/FlatHashMap.h"
#include "Common/Containers/ObjectPool.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <databento/enums.hpp>
#include <databento/record.hpp>
#include <stdexcept>
#include <utility>

namespace backtesting_engine::mbo {

/**
 * @brief Zero-Allocation, O(1) Limit Order Book using a Bitwise Circular Array.
 *
 * Employs a power-of-2 array mapped via (price / TickSize) & (CAPACITY - 1)
 * to guarantee strict O(1) lookups, insertions, and memory bounds safety
 * without branching or hardware modulo.
 */
template <int64_t TickSize> class DirectArrayLOB {
public:
  static constexpr size_t MAX_ORDERS = 1000000;
  static constexpr size_t ID_MAP_CAPACITY = 2097152; // 2^21
  static constexpr size_t LOB_CAPACITY =
      1048576; // 2^20 (covers $10k range at 1-cent tick)

  DirectArrayLOB();

  struct alignas(64) OrderNode {
    databento::MboMsg msg;
    int32_t next_idx = -1;
    int32_t prev_idx = -1;
  };

  struct PriceLevel {
    int64_t price = 0; // The actual price at this level (for BBO reporting and
                       // hash collision checks)
    uint64_t total_qty = 0;
    int32_t head_idx = -1; // FIFO head
    int32_t tail_idx = -1; // FIFO tail
  };

  void update_book(const databento::MboMsg &msg);
  std::pair<int64_t, int64_t> get_bbo() const;
  uint64_t get_level_qty(databento::Side side, int64_t price) const;
  uint64_t get_total_volume() const;

private:
  // Cached BBO to avoid scanning when the book is stable
  mutable int64_t best_bid_ = databento::kUndefPrice;
  mutable int64_t best_ask_ = databento::kUndefPrice;

  // Pools
  common::ObjectPool<OrderNode, MAX_ORDERS> order_pool_;
  common::FlatHashMap<uint64_t, int32_t, ID_MAP_CAPACITY> id_map_;

  // O(1) Circular Arrays
  std::array<PriceLevel, LOB_CAPACITY> bids_{};
  std::array<PriceLevel, LOB_CAPACITY> asks_{};

  // Highly optimized bitwise modulo function. Division by constant will be
  // optimized.
  inline size_t price_to_index(int64_t price) const {
    return static_cast<size_t>((price / TickSize) & (LOB_CAPACITY - 1));
  }

  void add_order(const databento::MboMsg &msg);
  void cancel_order(const databento::MboMsg &msg);
  void modify_order(const databento::MboMsg &msg);
  void clear_book();

  std::array<PriceLevel, LOB_CAPACITY> &get_side_array(databento::Side side);
  const std::array<PriceLevel, LOB_CAPACITY> &
  get_side_array(databento::Side side) const;

  void recompute_best_bid();
  void recompute_best_ask();
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

template <int64_t TickSize> inline DirectArrayLOB<TickSize>::DirectArrayLOB() {
  // Pre-fault order_pool pages and break OS Zero-Page COW by WRITING to memory
  for (size_t i = 0; i < MAX_ORDERS; i += 128) {
    if (i < order_pool_.capacity()) {
      static_cast<volatile int32_t &>(order_pool_[i].prev_idx) = -1;
    }
  }

  // Pre-fault LOB arrays and break OS Zero-Page COW by WRITING to memory
  for (size_t i = 0; i < LOB_CAPACITY; i += 128) {
    static_cast<volatile uint64_t &>(bids_[i].total_qty) = 0;
    static_cast<volatile uint64_t &>(asks_[i].total_qty) = 0;
  }
}

template <int64_t TickSize>
inline void
DirectArrayLOB<TickSize>::update_book(const databento::MboMsg &msg) {
  switch (msg.action) {
  case databento::Action::Add:
    add_order(msg);
    break;
  case databento::Action::Modify:
    modify_order(msg);
    break;
  case databento::Action::Cancel:
    cancel_order(msg);
    break;
  case databento::Action::Clear:
    clear_book();
    break;
  default:
    break; // Trade/Fill doesn't affect resting liquidity here
  }
}

template <int64_t TickSize>
inline std::pair<int64_t, int64_t> DirectArrayLOB<TickSize>::get_bbo() const {
  return {best_bid_, best_ask_};
}

template <int64_t TickSize>
inline uint64_t DirectArrayLOB<TickSize>::get_level_qty(databento::Side side,
                                                        int64_t price) const {
  const auto &arr = get_side_array(side);
  size_t idx = price_to_index(price);

  if (arr[idx].price == price) {
    return arr[idx].total_qty;
  }
  return 0;
}

template <int64_t TickSize>
inline uint64_t DirectArrayLOB<TickSize>::get_total_volume() const {
  uint64_t total = 0;
  for (const auto &level : bids_)
    total += level.total_qty;
  for (const auto &level : asks_)
    total += level.total_qty;
  return total;
}

template <int64_t TickSize>
inline void DirectArrayLOB<TickSize>::add_order(const databento::MboMsg &msg) {
  if (msg.flags.IsTob()) {
    if (msg.price == databento::kUndefPrice) {
      clear_book();
      return;
    }
  }

  auto &arr = get_side_array(msg.side);
  size_t idx = price_to_index(msg.price);
  PriceLevel &level = arr[idx];

  // Prevent silent LOB corruption: Crash if a bucket collision occurs.
  if (level.total_qty > 0 && level.price != msg.price) {
    throw std::runtime_error(
        "DirectArrayLOB Collision: Price span exceeded LOB_CAPACITY!");
  }

  if (level.total_qty == 0) {
    level.price = msg.price;
  }

  int32_t node_idx = order_pool_.allocate();
  OrderNode &node = order_pool_[node_idx];
  node.msg = msg;
  node.next_idx = -1;
  node.prev_idx = level.tail_idx;

  if (level.head_idx == -1) {
    level.head_idx = node_idx;
  } else {
    order_pool_[level.tail_idx].next_idx = node_idx;
  }
  level.tail_idx = node_idx;
  level.total_qty += msg.size;

  id_map_.insert(msg.order_id, node_idx);

  if (msg.side == databento::Side::Bid) {
    if (best_bid_ == databento::kUndefPrice || msg.price > best_bid_) {
      best_bid_ = msg.price;
    }
  } else {
    if (best_ask_ == databento::kUndefPrice || msg.price < best_ask_) {
      best_ask_ = msg.price;
    }
  }
}

template <int64_t TickSize>
inline void
DirectArrayLOB<TickSize>::cancel_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1)
    return;

  OrderNode &node = order_pool_[node_idx];
  auto &arr = get_side_array(node.msg.side);
  size_t idx = price_to_index(node.msg.price);
  PriceLevel &level = arr[idx];

  uint32_t cancel_qty = std::min(node.msg.size, msg.size);
  node.msg.size -= cancel_qty;
  level.total_qty -= cancel_qty;

  if (node.msg.size == 0) {
    if (node.prev_idx != -1) {
      order_pool_[node.prev_idx].next_idx = node.next_idx;
    } else {
      level.head_idx = node.next_idx;
    }

    if (node.next_idx != -1) {
      order_pool_[node.next_idx].prev_idx = node.prev_idx;
    } else {
      level.tail_idx = node.prev_idx;
    }

    id_map_.erase(msg.order_id);
    order_pool_.deallocate(node_idx);

    if (level.total_qty == 0) {
      level.head_idx = -1;
      level.tail_idx = -1;

      if (node.msg.side == databento::Side::Bid &&
          node.msg.price == best_bid_) {
        recompute_best_bid();
      } else if (node.msg.side == databento::Side::Ask &&
                 node.msg.price == best_ask_) {
        recompute_best_ask();
      }
    }
  }
}

template <int64_t TickSize>
inline void
DirectArrayLOB<TickSize>::modify_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1) {
    add_order(msg);
    return;
  }

  OrderNode &node = order_pool_[node_idx];

  if (node.msg.price != msg.price) {
    databento::MboMsg cancel_msg = node.msg;
    cancel_msg.action = databento::Action::Cancel;
    cancel_order(cancel_msg);
    add_order(msg);
  } else {
    auto &arr = get_side_array(node.msg.side);
    size_t idx = price_to_index(node.msg.price);
    PriceLevel &level = arr[idx];

    if (msg.size > node.msg.size) {
      level.total_qty -= node.msg.size;
      level.total_qty += msg.size;
      node.msg.size = msg.size;

      // Lose queue priority if size increases: move node to the back (tail)
      if (node_idx != level.tail_idx) {
        // 1. Unlink from current position
        if (node.prev_idx != -1) {
          order_pool_[node.prev_idx].next_idx = node.next_idx;
        } else {
          level.head_idx = node.next_idx;
        }

        if (node.next_idx != -1) {
          order_pool_[node.next_idx].prev_idx = node.prev_idx;
        } else {
          level.tail_idx = node.prev_idx;
        }

        // 2. Re-link at the tail
        node.next_idx = -1;
        node.prev_idx = level.tail_idx;

        if (level.tail_idx != -1) {
          order_pool_[level.tail_idx].next_idx = node_idx;
        } else {
          level.head_idx = node_idx;
        }
        level.tail_idx = node_idx;
      }
    } else {
      level.total_qty -= (node.msg.size - msg.size);
      node.msg.size = msg.size;
    }
  }
}

template <int64_t TickSize> inline void DirectArrayLOB<TickSize>::clear_book() {
  for (auto &level : bids_) {
    level.total_qty = 0;
    level.head_idx = -1;
    level.tail_idx = -1;
  }
  for (auto &level : asks_) {
    level.total_qty = 0;
    level.head_idx = -1;
    level.tail_idx = -1;
  }
  id_map_.clear();
  best_bid_ = databento::kUndefPrice;
  best_ask_ = databento::kUndefPrice;
}

template <int64_t TickSize>
inline std::array<typename DirectArrayLOB<TickSize>::PriceLevel,
                  DirectArrayLOB<TickSize>::LOB_CAPACITY> &
DirectArrayLOB<TickSize>::get_side_array(databento::Side side) {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

template <int64_t TickSize>
inline const std::array<typename DirectArrayLOB<TickSize>::PriceLevel,
                        DirectArrayLOB<TickSize>::LOB_CAPACITY> &
DirectArrayLOB<TickSize>::get_side_array(databento::Side side) const {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

template <int64_t TickSize>
inline void DirectArrayLOB<TickSize>::recompute_best_bid() {
  int64_t current_best = best_bid_;
  best_bid_ = databento::kUndefPrice;

  for (int i = 1; i < 10000; ++i) {
    int64_t candidate_price = current_best - (i * TickSize);
    size_t idx = price_to_index(candidate_price);
    if (bids_[idx].total_qty > 0 && bids_[idx].price == candidate_price) {
      best_bid_ = candidate_price;
      return;
    }
  }
}

template <int64_t TickSize>
inline void DirectArrayLOB<TickSize>::recompute_best_ask() {
  int64_t current_best = best_ask_;
  best_ask_ = databento::kUndefPrice;

  for (int i = 1; i < 10000; ++i) {
    int64_t candidate_price = current_best + (i * TickSize);
    size_t idx = price_to_index(candidate_price);
    if (asks_[idx].total_qty > 0 && asks_[idx].price == candidate_price) {
      best_ask_ = candidate_price;
      return;
    }
  }
}

static_assert(LimitOrderBookConcept<DirectArrayLOB<10000000>>,
              "DirectArrayLOB fails to implement LimitOrderBookConcept!");

} // namespace backtesting_engine::mbo
