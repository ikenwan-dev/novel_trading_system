#include "SimulatedExecutionHandler.h"
#include "Events/FillEvent/FillEvent.h"
#include <memory>

void SimulatedExecutionHandler::on_order(const OrderEvent &order_event) {
  double simulated_fill_price =
      data_handler_.get_latest_price_info(order_event.ticker_).open;

  auto fill_event = std::make_shared<FillEvent>(
      order_event.ticker_, order_event.timestamp_, order_event.direction_,
      order_event.quantity_, simulated_fill_price, 0.0);

  event_queue_.push(fill_event);
}