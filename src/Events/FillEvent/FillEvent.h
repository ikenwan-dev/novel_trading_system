#pragma once

#include "Events/Event.h"
#include "Events/OrderEvent/OrderEvent.h"
#include <chrono>
#include <string>

class FillEvent : public Event {
public:
  FillEvent(std::string ticker, std::chrono::system_clock::time_point timestamp,
            OrderDirection direction, int quantity, double fill_price,
            double commision)
      : ticker_(std::move(ticker)), timestamp_(timestamp),
        direction_(direction), quantity_(quantity), fill_price_(fill_price),
        commision_(commision) {}

  EventType get_type() const override { return EventType::Fill; }

  const std::string ticker_;
  const std::chrono::system_clock::time_point timestamp_;
  const OrderDirection direction_;
  const int quantity_;
  const double fill_price_;
  const double commision_;
};
