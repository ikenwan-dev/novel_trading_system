#pragma once
#include "Events/Bar/BarEvent.h"
#include <chrono>
#include <string>


namespace backtesting_engine::bar {
class MarketEvent : public Event {
public:
  MarketEvent(std::string ticker,
              std::chrono::system_clock::time_point timestamp, double open,
              double close, double high, double low, int volume)
      : ticker_(std::move(ticker)), timestamp_(timestamp), open_(open),
        close_(close), high_(high), low_(low), volume_(volume) {}

  EventType get_type() const override { return EventType::Market; }

  const std::string ticker_;
  const std::chrono::system_clock::time_point timestamp_;
  const double open_;
  const double close_;
  const double high_;
  const double low_;
  const int volume_;
};
} // namespace backtesting_engine::bar
