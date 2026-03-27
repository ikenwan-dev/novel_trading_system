#pragma once
#include "RiskResult.h"
#include <databento/enums.hpp>
#include <cstdint>
#include <cstdlib>

namespace backtesting_engine::mbo {

class MBORiskManager {
public:
    MBORiskManager(uint64_t max_order_qty, int64_t max_position)
        : max_order_qty_(max_order_qty), max_position_(max_position) {}

    // O(1) Pre-trade check
    inline RiskResult check_order(databento::Side side, uint64_t qty, int64_t price) const {
        if (qty > max_order_qty_) {
            return RiskResult::REJECTED_FAT_FINGER_QTY;
        }
        
        int64_t pos_change = (side == databento::Side::Ask) ? -static_cast<int64_t>(qty) : static_cast<int64_t>(qty);
        if (std::abs(current_position_ + pos_change) > max_position_) {
            return RiskResult::REJECTED_MAX_POSITION;
        }

        // Add additional $O(1)$ fast-path checks here as needed

        return RiskResult::APPROVED;
    }

    // Post-trade updates (to be hooked up into the event loop or OMS)
    inline void on_fill(databento::Side side, uint64_t filled_qty, int64_t /*price*/) {
        current_position_ += (side == databento::Side::Ask) ? -static_cast<int64_t>(filled_qty) : static_cast<int64_t>(filled_qty);
    }

private:
    uint64_t max_order_qty_;
    int64_t max_position_;
    int64_t current_position_{0};
};

} // namespace backtesting_engine::mbo
