#include "SimulatedExecutionHandler.h"
#include "Events/FillEvent/FillEvent.h"
#include "Events/OrderEvent/OrderEvent.h"
#include <iostream>
#include <memory>

void SimulatedExecutionHandler::on_order(const OrderEvent &order_event) {
  auto price_info = data_handler_.get_latest_price_info(order_event.ticker_);
  bool filled = false;

  double simulated_fill_price;
  if (!price_info.has_value()) {
    std::cout << "WARNING: No price info for " << order_event.ticker_
              << ". Halting trade." << std::endl;
    return;
  }
  if (order_event.order_type_ == OrderType::MARKET) {
    simulated_fill_price = price_info.value().open;
    filled = true;
  } else {
    if (order_event.direction_ == OrderDirection::BUY) {
      if (order_event.limit_price_.value() >= price_info.value().low) {
        simulated_fill_price = order_event.limit_price_.value();
        filled = true;
      }

    } else {
      if (order_event.limit_price_.value() <= price_info.value().high) {
        simulated_fill_price = order_event.limit_price_.value();
        filled = true;
      }
    }
  }

  if (!filled) {
    std::cout << "WARNING: Order not filled. Limit price not met." << std::endl;
    return;
  }

  auto temp_fill_event = FillEvent(
      order_event.ticker_, order_event.timestamp_, order_event.direction_,
      order_event.quantity_, simulated_fill_price, 0.0);
  double commision = transaction_cost_model_->calculate_cost(temp_fill_event);

  auto fill_event = std::make_shared<FillEvent>(
      order_event.ticker_, order_event.timestamp_, order_event.direction_,
      order_event.quantity_, simulated_fill_price, commision);

  event_queue_.push(fill_event);
}