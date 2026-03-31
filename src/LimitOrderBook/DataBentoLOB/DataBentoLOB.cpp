#include "DataBentoLOB.h"
#include "databento/enums.hpp"
#include "databento/record.hpp"
#include <algorithm>

namespace backtesting_engine::mbo {
void DataBentoLOB::update_book(const databento::MboMsg &msg) {
  if (msg.action == databento::Action::Add) {
    add_order(msg);
  } else if (msg.action == databento::Action::Modify) {
    modify_order(msg);
  } else if (msg.action == databento::Action::Cancel) {
    cancel_order(msg);
  } else if (msg.action == databento::Action::Clear) {
    clear_book();
  } else if (msg.action == databento::Action::Trade ||
             msg.action == databento::Action::Fill) {
    // do nothing
  } else {
    // throw std::runtime_error{std::string{"Unknown action"} +
    //                          databento::ToString(msg.action)};
    std::cout << "Unknown action: " << databento::ToString(msg.action) << "\n";
  }
}

std::pair<int64_t, int64_t> DataBentoLOB::get_bbo() const {
  return std::make_pair(
      bids_.empty() ? databento::kUndefPrice : bids_.rbegin()->first,
      asks_.empty() ? databento::kUndefPrice : asks_.begin()->first);
}

uint64_t DataBentoLOB::get_level_qty(databento::Side side,
                                     int64_t price) const {
  const auto &levels = get_side(side);
  auto it = levels.find(price);
  if (it != levels.end()) {
    return it->second.total_qty;
  }
  return 0; // Price level does not exist
}

uint64_t DataBentoLOB::get_total_volume() const {
  uint64_t total = 0;
  for (const auto &[price, level] : bids_) {
    total += level.total_qty;
  }
  for (const auto &[price, level] : asks_) {
    total += level.total_qty;
  }
  return total;
}

void DataBentoLOB::add_order(const databento::MboMsg &msg) {
  if (orders_.find(msg.order_id) != orders_.end()) {
    throw std::runtime_error{"Order already exists"};
  }
  PriceLevels &levels = get_side(msg.side);
  if (msg.flags.IsTob()) {
    levels.clear();
    // kUndefPrice indicates the side's book should be cleared
    // and doesn't represent an order that should be added
    if (msg.price != databento::kUndefPrice) {
      PriceLevel level = {msg.size, {msg}};
      levels.emplace(msg.price, level);
    }
  } else {
    orders_.emplace(msg.order_id, Order{msg.price, msg.side});
    levels[msg.price].messages.push_back(msg);
    levels[msg.price].total_qty += msg.size;
  }
}

void DataBentoLOB::modify_order(const databento::MboMsg &msg) {
  auto order_it = orders_.find(msg.order_id);
  if (order_it == orders_.end()) {
    add_order(msg);
    return;
  }

  PriceLevels &levels = get_side(order_it->second.side);
  auto &level = get_price_level(levels, order_it->second.price)->second;
  auto level_order_it = get_order_message(order_it->first, level);

  if (order_it->second.price != msg.price) {
    level.total_qty -= level_order_it->size;
    level.messages.erase(level_order_it);

    if (level.messages.empty()) {
      levels.erase(order_it->second.price);
    }
    order_it->second.price = msg.price;
    levels[msg.price].messages.push_back(msg);
    levels[msg.price].total_qty += msg.size;
  } else if (level_order_it->size < msg.size) {
    level.total_qty -= level_order_it->size;
    level.messages.erase(level_order_it);
    level.messages.push_back(msg);
    level.total_qty += msg.size;
  } else {
    level.total_qty -= (level_order_it->size - msg.size);
    level_order_it->size = msg.size;
  }
  return;
}
void DataBentoLOB::cancel_order(const databento::MboMsg &msg) {
  auto order_it = orders_.find(msg.order_id);
  if (order_it == orders_.end()) {
    throw std::runtime_error{"Order does not exist in orders_"};
  }

  PriceLevels &levels = get_side(order_it->second.side);
  auto &level = get_price_level(levels, order_it->second.price)->second;
  auto level_order_it = get_order_message(order_it->first, level);

  level_order_it->size -= msg.size;
  level.total_qty -= msg.size;
  if (level_order_it->size == 0) {
    const int64_t price = order_it->second.price;
    orders_.erase(order_it);
    level.messages.erase(level_order_it);
    if (level.messages.empty()) {
      levels.erase(price);
    }
  }
}
void DataBentoLOB::clear_book() {
  bids_.clear();
  asks_.clear();
  orders_.clear();
}

DataBentoLOB::PriceLevels &DataBentoLOB::get_side(databento::Side side) {
  if (side == databento::Side::Bid) {
    return bids_;
  } else {
    return asks_;
  }
}

// const version of get_side
const DataBentoLOB::PriceLevels &
DataBentoLOB::get_side(databento::Side side) const {
  if (side == databento::Side::Bid) {
    return bids_;
  } else {
    return asks_;
  }
}

DataBentoLOB::PriceLevels::iterator
DataBentoLOB::get_price_level(PriceLevels &price_levels, int64_t price) {
  auto level_it = price_levels.find(price);
  if (level_it == price_levels.end()) {
    throw std::runtime_error{"Requested level with price: " +
                             std::to_string(price) + " does not exist"};
  }
  return level_it;
}

DataBentoLOB::Order &DataBentoLOB::get_order(uint64_t order_id) {
  auto order_it = orders_.find(order_id);
  if (order_it == orders_.end()) {
    throw std::runtime_error{"Order does not exist in orders_"};
  }
  return order_it->second;
}

std::vector<databento::MboMsg>::iterator
DataBentoLOB::get_order_message(uint64_t order_id, PriceLevel &level) {
  auto level_order_it = std::find_if(
      level.messages.begin(), level.messages.end(),
      [order_id](databento::MboMsg msg) { return msg.order_id == order_id; });
  if (level_order_it == level.messages.end()) {
    throw std::runtime_error{
        "Order does not exist in level"}; // Or handle appropriately
  }
  return level_order_it;
}
} // namespace backtesting_engine::mbo
