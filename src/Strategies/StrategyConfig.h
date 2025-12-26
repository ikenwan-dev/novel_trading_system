#include <map>
#include <string>
#include <vector>
struct StrategyConfig {
  std::string name; // Strategy name
  std::vector<std::string> tickers;

  // Additional Parameters
  std::map<std::string, double> params;
};