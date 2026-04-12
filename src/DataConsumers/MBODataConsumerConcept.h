#pragma once
#include <concepts>
#include <databento/record.hpp>

namespace backtesting_engine::mbo {

template <typename T>
concept DataConsumerConcept = requires(T consumer, databento::MboMsg &out_msg) {
  { consumer.try_poll(out_msg) } -> std::same_as<bool>;
};

} // namespace backtesting_engine::mbo
