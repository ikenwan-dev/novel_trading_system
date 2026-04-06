#pragma once

#include "LimitOrderBook/LimitOrderBookConcept.h"
#include "Performance/LatencyProfiler.h"
#include "Strategies/MBOStrategies/MBOStrategyBase.h"
#include <optional>

namespace backtesting_engine::mbo {

class MarketMaker : public MBOStrategyBase<MarketMaker> {
public:
  MarketMaker(OrderManagementSystem &oms, int64_t half_spread_ticks,
              uint64_t order_qty,
              performance::LatencyProfiler *profiler = nullptr);

  template <LimitOrderBookConcept LOB>
  void impl_on_book_update(int64_t timestamp_ns, const LOB &lob) {
    auto bbo = lob.get_bbo();
    if (bbo.first == 0 || bbo.second == 0 ||
        bbo.first == databento::kUndefPrice ||
        bbo.second == databento::kUndefPrice) {
      cancel_active_orders(timestamp_ns);
      return;
    }

    int64_t mid_price = (bbo.first + bbo.second) / 2;
    last_valid_mid_price_ = mid_price;

    int64_t skew = 0;
    if (current_position_ > 0)
      skew = -half_spread_ticks_ / 2;
    if (current_position_ < 0)
      skew = half_spread_ticks_ / 2;

    int64_t target_bid = mid_price - half_spread_ticks_ + skew;
    int64_t target_ask = mid_price + half_spread_ticks_ + skew;

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

  void impl_on_fill(const databento::MboMsg &order);

  // CRTP network simulator callbacks
  void impl_on_order_accepted(uint64_t order_id);
  void impl_on_order_canceled(uint64_t order_id);
  void impl_on_order_filled(uint64_t order_id, uint64_t filled_qty,
                            int64_t price);

  int64_t get_position() const { return current_position_; }
  int64_t get_last_valid_mid_price() const { return last_valid_mid_price_; }

private:
  void cancel_active_orders(int64_t timestamp_ns);

  int64_t half_spread_ticks_;
  uint64_t order_qty_;

  std::optional<uint64_t> active_bid_id_;
  std::optional<uint64_t> active_ask_id_;
  int64_t active_bid_price_{0};
  int64_t active_ask_price_{0};

  int64_t current_position_{0};
  int64_t last_valid_mid_price_{0};
};

} // namespace backtesting_engine::mbo
