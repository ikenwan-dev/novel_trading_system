#include <chrono>
#include <vector>


namespace backtesting_engine {
struct EquityDataPoint {
  std::chrono::system_clock::time_point timestamp;
  double value;
};

class Statistics {
public:
  static double
  calculate_cagr(const std::vector<EquityDataPoint> &equity_curve);
};
} // namespace backtesting_engine
