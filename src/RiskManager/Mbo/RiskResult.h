#pragma once
#include <cstdint>

namespace backtesting_engine::mbo {
enum class RiskResult : uint8_t {
    APPROVED = 0,
    REJECTED_FAT_FINGER_QTY = 1,
    REJECTED_MAX_POSITION = 2,
    REJECTED_PRICE_BAND = 3
};
} // namespace backtesting_engine::mbo
