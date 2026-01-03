#include "Portfolio.h"
#include "Events/FillEvent/FillEvent.h"
#include "Events/MarketEvent/MarketEvent.h"
#include "Events/OrderEvent/OrderEvent.h"
#include <algorithm>
#include <iostream>

void Portfolio::on_fill(const FillEvent &event) {
  int signed_qty_change = (event.direction_ == OrderDirection::BUY)
                              ? event.quantity_
                              : -event.quantity_;
  cash_ -= signed_qty_change * event.fill_price_;
  cash_ -= event.commision_; // commision is always subtracted
  auto &pos = positions_[event.ticker_];

  int old_quantity = pos.quantity;
  int new_quantity = old_quantity + signed_qty_change;

  if (old_quantity == 0) {
    // opening position
    pos.cost_basis = event.fill_price_;
  } else if ((old_quantity > 0 && new_quantity > old_quantity) ||
             (old_quantity < 0 && new_quantity < old_quantity)) {
    // extending position, updates cost basis using weighted average
    double total_old_cost = pos.cost_basis * old_quantity;
    double fill_cost = signed_qty_change * event.fill_price_;
    pos.cost_basis = (total_old_cost + fill_cost) / new_quantity;
  } else if ((old_quantity > 0 && new_quantity < 0) ||
             (old_quantity < 0 && new_quantity > 0)) {
    // switching from short to long or vice versa
    pos.cost_basis = event.fill_price_;
  }
  // note if a position is reduced partially, the cost basis is not updated

  pos.quantity = new_quantity;
  std::cout << event.timestamp_ << ": " << event.direction_ << " "
            << event.ticker_ << " quantity : " << pos.quantity
            << " cost basis: " << pos.cost_basis
            << " market value: " << pos.market_value << std::endl;
}

void Portfolio::on_market_data(const MarketEvent &event) {
  auto it = positions_.find(event.ticker_);
  if (it != positions_.end()) {
    it->second.market_value = event.close_;
    peak_equity_ = std::max(peak_equity_, get_total_value());
  }
}

double Portfolio::get_total_value() const {
  double total_market_value = 0;
  for (const auto &[ticker, position] : positions_) {
    total_market_value +=
        position.quantity * (position.market_value - position.cost_basis);
  }
  return total_market_value + cash_;
}

double Portfolio::get_unrealized_pnl() const {
  double unrealized_pnl = 0;
  for (const auto &[ticker, position] : positions_) {
    unrealized_pnl +=
        position.quantity * (position.market_value - position.cost_basis);
  }
  return unrealized_pnl;
}