#pragma once

#include "Events/Event.h"
#include <chrono>
#include <string>

enum class OrderDirection { BUY, SELL };

class OrderEvent : public Event {
public:
  OrderEvent(std::string ticker,
             std::chrono::system_clock::time_point timestamp, int quantity,
             OrderDirection direction)
      : ticker_(std::move(ticker)), timestamp_(timestamp), quantity_(quantity),
        direction_(direction) {}

  EventType get_type() const override { return EventType::Order; }

  const std::string ticker_;
  const std::chrono::system_clock::time_point timestamp_;
  const int quantity_;
  const OrderDirection direction_;
};