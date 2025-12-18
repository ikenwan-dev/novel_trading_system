#include "MovingAverageCrossover.h"
#include "Events/SignalEvent/SignalEvent.h"
#include <numeric>
MovingAverageCrossover::MovingAverageCrossover(
    ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
    const std::vector<std::string> &tickers, int short_window, int long_window)
    : event_queue_(event_queue), tickers_(tickers), short_window_(short_window),
      long_window_(long_window) {
  for (const auto &ticker : tickers_) {
    price_history_[ticker] = std::deque<double>();
    short_mas_[ticker] = 0.0;
    long_mas_[ticker] = 0.0;
  }
}

double MovingAverageCrossover::calculate_moving_average(
    const std::deque<double> &prices) {
  if (prices.empty()) {
    return 0.0;
  }
  return std::accumulate(prices.begin(), prices.end(), 0.0) / prices.size();
}

void MovingAverageCrossover::on_market_data(const MarketEvent &event) {
  auto it = price_history_.find(event.ticker_);
  if (it == price_history_.end())
    return; // Not tracking ticker

  auto &long_history = it->second;
  long_history.push_back(event.close_);

  if (long_history.size() > long_window_)
    long_history.pop_front(); // maintain long window
  if (long_history.size() < long_window_)
    return; // Return if not enough data

  std::deque<double> short_history(long_history.end() - short_window_,
                                   long_history.end());

  double prev_short_ma = short_mas_[event.ticker_];
  double prev_long_ma = long_mas_[event.ticker_];

  short_mas_[event.ticker_] = calculate_moving_average(short_history);
  long_mas_[event.ticker_] = calculate_moving_average(long_history);

  if (prev_short_ma == 0 && prev_long_ma == 0)
    return; // don't emit signal on first update

  if (prev_short_ma <= prev_long_ma &&
      short_mas_[event.ticker_] > long_mas_[event.ticker_]) {
    event_queue_.push(std::make_shared<SignalEvent>(
        event.ticker_, event.timestamp_, SignalDirection::LONG));
  } else if (prev_short_ma >= prev_long_ma &&
             short_mas_[event.ticker_] < long_mas_[event.ticker_]) {
    event_queue_.push(std::make_shared<SignalEvent>(
        event.ticker_, event.timestamp_, SignalDirection::SHORT));
  }
}
