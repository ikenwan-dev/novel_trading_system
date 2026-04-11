#include "DirectArrayLOB.h"
#include <algorithm>

namespace backtesting_engine::mbo {

DirectArrayLOB::DirectArrayLOB(int64_t tick_size) : tick_size_(tick_size) {
  // Pre-fault order_pool pages to prevent OS page faults on hot path
  for (size_t i = 0; i < MAX_ORDERS; i += 128) {
    if (i < order_pool_.capacity()) {
      volatile auto touch = order_pool_[i].prev_idx;
      (void)touch;
    }
  }

  // Pre-fault LOB arrays
  for (size_t i = 0; i < LOB_CAPACITY; i += 128) {
    volatile auto b_touch = bids_[i].total_qty;
    volatile auto a_touch = asks_[i].total_qty;
    (void)b_touch;
    (void)a_touch;
  }
}

void DirectArrayLOB::update_book(const databento::MboMsg &msg) {
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

std::pair<int64_t, int64_t> DirectArrayLOB::get_bbo() const {
  return {best_bid_, best_ask_};
}

uint64_t DirectArrayLOB::get_level_qty(databento::Side side, int64_t price) const {
  const auto& arr = get_side_array(side);
  size_t idx = price_to_index(price);
  
  // Hash collision check (rare, but mathematically possible if the market swings wildly)
  if (arr[idx].price == price) {
    return arr[idx].total_qty;
  }
  return 0;
}

uint64_t DirectArrayLOB::get_total_volume() const {
  uint64_t total = 0;
  for (const auto& level : bids_) total += level.total_qty;
  for (const auto& level : asks_) total += level.total_qty;
  return total;
}

void DirectArrayLOB::add_order(const databento::MboMsg &msg) {
  if (msg.flags.IsTob()) {
    // If it's a TOB message that clears the book, we just wipe the caches
    // In a real array implementation, fully zeroing memory may be slow,
    // so we typically just reset BBO trackers or zero the specific levels.
    if (msg.price == databento::kUndefPrice) {
      clear_book();
      return; 
    }
  }

  auto& arr = get_side_array(msg.side);
  size_t idx = price_to_index(msg.price);
  PriceLevel& level = arr[idx];

  // Set the price if it's a freshly populated bucket
  if (level.total_qty == 0) {
    level.price = msg.price;
  }

  // O(1) Allocation
  int32_t node_idx = order_pool_.allocate();
  OrderNode& node = order_pool_[node_idx];
  node.msg = msg;
  node.next_idx = -1;
  node.prev_idx = level.tail_idx;

  // Intrusive queue linkage
  if (level.head_idx == -1) {
    level.head_idx = node_idx;
  } else {
    order_pool_[level.tail_idx].next_idx = node_idx;
  }
  level.tail_idx = node_idx;
  level.total_qty += msg.size;

  id_map_.insert(msg.order_id, node_idx);

  // Update BBO cache instantly if price is more aggressive
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

void DirectArrayLOB::cancel_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1) return; 

  OrderNode& node = order_pool_[node_idx];
  auto& arr = get_side_array(node.msg.side);
  size_t idx = price_to_index(node.msg.price);
  PriceLevel& level = arr[idx];

  uint32_t cancel_qty = std::min(node.msg.size, msg.size);
  node.msg.size -= cancel_qty;
  level.total_qty -= cancel_qty;

  if (node.msg.size == 0) {
    // Unlink O(1)
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

    // If level is completely depleted, check if it was our BBO
    if (level.total_qty == 0) { // Safety check
      level.head_idx = -1;
      level.tail_idx = -1;
      // level.price left as is since qty is 0. 

      if (node.msg.side == databento::Side::Bid && node.msg.price == best_bid_) {
        recompute_best_bid();
      } else if (node.msg.side == databento::Side::Ask && node.msg.price == best_ask_) {
        recompute_best_ask();
      }
    }
  }
}

void DirectArrayLOB::modify_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1) {
    add_order(msg);
    return;
  }

  OrderNode& node = order_pool_[node_idx];
  
  if (node.msg.price != msg.price) {
    databento::MboMsg cancel_msg = node.msg;
    cancel_msg.action = databento::Action::Cancel;
    cancel_order(cancel_msg);
    add_order(msg);
  } else {
    auto& arr = get_side_array(node.msg.side);
    size_t idx = price_to_index(node.msg.price);
    PriceLevel& level = arr[idx];
    
    if (msg.size > node.msg.size) {
      level.total_qty -= node.msg.size;
      level.total_qty += msg.size;
      node.msg.size = msg.size;
    } else {
      level.total_qty -= (node.msg.size - msg.size);
      node.msg.size = msg.size;
    }
  }
}

void DirectArrayLOB::clear_book() {
  for (auto& level : bids_) { level.total_qty = 0; level.head_idx = -1; level.tail_idx = -1; }
  for (auto& level : asks_) { level.total_qty = 0; level.head_idx = -1; level.tail_idx = -1; }
  id_map_.clear();
  best_bid_ = databento::kUndefPrice;
  best_ask_ = databento::kUndefPrice;
}

std::array<DirectArrayLOB::PriceLevel, DirectArrayLOB::LOB_CAPACITY>& DirectArrayLOB::get_side_array(databento::Side side) {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

const std::array<DirectArrayLOB::PriceLevel, DirectArrayLOB::LOB_CAPACITY>& DirectArrayLOB::get_side_array(databento::Side side) const {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

void DirectArrayLOB::recompute_best_bid() {
  // TODO: Implement the highly optimized __builtin_clzll() multi-level 
  // hierarchical bitmask traverse here for true O(1) resolution.
  
  // Standard linear depletion traverse (O(K))
  int64_t current_best = best_bid_;
  best_bid_ = databento::kUndefPrice;

  // We walk down tick by tick until we find volume or reach some sane limit
  // Note: Unbounded walking downwards can be risky if book is completely empty. 
  // In a robust implementation, keep track of "min_bid" or total volume.
  for (int i = 1; i < 10000; ++i) { 
    int64_t candidate_price = current_best - (i * tick_size_);
    size_t idx = price_to_index(candidate_price);
    if (bids_[idx].total_qty > 0 && bids_[idx].price == candidate_price) {
      best_bid_ = candidate_price;
      return;
    }
  }
}

void DirectArrayLOB::recompute_best_ask() {
  // TODO: Hierarchical Bitmask.
  
  int64_t current_best = best_ask_;
  best_ask_ = databento::kUndefPrice;

  for (int i = 1; i < 10000; ++i) { 
    int64_t candidate_price = current_best + (i * tick_size_);
    size_t idx = price_to_index(candidate_price);
    if (asks_[idx].total_qty > 0 && asks_[idx].price == candidate_price) {
      best_ask_ = candidate_price;
      return;
    }
  }
}

} // namespace backtesting_engine::mbo
