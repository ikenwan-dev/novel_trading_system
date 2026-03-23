#include <map>
#include <string>

namespace backtesting_engine::bar {
struct StrategyConfig {
  std::string name; // Strategy name

  // Additional Parameters
  std::map<std::string, double> params;
  std::map<std::string, std::string> ticker_map;
};
} // namespace backtesting_engine::bar
