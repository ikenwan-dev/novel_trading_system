#pragma once
#include "Events/Event.h"
#include <cstdint>
namespace backtesting_engine {
class NetworkSimulator {
public:
  explicit NetworkSimulator(EventQueue &event_queue, uint64_t outbound_latency,
                            uint64_t inbound_latency)
      : event_queue(event_queue), outbound_latency(outbound_latency),
        inbound_latency(inbound_latency) {}

  // Rule of 5: Delete default copy/move semantics because this is a unique
  // engine component
  NetworkSimulator(const NetworkSimulator &) = delete;
  NetworkSimulator &operator=(const NetworkSimulator &) = delete;
  NetworkSimulator(NetworkSimulator &&) = delete;
  NetworkSimulator &operator=(NetworkSimulator &&) = delete;
  ~NetworkSimulator() = default;

  void send_outbound_event(EventV2 event) {
    event.timestamp_ns += outbound_latency;
    event_queue.push(std::move(event));
  }

  void send_inbound_event(EventV2 event) {
    event.timestamp_ns += inbound_latency;
    event_queue.push(std::move(event));
  }

private:
  EventQueue &event_queue;
  uint64_t outbound_latency;
  uint64_t inbound_latency;
};
} // namespace backtesting_engine