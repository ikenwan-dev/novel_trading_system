#pragma once

#include "DataConsumers/DataConsumer.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "VirtualExchange/VirtualExchange.h"
#include <cstdint>
#include <databento/historical.hpp>
#include <variant>

namespace backtesting_engine::mbo {

template <typename Strategy> class MBOSimulationEngine {
public:
  MBOSimulationEngine(DataConsumer &consumer, NetworkSimulator &network_sim,
                      VirtualExchange &virtual_exchange, DataBentoLOB &lob,
                      OrderManagementSystem &oms, Strategy &strategy)
      : consumer_(consumer), network_sim_(network_sim),
        virtual_exchange_(virtual_exchange), lob_(lob), oms_(oms),
        strategy_(strategy), engine_time_ns_(0) {}

  void run();

private:
  void route_internal_event(const EventV2 &ev);

  DataConsumer &consumer_;
  NetworkSimulator &network_sim_;
  VirtualExchange &virtual_exchange_;
  DataBentoLOB &lob_;
  OrderManagementSystem &oms_;
  Strategy &strategy_;
  uint64_t engine_time_ns_;
};

// Helper for std::visit overload matching
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename Strategy>
void MBOSimulationEngine<Strategy>::route_internal_event(const EventV2 &ev) {
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

template <typename Strategy> void MBOSimulationEngine<Strategy>::run() {
  bool reading_feed = true;

  while (reading_feed || !network_sim_.get_event_queue().empty()) {
    uint64_t next_feed_ts = UINT64_MAX;
    databento::MboMsg msg{};

    if (reading_feed) {
      // Spin loop, polling the data consumer
      if (!consumer_.try_poll(msg))
        continue;

      if (msg == databento::MboMsg{}) {
        reading_feed = false; // End of stream
        continue;
      }
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
      case databento::Action::Cancel:
      case databento::Action::Clear:
        lob_.update_book(msg);
        strategy_.on_book_update(engine_time_ns_, lob_);
        break;
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
