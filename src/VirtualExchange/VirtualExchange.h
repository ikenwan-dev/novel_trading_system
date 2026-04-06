#pragma once
#include "Events/Mbo/MboEvent.h"
#include "LimitOrderBook/LimitOrderBookConcept.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "Performance/SimulationProfiler.h"
#include "Performance/TSC_Clock.h"
#include <databento/record.hpp>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace backtesting_engine::mbo {

/**
 * @brief Matches user orders against incoming market data (MBO).
 * 
 * Generic over the LimitOrderBook implementation to allow for static polymorphism
 * and aggressive compiler inlining.
 */
template <LimitOrderBookConcept LOB>
class VirtualExchange {
public:
  VirtualExchange(NetworkSimulator &simulator, LOB &lob,
                  performance::SimulationProfiler &sim_profiler)
      : simulator_(simulator), lob_(lob), sim_profiler_(sim_profiler) {}

  void on_fill(const databento::MboMsg &msg);
  void on_update(const EventV2 &event);

private:
  template <typename LevelIterator>
  void fill_orders_at_price_level(LevelIterator &levels_it,
                                  uint64_t &fill_qty_left,
                                  uint64_t timestamp_ns,
                                  int64_t actual_fill_price);

  struct VirtualOrderMetaData {
    databento::Side side;
    int64_t price;
  };
  
  struct VirtualOrder {
    uint64_t order_id;
    uint64_t qty_ahead;
    uint64_t remaining_qty;
    uint64_t original_qty;
  };

  std::unordered_map<uint64_t, VirtualOrderMetaData> order_metadata_;
  NetworkSimulator &simulator_;
  LOB &lob_;
  performance::SimulationProfiler &sim_profiler_;

  struct VirtualPriceLevel {
    std::vector<VirtualOrder> orders;
    uint64_t total_qty_ahead = 0;
  };

  using VirtualPriceLevels = std::map<int64_t, VirtualPriceLevel>;
  VirtualPriceLevels open_buy_orders_;  // key is price
  VirtualPriceLevels open_sell_orders_; // key is price

  VirtualPriceLevels &get_side(databento::Side side);
  
  typename VirtualPriceLevels::iterator get_price_level(VirtualPriceLevels &levels,
                                                        int64_t price);

  typename std::vector<VirtualOrder>::iterator get_order(VirtualPriceLevel &level,
                                                         uint64_t order_id);

  void on_create_order(uint64_t timestamp_ns, const CreateOrderEvent &event);
  void on_cancel_order(uint64_t timestamp_ns, const CancelOrderEvent &event);
};

// --- Implementation ---

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_update(const EventV2 &event) {
  if (std::holds_alternative<CreateOrderEvent>(event.payload)) {
    on_create_order(event.timestamp_ns,
                    std::get<CreateOrderEvent>(event.payload));
  } else if (std::holds_alternative<CancelOrderEvent>(event.payload)) {
    on_cancel_order(event.timestamp_ns,
                    std::get<CancelOrderEvent>(event.payload));
  } else {
    throw std::runtime_error{"Unsupported event type index: " +
                             std::to_string(event.payload.index())};
  }
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_create_order(uint64_t timestamp_ns,
                                            const CreateOrderEvent &event) {
  uint64_t start_tsc = performance::get_tsc();
  if (order_metadata_.find(event.order_id) != order_metadata_.end()) {
    throw std::runtime_error{"Order already exists"};
  }
  
  VirtualPriceLevels &levels = get_side(event.side);
  uint64_t qty_ahead = lob_.get_level_qty(event.side, event.price);

  auto it = levels.find(event.price);
  if (it != levels.end()) {
    qty_ahead -= it->second.total_qty_ahead;
  }

  levels[event.price].orders.emplace_back(event.order_id, qty_ahead, event.qty,
                                          event.qty);
  levels[event.price].total_qty_ahead += qty_ahead;
  order_metadata_.emplace(event.order_id,
                          VirtualOrderMetaData{event.side, event.price});
  
  simulator_.send_inbound_event(
      {timestamp_ns, AckCreateOrderEvent{event.order_id}});
  
  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_ADD,
                               performance::TSC_Clock::tsc_to_nanoseconds(end_tsc - start_tsc));
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_cancel_order(uint64_t timestamp_ns,
                                            const CancelOrderEvent &event) {
  uint64_t start_tsc = performance::get_tsc();
  auto order_metadata_it = order_metadata_.find(event.order_id);
  if (order_metadata_it == order_metadata_.end()) {
      simulator_.send_inbound_event({timestamp_ns, AckCancelOrderEvent{event.order_id}});
      
      uint64_t end_tsc = performance::get_tsc();
      sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_CANCEL,
                                   performance::TSC_Clock::tsc_to_nanoseconds(end_tsc - start_tsc));
      return;
  }
  
  int64_t price = order_metadata_it->second.price;
  VirtualPriceLevels &levels = get_side(order_metadata_it->second.side);
  auto level_it = get_price_level(levels, price);
  VirtualPriceLevel &level = level_it->second;

  auto order_it = get_order(level, event.order_id);
  auto next_order_it = std::next(order_it);
  if (next_order_it != level.orders.end()) {
    next_order_it->qty_ahead += order_it->qty_ahead;
  } else {
    level.total_qty_ahead -= order_it->qty_ahead;
  }
  
  level.orders.erase(order_it);
  if (level.orders.empty()) {
    levels.erase(level_it);
  }
  
  order_metadata_.erase(order_metadata_it);
  simulator_.send_inbound_event({timestamp_ns, AckCancelOrderEvent{event.order_id}});
      
  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_ORDER_CANCEL,
                               performance::TSC_Clock::tsc_to_nanoseconds(end_tsc - start_tsc));
}

