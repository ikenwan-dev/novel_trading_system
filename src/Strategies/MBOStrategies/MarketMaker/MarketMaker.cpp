#include "MarketMaker.h"
#include "../MBOStrategyConcept.h"
#include "../../../LimitOrderBook/DataBentoLOB/DataBentoLOB.h"

namespace backtesting_engine::mbo {

MarketMaker::MarketMaker(OrderManagementSystem &oms, int64_t half_spread_ticks,
                         uint64_t order_qty, performance::LatencyProfiler *profiler)
    : MBOStrategyBase<MarketMaker>(oms, profiler), half_spread_ticks_(half_spread_ticks),
      order_qty_(order_qty) {}


void MarketMaker::cancel_active_orders(int64_t timestamp_ns) {
  if (active_bid_id_.has_value()) {
    cancel_order(timestamp_ns, *active_bid_id_);
    active_bid_id_ = std::nullopt;
  }
  if (active_ask_id_.has_value()) {
    cancel_order(timestamp_ns, *active_ask_id_);
    active_ask_id_ = std::nullopt;
  }
}

void MarketMaker::impl_on_fill(const databento::MboMsg & /*order*/) {
  // Physical live feed tracking logic (unused in virtual simulation)
}

void MarketMaker::impl_on_order_filled(uint64_t order_id, uint64_t filled_qty,
                                       int64_t /*price*/) {
  const auto &order = oms_.get_order(order_id);

  // Always update position deterministically regardless of whether we are
  // actively tracking it
  if (order.side == databento::Side::Bid) {
    current_position_ += filled_qty;
  } else if (order.side == databento::Side::Ask) {
    current_position_ -= filled_qty;
  }

  // Clear active tracking states if the active order is the one that fully
  // filled
  if (active_bid_id_ == order_id) {
    if (order.status == OrderManagementSystem::OMSOrderStatus::FILLED) {
      active_bid_id_ = std::nullopt;
      active_bid_price_ = 0;
    }
  } else if (active_ask_id_ == order_id) {
    if (order.status == OrderManagementSystem::OMSOrderStatus::FILLED) {
      active_ask_id_ = std::nullopt;
      active_ask_price_ = 0;
    }
  }
}

void MarketMaker::impl_on_order_accepted(uint64_t /*order_id*/) {
  // Sync tracked
}

void MarketMaker::impl_on_order_canceled(uint64_t order_id) {
  if (active_bid_id_ == order_id) {
    active_bid_id_ = std::nullopt;
    active_bid_price_ = 0;
  }
  if (active_ask_id_ == order_id) {
    active_ask_id_ = std::nullopt;
    active_ask_price_ = 0;
  }
}

static_assert(StrategyConcept<MarketMaker, DataBentoLOB>, "MarketMaker fails to implement StrategyConcept!");
} // namespace backtesting_engine::mbo
