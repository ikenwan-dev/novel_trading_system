#include "Portfolio.h"
#include "Events/FillEvent/FillEvent.h"
#include "Events/MarketEvent/MarketEvent.h"
#include "Events/OrderEvent/OrderEvent.h"
#include <algorithm>

void Portfolio::on_fill(const FillEvent &event) {
  double fill_cost = event.quantity_ * event.fill_price_ + event.commision_;
  if (event.direction_ == OrderDirection::BUY) {
    cash_ -= fill_cost;
    holdings_[event.ticker_] += event.quantity_;
    positions_[event.ticker_].cost_basis += fill_cost;
  } else {
    cash_ += fill_cost;
    holdings_[event.ticker_] -= event.quantity_;
    positions_[event.ticker_].cost_basis -= fill_cost;
  }
}

void Portfolio::on_market_data(const MarketEvent &event) {
  auto it = positions_.find(event.ticker_);
  if (it != positions_.end()) {
    it->second.market_value = event.close_ * holdings_[event.ticker_];
  }
  peak_equity_ = std::max(peak_equity_, get_total_value());
}

double Portfolio::get_total_value() const {
  double total_market_value = 0;
  for (const auto &[ticker, position] : positions_) {
    total_market_value += position.market_value;
  }
  return total_market_value;
}

double Portfolio::get_unrealized_pnl() const {
  double unrealized_pnl = 0;

  for (const auto &[ticker, position] : positions_) {
    unrealized_pnl += position.market_value - position.cost_basis;
  }
  return unrealized_pnl;
}