#pragma once
#include <cstdint>
#include <databento/enums.hpp>
#include <queue>
#include <variant>
#include <vector>

namespace backtesting_engine {
// Enum for all event subtypes. This will be used for easy and readible checking
// of event types
enum class EventType { Market, Signal, Order, Fill };

// Base class for all event sub types. Sub types should have a corresponding
// enum EventType
class Event {
public:
  virtual ~Event() = default;

  // Pure virtual function to get the event type.
  // MUST BE IMPLEMENTED BY SUBCLASSES
  virtual EventType get_type() const = 0;
};

// MBO event definitions below
//
//
//
struct CreateOrderEvent {
  uint64_t order_id;
  int64_t price;
  uint64_t qty;
  databento::Side side;
};

struct AckOrderEvent {
  uint64_t order_id;
};

// NOTE: databento cancel events act as fill events. This represents the actual
// cancellation of an order
struct CancelOrderEvent {
  uint64_t order_id;
};

struct FillOrderEvent {
  uint64_t order_id;
  uint64_t filled_qty;
  // int64_t price;
};

using EventPayload = std::variant<CreateOrderEvent, AckOrderEvent,
                                  CancelOrderEvent, FillOrderEvent>;

struct EventV2 {
  uint64_t timestamp_ns; // The exact time this event occurs
  EventPayload payload;  // The actual event data
};

// Comparator that makes the priority queue act as a Min-Heap (earliest time
// first)
struct EventCompare {
  bool operator()(const EventV2 &a, const EventV2 &b) const {
    // Return true if 'a' should be ordered AFTER 'b'
    return a.timestamp_ns > b.timestamp_ns;
  }
};
// Define the discrete event queue
using EventQueue =
    std::priority_queue<EventV2, std::vector<EventV2>, EventCompare>;
} // namespace backtesting_engine
