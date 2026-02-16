#include "DataBentoLOB.h"
#include "databento/enums.hpp"
#include "databento/record.hpp"
#include <algorithm>

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
    throw std::runtime_error{std::string{"Unknown action"} +
                             databento::ToString(msg.action)};
  }
}

void DataBentoLOB::add_order(const databento::MboMsg &msg) {
  if (orders_.find(msg.order_id) != orders_.end()) {
    throw std::runtime_error{"Order already exists"};
  }
  if (msg.flags.IsTob()) {
    PriceLevels &levels = get_side(msg.side);
    levels.clear();
    // kUndefPrice indicates the side's book should be cleared
    // and doesn't represent an order that should be added
    if (msg.price != databento::kUndefPrice) {
      PriceLevel level = {msg};
      levels.emplace(msg.price, level);
    }
  } else {
    orders_.emplace(msg.order_id, Order{msg.price, msg.side});
    get_side(msg.side)[msg.price].push_back(msg);
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
    level.erase(level_order_it);
    if (level.empty()) {
      levels.erase(order_it->second.price);
    }
    order_it->second.price = msg.price;
    levels[msg.price].push_back(msg);
  } else if (level_order_it->size < msg.size) {
    level.erase(level_order_it);
    level.push_back(msg);
  } else {
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
  if (level_order_it->size == 0) {
    const int64_t price = order_it->second.price;
    orders_.erase(order_it);
    level.erase(level_order_it);
    if (level.empty()) {
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

DataBentoLOB::PriceLevel::iterator
DataBentoLOB::get_order_message(uint64_t order_id, PriceLevel &level) {
  auto level_order_it = std::find_if(
      level.begin(), level.end(),
      [order_id](databento::MboMsg msg) { return msg.order_id == order_id; });
  if (level_order_it == level.end()) {
    throw std::runtime_error{
        "Order does not exist in level"}; // Or handle appropriately
  }
  return level_order_it;
}