template <LimitOrderBookConcept LOB>
void VirtualExchange<LOB>::on_fill(const databento::MboMsg &msg) {
  uint64_t start_tsc = performance::get_tsc();
  VirtualPriceLevels &levels = get_side(msg.side);
  uint64_t fill_qty_left = msg.size;
  uint64_t timestamp_ns = msg.ts_recv.time_since_epoch().count();

  if (msg.side == databento::Side::Bid) {
    auto levels_it = levels.rbegin();
    while (levels_it != levels.rend() && levels_it->first >= msg.price &&
           fill_qty_left) {
      fill_orders_at_price_level(levels_it, fill_qty_left, timestamp_ns,
                                 msg.price);
      levels_it++;
    }
  } else {
    auto levels_it = levels.begin();
    while (levels_it != levels.end() && levels_it->first <= msg.price &&
           fill_qty_left) {
      fill_orders_at_price_level(levels_it, fill_qty_left, timestamp_ns,
                                 msg.price);
      levels_it++;
    }
  }
  
  uint64_t end_tsc = performance::get_tsc();
  sim_profiler_.record_latency(performance::SimMetric::VEX_MATCHING,
                               performance::TSC_Clock::tsc_to_nanoseconds(end_tsc - start_tsc));
}

template <LimitOrderBookConcept LOB>
VirtualExchange<LOB>::VirtualPriceLevels &
VirtualExchange<LOB>::get_side(databento::Side side) {
  return (side == databento::Side::Bid) ? open_buy_orders_ : open_sell_orders_;
}

template <LimitOrderBookConcept LOB>
typename VirtualExchange<LOB>::VirtualPriceLevels::iterator
VirtualExchange<LOB>::get_price_level(VirtualPriceLevels &levels, int64_t price) {
  auto level_it = levels.find(price);
  if (level_it == levels.end()) {
    throw std::runtime_error{"Level with price: " + std::to_string(price) +
                             " does not exist"};
  }
  return level_it;
}

template <LimitOrderBookConcept LOB>
typename std::vector<typename VirtualExchange<LOB>::VirtualOrder>::iterator
VirtualExchange<LOB>::get_order(VirtualPriceLevel &level, uint64_t order_id) {
  auto order_it = std::find_if(level.orders.begin(), level.orders.end(),
                                [order_id](const VirtualOrder &order) {
                                  return order.order_id == order_id;
                                });
  if (order_it == level.orders.end()) {
    throw std::runtime_error{"Order with id " + std::to_string(order_id) +
                             " does not exist in level"};
  }
  return order_it;
}

template <LimitOrderBookConcept LOB>
template <typename LevelIterator>
void VirtualExchange<LOB>::fill_orders_at_price_level(LevelIterator &levels_it,
                                                       uint64_t &fill_qty_left,
                                                       uint64_t timestamp_ns,
                                                       int64_t actual_fill_price) {
  auto level_it = levels_it->second.orders.begin();
  while (level_it != levels_it->second.orders.end() && fill_qty_left) {
    if (level_it->qty_ahead < fill_qty_left) {
      fill_qty_left -= level_it->qty_ahead;
      levels_it->second.total_qty_ahead -= level_it->qty_ahead;
      level_it->qty_ahead = 0;
    } else {
      level_it->qty_ahead -= fill_qty_left;
      levels_it->second.total_qty_ahead -= fill_qty_left;
      fill_qty_left = 0;
    }

    uint64_t filled_qty = 0;
    if (level_it->qty_ahead == 0 && fill_qty_left) {
      if (level_it->remaining_qty >= fill_qty_left) {
        filled_qty = fill_qty_left;
        level_it->remaining_qty -= fill_qty_left;
        fill_qty_left = 0;
      } else {
        filled_qty = level_it->remaining_qty;
        fill_qty_left -= level_it->remaining_qty;
        level_it->remaining_qty = 0;
      }
    }
    
    if (filled_qty) {
      simulator_.send_inbound_event(
          {timestamp_ns, AckFillOrderEvent{level_it->order_id, filled_qty, actual_fill_price}});
    }

    if (level_it->remaining_qty == 0) {
      order_metadata_.erase(level_it->order_id);
      level_it = levels_it->second.orders.erase(level_it);
    } else {
      level_it++;
    }
  }
}

} // namespace backtesting_engine::mbo