#pragma once

namespace backtesting_engine::bar {
enum class EventType { Market, Signal, Order, Fill };

class Event {
public:
  virtual ~Event() = default;
  virtual EventType get_type() const = 0;
};
} // namespace backtesting_engine::bar::bar
