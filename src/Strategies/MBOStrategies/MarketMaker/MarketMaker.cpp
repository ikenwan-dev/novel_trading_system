#include "MarketMaker.h"

namespace backtesting_engine::mbo {

MarketMaker::MarketMaker(OrderManagementSystem &oms, int64_t half_spread_ticks,
                         uint64_t order_qty, performance::LatencyProfiler *profiler)
    : MBOStrategyBase<MarketMaker>(oms, profiler), half_spread_ticks_(half_spread_ticks),
      order_qty_(order_qty) {}

void MarketMaker::impl_on_book_update(int64_t timestamp_ns,
                                      const DataBentoLOB &lob) {
  auto bbo = lob.get_bbo();
  if (bbo.first == 0 || bbo.second == 0 ||
      bbo.first == databento::kUndefPrice ||
      bbo.second == databento::kUndefPrice) {
    // HFT Best Practice: If the book is empty or data is invalid, immediately
    // pull all quotes!
    cancel_active_orders(timestamp_ns);
    return;
  }

  int64_t mid_price = (bbo.first + bbo.second) / 2;
  last_valid_mid_price_ = mid_price;

  int64_t skew = 0;
  // Inventory risk management: Shift quotes down to get flat if long
  if (current_position_ > 0)
    skew = -half_spread_ticks_ / 2;
  // Shift quotes up to buy back if short
  if (current_position_ < 0)
    skew = half_spread_ticks_ / 2;

  int64_t target_bid = mid_price - half_spread_ticks_ + skew;
  int64_t target_ask = mid_price + half_spread_ticks_ + skew;

  // Price Drift Cancellation Logic
  if (active_bid_id_.has_value() && active_bid_price_ != target_bid) {
    cancel_order(timestamp_ns, *active_bid_id_);
    active_bid_id_ = std::nullopt;
  } else if (!active_bid_id_.has_value()) {
    auto [bid_id, risk] =
        send_order(timestamp_ns, target_bid, order_qty_, databento::Side::Bid);
    if (risk == RiskResult::APPROVED) {
      active_bid_id_ = bid_id;
      active_bid_price_ = target_bid;
    }
  }

  if (active_ask_id_.has_value() && active_ask_price_ != target_ask) {
    cancel_order(timestamp_ns, *active_ask_id_);
    active_ask_id_ = std::nullopt;
  } else if (!active_ask_id_.has_value()) {
    auto [ask_id, risk] =
        send_order(timestamp_ns, target_ask, order_qty_, databento::Side::Ask);
    if (risk == RiskResult::APPROVED) {
      active_ask_id_ = ask_id;
      active_ask_price_ = target_ask;
    }
  }
}

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

} // namespace backtesting_engine::mbo
