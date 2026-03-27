#pragma once
#include <cstdint>
#include <databento/enums.hpp>
#include <queue>
#include <variant>
#include <vector>
#include "RiskManager/Mbo/RiskResult.h"

namespace backtesting_engine::mbo {
struct CreateOrderEvent {
  uint64_t order_id;
  int64_t price;
  uint64_t qty;
  databento::Side side; // todo: remove dependency on databento
};

struct AckCreateOrderEvent {
  uint64_t order_id;
};

// NOTE: databento cancel events act as fill events. This represents the actual
// cancellation of an order
struct CancelOrderEvent {
  uint64_t order_id;
};

struct AckCancelOrderEvent {
  uint64_t order_id;
};

struct AckFillOrderEvent {
  uint64_t order_id;
  uint64_t filled_qty;
  int64_t price;
};

struct RejectOrderEvent {
  uint64_t order_id;
  RiskResult reason;
};

using EventPayload =
    std::variant<CreateOrderEvent, AckCreateOrderEvent, CancelOrderEvent,
                 AckCancelOrderEvent, AckFillOrderEvent, RejectOrderEvent>;

struct EventV2 {
  uint64_t timestamp_ns; // The exact time this event occurs(nanoseconds since
                         // unix epoch)
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
static_assert(sizeof(EventV2) <= 64,
              "EventV2 exceeds 64-byte cache line size! This will cause "
              "performance degradation.");
} // namespace backtesting_engine::mbo
