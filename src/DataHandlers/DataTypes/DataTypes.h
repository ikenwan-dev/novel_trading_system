// File containing simple finicial data types (Bar, Tick)

#include <chrono>
#include <string>

struct Bar {
  std::string symbol;
  std::chrono::system_clock::time_point timestamp;
  double open;
  double high;
  double low;
  double close;
  int volume;
  // This allows std::sort to work
  bool operator<(const Bar &other) const { return timestamp < other.timestamp; }
};