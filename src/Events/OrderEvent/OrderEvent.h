#pragma once

#include "Events/Event.h"
#include <chrono>
#include <optional>
#include <string>


namespace backtesting_engine {
enum class OrderDirection { BUY, SELL };
inline std::ostream &operator<<(std::ostream &os, OrderDirection od) {
  switch (od) {
  case OrderDirection::BUY:
    return os << "BUY";
  case OrderDirection::SELL:
    return os << "SELL";
  default:
    return os << "UNKNOWN";
  }
}
enum class OrderType { MARKET, LIMIT };
inline std::ostream &operator<<(std::ostream &os, OrderType ot) {
  switch (ot) {
  case OrderType::MARKET:
    return os << "MARKET";
  case OrderType::LIMIT:
    return os << "LIMIT";
  default:
    return os << "UNKNOWN";
  }
}
class OrderEvent : public Event {
public:
  OrderEvent(std::string ticker,
             std::chrono::system_clock::time_point timestamp, int quantity,
             OrderDirection direction, OrderType order_type = OrderType::MARKET,
             std::optional<double> limit_price = std::nullopt)
      : ticker_(std::move(ticker)), timestamp_(timestamp), quantity_(quantity),
        direction_(direction), order_type_(order_type),
        limit_price_(limit_price) {
    if (order_type == OrderType::LIMIT && !limit_price.has_value()) {
      throw std::invalid_argument("Limit order must have a limit price");
    } else if (order_type == OrderType::MARKET && limit_price.has_value()) {
      throw std::invalid_argument("Market order cannot have a limit price");
    }
  }

  EventType get_type() const override { return EventType::Order; }

  const OrderType order_type_;
  const std::string ticker_;
  const std::chrono::system_clock::time_point timestamp_;
  const int quantity_;
  const OrderDirection direction_;
  const std::optional<double> limit_price_;
};
} // namespace backtesting_engine
