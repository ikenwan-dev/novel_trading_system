#pragma once
#include <concepts>
#include <cstdint>
#include <databento/enums.hpp>
#include <databento/record.hpp>
#include <utility>

namespace backtesting_engine::mbo {

/**
 * @brief Concept for a Limit Order Book (LOB) implementation.
 * 
 * This ensures that any LOB implementation provided to the VirtualExchange
 * or SimulationEngine provides the necessary high-performance interface.
 */
template <typename T>
concept LimitOrderBookConcept = requires(const T lob, T mutable_lob, 
                                        const databento::MboMsg& msg, 
                                        databento::Side side, 
                                        int64_t price) {
  // Hot path book updates
  { mutable_lob.update_book(msg) } -> std::same_as<void>;
  
  // Query interface
  { lob.get_bbo() } -> std::same_as<std::pair<int64_t, int64_t>>;
  { lob.get_level_qty(side, price) } -> std::same_as<uint64_t>;
  { lob.get_total_volume() } -> std::same_as<uint64_t>;
};

} // namespace backtesting_engine::mbo
