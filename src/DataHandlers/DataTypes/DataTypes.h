// File containing simple finicial data types (Bar, Tick)
#pragma once
#include <chrono>
#include <string>


namespace backtesting_engine::bar {
struct Bar {
  std::string symbol;
  std::chrono::system_clock::time_point timestamp;
  double open;
  double high;
  double low;
  double close;
  double volume;
  // This allows std::sort to work
  bool operator<(const Bar &other) const { return timestamp < other.timestamp; }
};
} // namespace backtesting_engine::bar
