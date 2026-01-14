#include "Events/SignalEvent/SignalEvent.h"
#include "Strategies/MovingAverageCrossover/MovingAverageCrossover.h"
#include "gtest/gtest.h"
#include <chrono>
#include <memory>

class MovingAverageCrossoverFixture : public ::testing::Test {
protected:
  std::unique_ptr<MovingAverageCrossover> mac;

  double call_calculate_moving_average(const std::deque<double> &prices) {
    return mac->calculate_moving_average(prices);
  }

  void initialize(ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
                  const std::vector<std::string> &tickers, int short_window,
                  int long_window) {
    mac = std::make_unique<MovingAverageCrossover>(event_queue, tickers,
                                                   short_window, long_window);
  }
};

TEST_F(MovingAverageCrossoverFixture, CalculatesMovingAverage) {
  ThreadSafeQueue<std::shared_ptr<Event>> event_queue;
  initialize(event_queue, {"AAPL"}, 2, 5);
  std::deque<double> prices = {10.0, 20.0, 30.0};
  double ma = call_calculate_moving_average(prices);
  EXPECT_DOUBLE_EQ(ma, 20.0);
}

TEST_F(MovingAverageCrossoverFixture, EmitsLongSignalOnCrossover) {
  ThreadSafeQueue<std::shared_ptr<Event>> event_queue;
  std::vector<std::string> tickers = {"AAPL"};
  // Short 2, Long 3
  initialize(event_queue, tickers, 2, 3);

  // Fill up the history
  auto now = std::chrono::system_clock::now();
  mac->on_market_data(MarketEvent("AAPL", now, 10.0, 10.0, 10.0, 10.0, 100));
  mac->on_market_data(MarketEvent("AAPL", now, 10.0, 10.0, 10.0, 10.0, 100));
  mac->on_market_data(MarketEvent("AAPL", now, 10.0, 10.0, 10.0, 10.0, 100));

  // Current Short MA (2) = (10+10)/2 = 10.0
  // Current Long MA (3) = (10+10+10)/3 = 10.0
  // prev_short = 10.0, prev_long = 10.0

  // To get a LONG signal, we need:
  // prev_short <= prev_long (True: 10 <= 10)
  // current_short > current_long

  // Let's push a high price
  mac->on_market_data(MarketEvent("AAPL", now, 20.0, 20.0, 20.0, 20.0, 100));
  // New Short MA (2) = (10+20)/2 = 15.0
  // New Long MA (3) = (10+10+20)/3 = 13.33
  // 15.0 > 13.33 -> SHOULD EMIT SIGNAL

  std::shared_ptr<Event> event;
  bool popped = event_queue.try_pop(event);

  ASSERT_TRUE(popped);
  ASSERT_EQ(event->get_type(), EventType::Signal);
  auto signal = std::dynamic_pointer_cast<SignalEvent>(event);
  EXPECT_EQ(signal->direction_, SignalDirection::LONG);
}