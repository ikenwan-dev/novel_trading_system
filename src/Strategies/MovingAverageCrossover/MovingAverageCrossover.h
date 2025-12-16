#pragma once
#include "../../ThreadSafeQueue/ThreadSafeQueue.h"
#include "../Strategy.h"
#include <deque>
#include <map>
#include <string>
#include <vector>

class MovingAverageCrossover : public Strategy {
public:
  MovingAverageCrossover(ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
                         const std::vector<std::string> &tickers,
                         int short_window, int long_window);
  void on_market_data(const MarketEvent &event) override;

private:
  double calculate_moving_average(const std::deque<double> &prices);
  ThreadSafeQueue<std::shared_ptr<Event>> &event_queue_;
  const std::vector<std::string> tickers_;
  const int short_window_;
  const int long_window_;

  // Map of ticker to deque of prices
  std::map<std::string, std::deque<double>> price_history_;
  std::map<std::string, double> short_mas_;
  std::map<std::string, double> long_mas_;
};