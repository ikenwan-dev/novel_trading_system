#include "VirtualExchange.h"
#include "Events/Mbo/MboEvent.h"
#include <algorithm>

namespace backtesting_engine::mbo {
void VirtualExchange::on_update(const EventV2 &event) {
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

void VirtualExchange::on_create_order(uint64_t timestamp_ns,
                                      const CreateOrderEvent &event) {
  if (order_metadata_.find(event.order_id) != order_metadata_.end()) {
    throw std::runtime_error{"Order already exists"};
    // Could also potentially reject order
  }
  VirtualPriceLevels &levels = get_side(event.side);
  uint64_t qty_ahead = lob_.get_level_qty(event.side, event.price);

  // if there are orders at this price level, subtract the qty ahead of the
  // order that is ahead of this one
  auto it = levels.find(event.price);
  if (it != levels.end()) {
    // Subtract ALL relative qty_ahead values from prior orders to find the
    // remaining market orders
    qty_ahead -= it->second.total_qty_ahead;
  }

  levels[event.price].orders.emplace_back(event.order_id, qty_ahead, event.qty,
                                          event.qty);
  levels[event.price].total_qty_ahead += qty_ahead;
  order_metadata_.emplace(event.order_id,
                          VirtualOrderMetaData{event.side, event.price});
  simulator_.send_inbound_event(
      {timestamp_ns, AckCreateOrderEvent{event.order_id}});
}

void VirtualExchange::on_cancel_order(uint64_t timestamp_ns,
                                      const CancelOrderEvent &event) {
  auto order_metadata_it = order_metadata_.find(event.order_id);
  if (order_metadata_it == order_metadata_.end()) {
      // Race condition: Order was already completely filled in the matching engine, 
      // but the strategy hasn't received the AckFill due to latency, so it tried to cancel it.
      // We safely ignore this "too late to cancel" request.
      simulator_.send_inbound_event(
          {timestamp_ns, AckCancelOrderEvent{event.order_id}}); // Ideally a CancelReject, but we map to AckCancel to let OMS clean it up gracefully if it wants.
      return;
  }
  int64_t price = order_metadata_it->second.price;
  VirtualPriceLevels &levels = get_side(order_metadata_it->second.side);
  VirtualPriceLevel &level = get_price_level(levels, price)->second;

  auto order_it = get_order(level, event.order_id);
  auto next_order_it = std::next(order_it);
  if (next_order_it != level.orders.end()) {
    next_order_it->qty_ahead += order_it->qty_ahead;
  } else {
    level.total_qty_ahead -= order_it->qty_ahead;
  }
  level.orders.erase(order_it);
  if (level.orders.empty()) {
    levels.erase(price);
  }
  order_metadata_.erase(order_metadata_it);
  simulator_.send_inbound_event(
      {timestamp_ns, AckCancelOrderEvent{event.order_id}});
}
void VirtualExchange::on_fill(const databento::MboMsg &msg) {
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
}

VirtualExchange::VirtualPriceLevels &
VirtualExchange::get_side(databento::Side side) {
  if (side == databento::Side::Bid) {
    return open_buy_orders_;
  } else {
    return open_sell_orders_;
  }
}

VirtualExchange::VirtualPriceLevels::iterator
VirtualExchange::get_price_level(VirtualPriceLevels &levels, int64_t price) {
  auto level_it = levels.find(price);
  if (level_it == levels.end()) {
    throw std::runtime_error{"Level with price: " + std::to_string(price) +
                             " does not exist"};
  }
  return level_it;
}

std::vector<VirtualExchange::VirtualOrder>::iterator
VirtualExchange::get_order(VirtualPriceLevel &level, uint64_t order_id) {
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

std::unordered_map<uint64_t, VirtualExchange::VirtualOrderMetaData>::iterator
VirtualExchange::get_order_metadata(uint64_t order_id) {
  auto order_metadata_it = order_metadata_.find(order_id);
  if (order_metadata_it == order_metadata_.end()) {
    throw std::runtime_error{"Order does not exist"};
  }
  return order_metadata_it;
}
} // namespace backtesting_engine::mbo