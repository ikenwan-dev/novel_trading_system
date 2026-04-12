#pragma once
#include <concepts>
#include <cstdint>

namespace backtesting_engine::mbo {

template <typename T, typename LOB>
concept StrategyConcept =
    requires(T strategy, uint64_t order_id, uint64_t qty, int64_t price,
             uint64_t ts, uint64_t tsc, const LOB &lob) {
      { strategy.on_order_accepted(order_id) } -> std::same_as<void>;
      { strategy.on_order_canceled(order_id) } -> std::same_as<void>;
      { strategy.on_order_filled(order_id, qty, price) } -> std::same_as<void>;
      { strategy.on_book_update(ts, tsc, lob) } -> std::same_as<void>;
    };

} // namespace backtesting_engine::mbo
