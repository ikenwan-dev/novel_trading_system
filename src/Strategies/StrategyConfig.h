#include <map>
#include <string>
struct StrategyConfig {
  std::string name; // Strategy name

  // Additional Parameters
  std::map<std::string, double> params;
  std::map<std::string, std::string> ticker_map;
};