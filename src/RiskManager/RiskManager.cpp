#include "RiskManager.h"
#include "Events/OrderEvent/OrderEvent.h"
#include "Events/SignalEvent/SignalEvent.h"
#include <iostream>
#include <memory>

void RiskManager::on_signal(const SignalEvent &signal_event) {
  // TODO: dont use fixed position sizing and instead do 1-2% based on account
  // value
  int position_size = 100;
  double latest_price =
      portfolio_.get_latest_closing_price(signal_event.ticker_);
  double position_value = position_size * latest_price;

  if (portfolio_.get_cash() < position_value) {
    std::cout << "Not enough cash to open position for ticker "
              << signal_event.ticker_ << " with value " << position_value
              << " and cash " << portfolio_.get_cash() << std::endl;
    return;
  }

  if (portfolio_.get_current_drawdown() > 0.2) {
    // TODO: this should be a configurable parameter
    // TODO: this should liquidate all positions
    std::cout << "Current drawdown is too high. Halting strategy." << std::endl;
    return;
  }

  OrderDirection order_direction =
      signal_event.direction_ == SignalDirection::LONG ? OrderDirection::BUY
                                                       : OrderDirection::SELL;

  queue_.push(std::make_shared<OrderEvent>(signal_event.ticker_,
                                           signal_event.timestamp_,
                                           position_size, order_direction));
}