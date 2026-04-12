#pragma once
#include "Common/Containers/FlatHashMap.h"
#include "Common/Containers/ObjectPool.h"
#include "Events/Mbo/MboEvent.h"
#include "LimitOrderBook/LimitOrderBookConcept.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "Performance/SimulationProfiler.h"
#include "Performance/TSC_Clock.h"
#include <databento/record.hpp>
#include <stdexcept>
#include <string>

namespace backtesting_engine::mbo {

template <LimitOrderBookConcept LOB> class VirtualExchange {
public:
  VirtualExchange(NetworkSimulator &simulator, LOB &lob,
                  performance::SimulationProfiler &sim_profiler)
      : simulator_(simulator), lob_(lob), sim_profiler_(sim_profiler) {}

  void on_fill(const databento::MboMsg &msg);
  void on_update(const EventV2 &event);

private:
  struct alignas(64) VirtualOrder {
    uint64_t order_id;
    int64_t price;
    uint64_t qty_ahead;
    uint64_t remaining_qty;
    uint64_t original_qty;
    int32_t prev_order_idx = -1;
    int32_t next_order_idx = -1;
    databento::Side side;
  };

  struct VirtualPriceLevel {
    int64_t price;
    databento::Side side;
    int32_t prev_level_idx = -1;
    int32_t next_level_idx = -1;
    int32_t head_order_idx = -1;
    int32_t tail_order_idx = -1;
    uint64_t total_qty_ahead = 0;
  };

  void fill_orders_at_price_level(int32_t level_idx, uint64_t &fill_qty_left,
                                  uint64_t timestamp_ns,
                                  int64_t actual_fill_price);

  void erase_level(int32_t level_idx);

  NetworkSimulator &simulator_;
  LOB &lob_;
  performance::SimulationProfiler &sim_profiler_;

  common::FlatHashMap<uint64_t, int32_t, 8192>
      order_idx_map_; // order_id to order_idx in order_pool_
  common::FlatHashMap<int64_t, int32_t, 2048>
      buy_level_idx_map_; // price to level_idx in level_pool_
  common::FlatHashMap<int64_t, int32_t, 2048>
      sell_level_idx_map_; // price to level_idx in level_pool_

  common::ObjectPool<VirtualOrder, 8192> order_pool_;
  common::ObjectPool<VirtualPriceLevel, 2048> level_pool_;

  int32_t best_bid_level_idx_ = -1;
  int32_t best_ask_level_idx_ = -1;

  void on_create_order(uint64_t timestamp_ns, const CreateOrderEvent &event);
  void on_cancel_order(uint64_t timestamp_ns, const CancelOrderEvent &event);
};

// --- Implementation ---

namespace detail {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace detail

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_update(const EventV2 &event) {
  std::visit(detail::overloaded{[&](const CreateOrderEvent &e) {
                                  on_create_order(event.timestamp_ns, e);
                                },
                                [&](const CancelOrderEvent &e) {
                                  on_cancel_order(event.timestamp_ns, e);
                                },
                                [](const auto &) {
                                  throw std::runtime_error{
                                      "Unsupported event type!"};
                                }},
             event.payload);
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_create_order(uint64_t timestamp_ns,
                                           const CreateOrderEvent &event) {
  uint64_t start_tsc = performance::get_tsc();

  if (order_idx_map_.find(event.order_id) != -1) {
    throw std::runtime_error{"Order already exists"};
  }

  uint64_t qty_ahead = lob_.get_level_qty(event.side, event.price);

  auto &level_map = (event.side == databento::Side::Bid) ? buy_level_idx_map_
                                                         : sell_level_idx_map_;
  int32_t level_idx = level_map.find(event.price);

  if (level_idx != -1) {
    qty_ahead -= level_pool_[level_idx].total_qty_ahead;
  } else {
    // Create new level
    level_idx = level_pool_.allocate();
    auto &level = level_pool_[level_idx];
    level.price = event.price;
    level.side = event.side;
    level.prev_level_idx = -1;
    level.next_level_idx = -1;
    level.head_order_idx = -1;
    level.tail_order_idx = -1;
    level.total_qty_ahead = 0;

    level_map.insert(event.price, level_idx);

    // Insert into sorted list
    int32_t *best_level_idx_ptr = (event.side == databento::Side::Bid)
                                      ? &best_bid_level_idx_
                                      : &best_ask_level_idx_;

    if (*best_level_idx_ptr == -1) {
      *best_level_idx_ptr = level_idx;
    } else {
      int32_t curr_idx = *best_level_idx_ptr;
      int32_t prev_idx = -1;

      // --- ARCHITECTURE DECISION: O(N) Insertion vs O(1) Arrays ---
      // We explicitly choose a linear O(N) insertion here to maintain an
      // intrusive doubly-linked list. While a Direct-Mapped Array +
      // Hierarchical Bitmask would yield O(1) insertions, it would force
      // on_fill() to unnecessarily scan across potentially massive empty price
      // gaps. Since this is the *Virtual* Exchange, we assume the user's
      // strategy maintains a small number of active price levels (e.g., N < 50)
      // tightly around the BBO. At this scale, the O(N) insertion adds only a
      // few nanoseconds, while delivering O(1) jump-to-next-active-level
      // performance during latency-critical market data sweeps.
      //
      // Bids are sorted descending (highest price first)
      // Asks are sorted ascending (lowest price first)
      while (curr_idx != -1) {
        bool should_insert = false;
        if (event.side == databento::Side::Bid) {
          should_insert = event.price > level_pool_[curr_idx].price;
        } else {
          should_insert = event.price < level_pool_[curr_idx].price;
        }

        if (should_insert) {
          break;
        }
        prev_idx = curr_idx;
        curr_idx = level_pool_[curr_idx].next_level_idx;
      }

      // Insert between prev_idx and curr_idx
      level.next_level_idx = curr_idx;
      level.prev_level_idx = prev_idx;

      if (curr_idx != -1) {
        level_pool_[curr_idx].prev_level_idx = level_idx;
      }

      if (prev_idx != -1) {
        level_pool_[prev_idx].next_level_idx = level_idx;
      } else {
        *best_level_idx_ptr = level_idx; // New best price
      }
    }
  }

  // Create Order
  int32_t order_idx = order_pool_.allocate();
  auto &order = order_pool_[order_idx];
  order.order_id = event.order_id;
  order.side = event.side;
  order.price = event.price;
  order.qty_ahead = qty_ahead;
  order.remaining_qty = event.qty;
  order.original_qty = event.qty;
  order.prev_order_idx = -1;
  order.next_order_idx = -1;

  order_idx_map_.insert(event.order_id, order_idx);

  // Link Order to Level
  auto &level = level_pool_[level_idx];
  level.total_qty_ahead += qty_ahead;

  if (level.tail_order_idx == -1) {
    level.head_order_idx = order_idx;
    level.tail_order_idx = order_idx;
  } else {
    order.prev_order_idx = level.tail_order_idx;
    order_pool_[level.tail_order_idx].next_order_idx = order_idx;
    level.tail_order_idx = order_idx;
  }

  simulator_.send_inbound_event(
      {timestamp_ns, AckCreateOrderEvent{event.order_id}});

  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_ADD,
                               end_tsc - start_tsc);
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_cancel_order(uint64_t timestamp_ns,
                                           const CancelOrderEvent &event) {
  uint64_t start_tsc = performance::get_tsc();

  int32_t order_idx = order_idx_map_.find(event.order_id);
  if (order_idx == -1) {
    simulator_.send_inbound_event(
        {timestamp_ns, AckCancelOrderEvent{event.order_id}});

    uint64_t end_tsc = performance::get_tsc();
    sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_CANCEL,
                                 end_tsc - start_tsc);
    return;
  }

  auto &order = order_pool_[order_idx];
  auto &level_map = (order.side == databento::Side::Bid) ? buy_level_idx_map_
                                                         : sell_level_idx_map_;
  int32_t level_idx = level_map.find(order.price);
  auto &level = level_pool_[level_idx];

  if (order.next_order_idx != -1) {
    order_pool_[order.next_order_idx].qty_ahead += order.qty_ahead;
  } else {
    level.total_qty_ahead -= order.qty_ahead;
  }

  // Remove order from level's doubly linked list
  if (order.prev_order_idx != -1) {
    order_pool_[order.prev_order_idx].next_order_idx = order.next_order_idx;
  } else {
    level.head_order_idx = order.next_order_idx;
  }

  if (order.next_order_idx != -1) {
    order_pool_[order.next_order_idx].prev_order_idx = order.prev_order_idx;
  } else {
    level.tail_order_idx = order.prev_order_idx;
  }

  uint64_t ev_ord_id = event.order_id;
  order_idx_map_.erase(ev_ord_id);
  order_pool_.deallocate(order_idx);

  if (level.head_order_idx == -1) {
    erase_level(level_idx);
  }

  simulator_.send_inbound_event({timestamp_ns, AckCancelOrderEvent{ev_ord_id}});

  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_CANCEL,
                               end_tsc - start_tsc);
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::erase_level(int32_t level_idx) {
  auto &level = level_pool_[level_idx];
  auto &level_map = (level.side == databento::Side::Bid) ? buy_level_idx_map_
                                                         : sell_level_idx_map_;

  if (level.prev_level_idx != -1) {
    level_pool_[level.prev_level_idx].next_level_idx = level.next_level_idx;
  } else {
    if (level.side == databento::Side::Bid) {
      best_bid_level_idx_ = level.next_level_idx;
    } else {
      best_ask_level_idx_ = level.next_level_idx;
    }
  }

  if (level.next_level_idx != -1) {
    level_pool_[level.next_level_idx].prev_level_idx = level.prev_level_idx;
  }

  level_map.erase(level.price);
  level_pool_.deallocate(level_idx);
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_fill(const databento::MboMsg &msg) {
  uint64_t start_tsc = performance::get_tsc();
  uint64_t fill_qty_left = msg.size;
  uint64_t timestamp_ns = msg.ts_recv.time_since_epoch().count();

  if (msg.side == databento::Side::Bid) {
    int32_t curr_level_idx = best_bid_level_idx_;
    // Bids are sorted descending. So we iterate while level_price >= msg.price
    while (curr_level_idx != -1 &&
           level_pool_[curr_level_idx].price >= msg.price && fill_qty_left) {
      int32_t next_level_idx = level_pool_[curr_level_idx].next_level_idx;
      fill_orders_at_price_level(curr_level_idx, fill_qty_left, timestamp_ns,
                                 msg.price);
      curr_level_idx = next_level_idx;
    }
  } else {
    int32_t curr_level_idx = best_ask_level_idx_;
    // Asks are sorted ascending. So we iterate while level_price <= msg.price
    while (curr_level_idx != -1 &&
           level_pool_[curr_level_idx].price <= msg.price && fill_qty_left) {
      int32_t next_level_idx = level_pool_[curr_level_idx].next_level_idx;
      fill_orders_at_price_level(curr_level_idx, fill_qty_left, timestamp_ns,
                                 msg.price);
      curr_level_idx = next_level_idx;
    }
  }

  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_MATCHING,
                               end_tsc - start_tsc);
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::fill_orders_at_price_level(
    int32_t level_idx, uint64_t &fill_qty_left, uint64_t timestamp_ns,
    int64_t actual_fill_price) {
  auto &level = level_pool_[level_idx];
  int32_t curr_order_idx = level.head_order_idx;

  while (curr_order_idx != -1 && fill_qty_left) {
    auto &order = order_pool_[curr_order_idx];
    int32_t next_order_idx = order.next_order_idx;

    if (order.qty_ahead < fill_qty_left) {
      fill_qty_left -= order.qty_ahead;
      level.total_qty_ahead -= order.qty_ahead;
      order.qty_ahead = 0;
    } else {
      order.qty_ahead -= fill_qty_left;
      level.total_qty_ahead -= fill_qty_left;
      fill_qty_left = 0;
    }

    uint64_t filled_qty = 0;
    if (order.qty_ahead == 0 && fill_qty_left) {
      if (order.remaining_qty >= fill_qty_left) {
        filled_qty = fill_qty_left;
        order.remaining_qty -= fill_qty_left;
        fill_qty_left = 0;
      } else {
        filled_qty = order.remaining_qty;
        fill_qty_left -= order.remaining_qty;
        order.remaining_qty = 0;
      }
    }

    if (filled_qty) {
      simulator_.send_inbound_event(
          {timestamp_ns,
           AckFillOrderEvent{order.order_id, filled_qty, actual_fill_price}});
    }

    if (order.remaining_qty == 0) {
      uint64_t ord_id = order.order_id;
      // Unlink order
      if (order.prev_order_idx != -1) {
        order_pool_[order.prev_order_idx].next_order_idx = order.next_order_idx;
      } else {
        level.head_order_idx = order.next_order_idx;
      }

      if (order.next_order_idx != -1) {
        order_pool_[order.next_order_idx].prev_order_idx = order.prev_order_idx;
      } else {
        level.tail_order_idx = order.prev_order_idx;
      }

      order_idx_map_.erase(ord_id);
      order_pool_.deallocate(curr_order_idx);
    }

    curr_order_idx = next_order_idx;
  }

  if (level.head_order_idx == -1) {
    erase_level(level_idx);
  }
}

} // namespace backtesting_engine::mbo