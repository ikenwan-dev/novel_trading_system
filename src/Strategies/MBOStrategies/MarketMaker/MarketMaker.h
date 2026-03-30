#pragma once

#include "Strategies/MBOStrategies/MBOStrategyBase.h"
#include <optional>

namespace backtesting_engine::mbo {

class MarketMaker : public MBOStrategyBase<MarketMaker> {
public:
    MarketMaker(OrderManagementSystem &oms, int64_t half_spread_ticks, uint64_t order_qty);

    void impl_on_book_update(int64_t timestamp_ns, const DataBentoLOB &lob);
    void impl_on_fill(const databento::MboMsg &order);
    
    // CRTP network simulator callbacks
    void impl_on_order_accepted(uint64_t order_id);
    void impl_on_order_canceled(uint64_t order_id);
    void impl_on_order_filled(uint64_t order_id, uint64_t filled_qty, int64_t price);

    int64_t get_position() const { return current_position_; }

private:
    void cancel_active_orders(int64_t timestamp_ns);

    int64_t half_spread_ticks_;
    uint64_t order_qty_;

    std::optional<uint64_t> active_bid_id_;
    std::optional<uint64_t> active_ask_id_;
    int64_t active_bid_price_{0};
    int64_t active_ask_price_{0};

    int64_t current_position_{0};
};

} // namespace backtesting_engine::mbo
