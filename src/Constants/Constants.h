#pragma once
#include <cstddef>
#include <cstdint>

namespace backtesting_engine {
namespace Constants {
static constexpr size_t RING_BUFFER_SIZE = 65536;
static constexpr uint64_t OUTBOUND_LATENCY = 10000; // 10 microseconds
static constexpr uint64_t INBOUND_LATENCY = 10000;  // 10 microseconds
} // namespace Constants

} // namespace backtesting_engine
