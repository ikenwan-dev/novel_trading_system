#include "OptimizedDataBentoLOB.h"
#include <algorithm>

namespace backtesting_engine::mbo {

OptimizedDataBentoLOB::OptimizedDataBentoLOB() {
  bids_.reserve(100000);
  asks_.reserve(100000);

  // Pre-fault order_pool pages to prevent OS page faults on hot path
  for (size_t i = 0; i < MAX_ORDERS; i += 128) {
    if (i < order_pool_.capacity()) {
      volatile auto touch = order_pool_[i].prev_idx;
      (void)touch;
    }
  }
}

void OptimizedDataBentoLOB::update_book(const databento::MboMsg &msg) {
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
      // Other actions like Trade/Fill don't affect the LOB resting liquidity
      break;
  }
}

std::pair<int64_t, int64_t> OptimizedDataBentoLOB::get_bbo() const {
  int64_t best_bid = databento::kUndefPrice;
  // Bids sorted ASCENDING. Best is at the back.
  for (auto it = bids_.rbegin(); it != bids_.rend(); ++it) {
    if (it->total_qty > 0) {
      best_bid = it->price;
      break;
    }
  }

  int64_t best_ask = databento::kUndefPrice;
  // Asks sorted DESCENDING. Best is at the back.
  for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
    if (it->total_qty > 0) {
      best_ask = it->price;
      break;
    }
  }

  return {best_bid, best_ask};
}

uint64_t OptimizedDataBentoLOB::get_level_qty(databento::Side side, int64_t price) const {
  const auto& levels = get_side(side);
  bool is_bid = (side == databento::Side::Bid);
  
  auto it = std::lower_bound(levels.begin(), levels.end(), price, 
      [is_bid](const PriceLevel& pl, int64_t p) {
        return is_bid ? pl.price < p : pl.price > p;
      });
      
  if (it != levels.end() && it->price == price) {
    return it->total_qty;
  }
  return 0;
}

uint64_t OptimizedDataBentoLOB::get_total_volume() const {
  uint64_t total = 0;
  for (const auto& level : bids_) total += level.total_qty;
  for (const auto& level : asks_) total += level.total_qty;
  return total;
}

void OptimizedDataBentoLOB::add_order(const databento::MboMsg &msg) {
  auto& levels = get_side(msg.side);
  bool is_bid = (msg.side == databento::Side::Bid);

  // Handle TOB (Top of Book) logic if specified by Databento flags
  if (msg.flags.IsTob()) {
    levels.clear();
    if (msg.price != databento::kUndefPrice) {
      // For TOB, we don't necessarily track individual orders if they are aggregated,
      // but the original logic clears the book and adds this price.
    } else {
      return; // UndertPrice TOB means clear only
    }
  }

  // Find or create price level
  PriceLevel* level = find_price_level(levels, msg.price, is_bid);
  
  // Allocate node from pool
  int32_t node_idx = order_pool_.allocate();
  OrderNode& node = order_pool_[node_idx];
  node.msg = msg;
  node.next_idx = -1;
  node.prev_idx = level->tail_idx;

  // Update intrusive list at this price level
  if (level->head_idx == -1) {
    level->head_idx = node_idx;
  } else {
    order_pool_[level->tail_idx].next_idx = node_idx;
  }
  level->tail_idx = node_idx;
  level->total_qty += msg.size;

  // Update ID lookup map
  id_map_.insert(msg.order_id, node_idx);
}

void OptimizedDataBentoLOB::cancel_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1) return; // Order not found (might have been already filled/cancelled)

  OrderNode& node = order_pool_[node_idx];
  auto& levels = get_side(node.msg.side);
  bool is_bid = (node.msg.side == databento::Side::Bid);
  
  PriceLevel* level = find_price_level(levels, node.msg.price, is_bid);
  if (!level) return; // Should not happen if LOB is consistent

  // Adjust quantities
  uint32_t cancel_qty = std::min(node.msg.size, msg.size);
  node.msg.size -= cancel_qty;
  level->total_qty -= cancel_qty;

  // Fully remove if size is zero
  if (node.msg.size == 0) {
    // UNLINK from intrusive list
    if (node.prev_idx != -1) {
      order_pool_[node.prev_idx].next_idx = node.next_idx;
    } else {
      level->head_idx = node.next_idx;
    }

    if (node.next_idx != -1) {
      order_pool_[node.next_idx].prev_idx = node.prev_idx;
    } else {
      level->tail_idx = node.prev_idx;
    }

    // Clean up empty price level (Tombstone logic)
    if (level->head_idx == -1) {
      // If it is a tombstone at the very edge (Top of Book), we pop it.
      // This keeps Top-Of-Book pops O(1) and clears trailing tombstones.
      // Otherwise, it sits safely in the middle without triggering O(N) shifts.
      while (!levels.empty() && levels.back().total_qty == 0) {
        levels.pop_back();
      }
    }

    // Return node to pool and clear map
    id_map_.erase(msg.order_id);
    order_pool_.deallocate(node_idx);
  }
}

void OptimizedDataBentoLOB::modify_order(const databento::MboMsg &msg) {
  int32_t node_idx = id_map_.find(msg.order_id);
  if (node_idx == -1) {
    add_order(msg);
    return;
  }

  OrderNode& node = order_pool_[node_idx];
  
  // Complexity: If price changes, it's a Cancel + Add (HFT Standard)
  if (node.msg.price != msg.price) {
    databento::MboMsg cancel_msg = node.msg;
    cancel_msg.action = databento::Action::Cancel;
    cancel_order(cancel_msg);
    add_order(msg);
  } else {
    // Same price, different size
    auto& levels = get_side(node.msg.side);
    bool is_bid = (node.msg.side == databento::Side::Bid);
    PriceLevel* level = find_price_level(levels, node.msg.price, is_bid);
    
    if (msg.size > node.msg.size) {
      // In many HFT systems, increasing size loses priority (FIFO)
      // For this implementation, we follow the original logic 
      // which might re-link or just update.
      level->total_qty -= node.msg.size;
      level->total_qty += msg.size;
      node.msg.size = msg.size;
    } else {
      level->total_qty -= (node.msg.size - msg.size);
      node.msg.size = msg.size;
    }
  }
}

void OptimizedDataBentoLOB::clear_book() {
  bids_.clear();
  asks_.clear();
  id_map_.clear();
  // Note: ObjectPool doesn't necessarily need to be cleared as dealloc handles it,
  // but for a full system reset, we'd recreate it.
}

std::vector<OptimizedDataBentoLOB::PriceLevel>& OptimizedDataBentoLOB::get_side(databento::Side side) {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

const std::vector<OptimizedDataBentoLOB::PriceLevel>& OptimizedDataBentoLOB::get_side(databento::Side side) const {
  return (side == databento::Side::Bid) ? bids_ : asks_;
}

OptimizedDataBentoLOB::PriceLevel* OptimizedDataBentoLOB::find_price_level(std::vector<PriceLevel>& side, int64_t price, bool is_bid) {
  auto it = std::lower_bound(side.begin(), side.end(), price, 
      [is_bid](const PriceLevel& pl, int64_t p) {
        return is_bid ? pl.price < p : pl.price > p;
      });

  if (it != side.end() && it->price == price) {
    return &(*it);
  } else {
    // NOT FOUND: Insert new price level at the correct sorted position
    return &(*side.insert(it, {price, 0, -1, -1}));
  }
}

} // namespace backtesting_engine::mbo
