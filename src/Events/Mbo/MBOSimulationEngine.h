#pragma once

#include "LimitOrderBook/LimitOrderBookConcept.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "Performance/LatencyProfiler.h"
#include "Performance/TSC_Clock.h"
#include "VirtualExchange/VirtualExchange.h"
#include <cstdint>
#include <databento/historical.hpp>
#include <variant>

#include "DataConsumers/MBODataConsumerConcept.h"
#include "Strategies/MBOStrategies/MBOStrategyConcept.h"

namespace backtesting_engine::mbo {

template <DataConsumerConcept Consumer, LimitOrderBookConcept LOB,
          StrategyConcept<LOB> Strategy>
class MBOSimulationEngine {
public:
  MBOSimulationEngine(Consumer &consumer, NetworkSimulator &network_sim,
                      VirtualExchange<LOB> &virtual_exchange, LOB &lob,
                      OrderManagementSystem &oms, Strategy &strategy,
                      performance::LatencyProfiler &profiler)
      : consumer_(consumer), network_sim_(network_sim),
        virtual_exchange_(virtual_exchange), lob_(lob), oms_(oms),
        strategy_(strategy), profiler_(profiler), engine_time_ns_(0) {}

  void run();

private:
  void route_internal_event(const EventV2 &ev);

  Consumer &consumer_;
  NetworkSimulator &network_sim_;
  VirtualExchange<LOB> &virtual_exchange_;
  LOB &lob_;
  OrderManagementSystem &oms_;
  Strategy &strategy_;
  performance::LatencyProfiler &profiler_;
  uint64_t engine_time_ns_;
};

// Helper for std::visit overload matching
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <DataConsumerConcept Consumer, LimitOrderBookConcept LOB,
          StrategyConcept<LOB> Strategy>
void MBOSimulationEngine<Consumer, LOB, Strategy>::route_internal_event(
    const EventV2 &ev) {
  std::visit(
      overloaded{[&](const CreateOrderEvent & /*payload*/) {
                   virtual_exchange_.on_update(ev);
                 },
                 [&](const CancelOrderEvent & /*payload*/) {
                   virtual_exchange_.on_update(ev);
                 },
                 [&](const AckCreateOrderEvent &payload) {
                   oms_.ack_create_order(payload.order_id);
                   strategy_.on_order_accepted(payload.order_id);
                 },
                 [&](const AckCancelOrderEvent &payload) {
                   oms_.ack_cancel_order(payload.order_id);
                   strategy_.on_order_canceled(payload.order_id);
                 },
                 [&](const AckFillOrderEvent &payload) {
                   oms_.ack_fill_order(payload.order_id, payload.filled_qty,
                                       payload.price);
                   strategy_.on_order_filled(payload.order_id,
                                             payload.filled_qty, payload.price);
                 }},
      ev.payload);
}

template <DataConsumerConcept Consumer, LimitOrderBookConcept LOB,
          StrategyConcept<LOB> Strategy>
void MBOSimulationEngine<Consumer, LOB, Strategy>::run() {
  bool reading_feed = true;

  while (reading_feed || !network_sim_.get_event_queue().empty()) {
    uint64_t next_feed_ts = UINT64_MAX;
    databento::MboMsg msg{};

    if (reading_feed) {
      if (!consumer_.try_poll(msg))
        continue;

      if (msg == databento::MboMsg{}) {
        reading_feed = false;
        continue;
      }
      profiler_.increment_throughput_counter();
      next_feed_ts = msg.ts_recv.time_since_epoch().count();
    }

    // Process all internal events that occurred BEFORE or AT the incoming
    // market data timestamp
    while (!network_sim_.get_event_queue().empty() &&
           network_sim_.get_event_queue().top().timestamp_ns <= next_feed_ts) {
      EventV2 ev = network_sim_.get_event_queue().top();
      network_sim_.get_event_queue().pop();

      engine_time_ns_ = ev.timestamp_ns;
      route_internal_event(ev);
    }

    // Apply Market Message
    if (reading_feed) {
      engine_time_ns_ = next_feed_ts;
      switch (msg.action) {
      case databento::Action::Add:
      case databento::Action::Modify:
      case databento::Action::Cancel: {
        uint64_t start_tsc = performance::get_tsc();
        lob_.update_book(msg);
        uint64_t end_tsc = performance::get_tsc();

        performance::Metric metric = performance::Metric::LOB_ADD;
        if (msg.action == databento::Action::Cancel) {
          metric = performance::Metric::LOB_CANCEL;
        } else if (msg.action == databento::Action::Modify) {
          metric = performance::Metric::LOB_MODIFY;
        }

        profiler_.record_latency(metric, end_tsc - start_tsc);

        strategy_.on_book_update(engine_time_ns_, start_tsc, lob_);
        break;
      }
      case databento::Action::Clear: {
        // We omit Clear from timing metrics because its O(N) memory wipe
        // will skew our microsecond Hot Path histograms and typically
        // only happens at market boundaries.
        uint64_t start_tsc = performance::get_tsc();
        lob_.update_book(msg);
        strategy_.on_book_update(engine_time_ns_, start_tsc, lob_);
        break;
      }
      case databento::Action::Fill:
        virtual_exchange_.on_fill(msg);
        break;
      case databento::Action::Trade:
      case databento::Action::None:
        break;
      }
    }
  }
}

} // namespace backtesting_engine::mbo
