#include "Portfolio.h"
#include "Events/FillEvent/FillEvent.h"
#include "Events/MarketEvent/MarketEvent.h"
#include "Events/OrderEvent/OrderEvent.h"
#include <algorithm>
#include <iostream>

void Portfolio::on_fill(const FillEvent &event) {
  auto &pos = positions_[event.ticker_];
  auto &quantity = pos.quantity;
  int old_quantity = quantity;
  int trade_signed_qty = (event.direction_ == OrderDirection::BUY)
                             ? event.quantity_
                             : -event.quantity_;

  // Update cash
  if (event.direction_ == OrderDirection::BUY) {
    cash_ -= (event.quantity_ * event.fill_price_ + event.commision_);
  } else {
    cash_ += (event.quantity_ * event.fill_price_ - event.commision_);
  }

  // Update holdings
  quantity += trade_signed_qty;

  // Update cost basis
  // We are "opening" or "extending" if the trade is in the same direction as
  // the current position, or if we have no position.
  bool is_opening = (old_quantity == 0) ||
                    (old_quantity > 0 && trade_signed_qty > 0) ||
                    (old_quantity < 0 && trade_signed_qty < 0);

  if (is_opening) {
    double trade_cost = (double)trade_signed_qty * event.fill_price_;
    // Commissions always increase the cost basis (outlay for long, reduced
    // proceeds for short).
    trade_cost += event.commision_;
    pos.cost_basis += trade_cost;
  } else {
    // Reducing or flipping position
    if (std::abs(trade_signed_qty) <= std::abs(old_quantity)) {
      // Partial or full close: reduce cost basis proportionally
      double reduction_ratio =
          (double)std::abs(trade_signed_qty) / std::abs(old_quantity);
      pos.cost_basis -= pos.cost_basis * reduction_ratio;
    } else {
      // Flipping position (e.g., Long 10 -> Short 5)
      // 1. Close the current position (original basis becomes 0)
      // 2. Open new position with the remainder of the trade
      int remaining_qty = trade_signed_qty + old_quantity;
      double comm_frac =
          (double)std::abs(remaining_qty) / std::abs(trade_signed_qty);
      double trade_cost = (double)remaining_qty * event.fill_price_ +
                          event.commision_ * comm_frac;
      pos.cost_basis = trade_cost;
    }
  }

  // Update market value immediately to avoid stale values in print/log
  if (quantity == 0) {
    pos.market_value = 0.0;
    pos.cost_basis = 0.0; // Ensure it's exactly zero
  } else {
    pos.market_value = (double)quantity * event.fill_price_;
  }

  std::cout << event.timestamp_ << ": " << event.direction_ << " "
            << event.ticker_ << " quantity : " << quantity
            << " cost basis: " << pos.cost_basis
            << " market value: " << pos.market_value << std::endl;
}

void Portfolio::on_market_data(const MarketEvent &event) {
  auto it = positions_.find(event.ticker_);
  if (it != positions_.end()) {
    it->second.market_value = event.close_ * it->second.quantity;
    peak_equity_ = std::max(peak_equity_, get_total_value());
  }
}

double Portfolio::get_total_value() const {
  double total_market_value = 0;
  for (const auto &[ticker, position] : positions_) {
    total_market_value += position.market_value;
  }
  return total_market_value + cash_;
}

double Portfolio::get_unrealized_pnl() const {
  double unrealized_pnl = 0;
  for (const auto &[ticker, position] : positions_) {
    unrealized_pnl += (position.market_value - position.cost_basis);
  }
  return unrealized_pnl;
}