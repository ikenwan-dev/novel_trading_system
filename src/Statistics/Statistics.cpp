#include "Statistics.h"

const int YEAR_IN_SECONDS = 365 * 24 * 60 * 60;
double
Statistics::calculate_cagr(const std::vector<EquityDataPoint> &equity_curve) {
  double start_value = equity_curve.front().value;
  double end_value = equity_curve.back().value;
  auto duration =
      equity_curve.back().timestamp - equity_curve.front().timestamp;
  double years =
      std::chrono::duration<double, std::ratio<YEAR_IN_SECONDS>>(duration)
          .count();
  return std::pow(end_value / start_value, 1.0 / years) - 1.0;
}