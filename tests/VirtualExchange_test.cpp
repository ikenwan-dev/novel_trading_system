#include "VirtualExchange/VirtualExchange.h"
#include <gtest/gtest.h>
#include "Performance/SimulationProfiler.h"

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

class VirtualExchangeTest : public ::testing::Test {
protected:
  EventQueue event_queue;
  NetworkSimulator simulator;
  DataBentoLOB lob;
  performance::SimulationProfiler sim_profiler;
  VirtualExchange vx;

  VirtualExchangeTest()
      : simulator(event_queue, 0, 0), lob(), sim_profiler(), vx(simulator, lob, sim_profiler) {}

  databento::MboMsg create_mbo_msg(uint64_t order_id, int64_t price,
                                   uint32_t size, databento::Action action,
                                   databento::Side side) {
    databento::MboMsg msg{};
    msg.order_id = order_id;
    msg.price = price;
    msg.size = size;
    msg.action = action;
    msg.side = side;
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch());
    msg.ts_recv = databento::UnixNanos{duration};
    return msg;
  }
};

TEST_F(VirtualExchangeTest, AddCreateOrderEvent) {
  EventV2 event;
  event.timestamp_ns = 1000;
  event.payload = CreateOrderEvent{1, 100 * databento::kFixedPriceScale, 10,
                                   databento::Side::Bid};

  vx.on_update(event);

  ASSERT_FALSE(event_queue.empty());
  auto response = event_queue.top();
  event_queue.pop();

  ASSERT_TRUE(std::holds_alternative<AckCreateOrderEvent>(response.payload));
  auto ack = std::get<AckCreateOrderEvent>(response.payload);
  EXPECT_EQ(ack.order_id, 1);
}

TEST_F(VirtualExchangeTest, CancelOrderRemovesBookLevelIfEmpty) {
  EventV2 create_event;
  create_event.timestamp_ns = 1000;
  create_event.payload = CreateOrderEvent{1, 100 * databento::kFixedPriceScale,
                                          10, databento::Side::Bid};
  vx.on_update(create_event);
  event_queue.pop(); // pop AckCreate

  EventV2 cancel_event;
  cancel_event.timestamp_ns = 2000;
  cancel_event.payload = CancelOrderEvent{1};
  vx.on_update(cancel_event);

  ASSERT_FALSE(event_queue.empty());
  auto response = event_queue.top();
  event_queue.pop();

  ASSERT_TRUE(std::holds_alternative<AckCancelOrderEvent>(response.payload));
  auto ack = std::get<AckCancelOrderEvent>(response.payload);
  EXPECT_EQ(ack.order_id, 1);
}

TEST_F(VirtualExchangeTest, FillOrdersWithZeroMarketDifference) {
  // Adding two virtual orders consecutively. Since no real market events
  // happened between them, the internal physical volume between them is 0.

  // Order 1
  EventV2 ev1;
  ev1.timestamp_ns = 1000;
  ev1.payload = CreateOrderEvent{1, 100 * databento::kFixedPriceScale, 10,
                                 databento::Side::Bid};
  vx.on_update(ev1);
  event_queue.pop(); // pop ack

  // Order 2
  EventV2 ev2;
  ev2.timestamp_ns = 2000;
  ev2.payload = CreateOrderEvent{2, 100 * databento::kFixedPriceScale, 15,
                                 databento::Side::Bid};
  vx.on_update(ev2);
  event_queue.pop(); // pop ack

  // A fill of 15 comes in
  auto msg = create_mbo_msg(0, 100 * databento::kFixedPriceScale, 15,
                            databento::Action::Trade, databento::Side::Bid);
  vx.on_fill(msg);

  // We expect order 1 to be fully filled (10 qty)
  ASSERT_FALSE(event_queue.empty());
  auto response1 = event_queue.top();
  event_queue.pop();
  ASSERT_TRUE(std::holds_alternative<AckFillOrderEvent>(response1.payload));
  auto fill1 = std::get<AckFillOrderEvent>(response1.payload);
  EXPECT_EQ(fill1.order_id, 1);
  EXPECT_EQ(fill1.filled_qty, 10);

  // We expect order 2 to be partially filled (5 qty)
  ASSERT_FALSE(event_queue.empty());
  auto response2 = event_queue.top();
  event_queue.pop();
  ASSERT_TRUE(std::holds_alternative<AckFillOrderEvent>(response2.payload));
  auto fill2 = std::get<AckFillOrderEvent>(response2.payload);
  EXPECT_EQ(fill2.order_id, 2);
  EXPECT_EQ(fill2.filled_qty, 5);
}

TEST_F(VirtualExchangeTest, FillOrdersWithMarketQueueAhead) {
  // 1. 20 shares form in the physical market at price 100 (Bid).
  auto mkt_msg1 = create_mbo_msg(99, 100 * databento::kFixedPriceScale, 20,
                                 databento::Action::Add, databento::Side::Bid);
  lob.update_book(mkt_msg1);

  // 2. We submit our virtual Order 1 (behind the 20 physical shares).
  EventV2 ev1;
  ev1.timestamp_ns = 1000;
  ev1.payload = CreateOrderEvent{1, 100 * databento::kFixedPriceScale, 10,
                                 databento::Side::Bid};
  vx.on_update(ev1);
  event_queue.pop(); // pop ack

  // 3. 15 more physical shares arrive.
  auto mkt_msg2 = create_mbo_msg(100, 100 * databento::kFixedPriceScale, 15,
                                 databento::Action::Add, databento::Side::Bid);
  lob.update_book(mkt_msg2);

  // 4. We submit virtual Order 2.
  EventV2 ev2;
  ev2.timestamp_ns = 2000;
  ev2.payload = CreateOrderEvent{2, 100 * databento::kFixedPriceScale, 10,
                                 databento::Side::Bid};
  vx.on_update(ev2);
  event_queue.pop(); // pop ack

  // Total book represents: 20 physical -> [VO1: 10] -> 15 physical -> [VO2: 10]

  // Trade of size 15 happens.
  // It consumes 15 of the first 20 physical shares. Neither VO1 nor VO2 fill.
  auto trade1 = create_mbo_msg(0, 100 * databento::kFixedPriceScale, 15,
                               databento::Action::Trade, databento::Side::Bid);
  vx.on_fill(trade1);
  EXPECT_TRUE(event_queue.empty()); // No virtual fills!

  // Trade of size 10 happens.
  // Consumes remaining 5 physical shares in front of VO1, and 5 shares of VO1.
  auto trade2 = create_mbo_msg(0, 100 * databento::kFixedPriceScale, 10,
                               databento::Action::Trade, databento::Side::Bid);
  vx.on_fill(trade2);

  ASSERT_FALSE(event_queue.empty());
  auto response = event_queue.top();
  event_queue.pop();
  ASSERT_TRUE(std::holds_alternative<AckFillOrderEvent>(response.payload));
  auto fill = std::get<AckFillOrderEvent>(response.payload);
  EXPECT_EQ(fill.order_id, 1);
  EXPECT_EQ(fill.filled_qty, 5); // VO1 part fill 

  // Trade of size 30 happens.
  // Consumes remaining 5 shares of VO1 (5 filled).
  // Consumes 15 physical shares between VO1 and VO2 (20 remaining).
  // Consumes 10 shares of VO2 (10 remaining). VO2 Fully fills!
  auto trade3 = create_mbo_msg(0, 100 * databento::kFixedPriceScale, 30,
                               databento::Action::Trade, databento::Side::Bid);
  vx.on_fill(trade3);

  // VO1 fully fills (5 qty)
  ASSERT_FALSE(event_queue.empty());
  auto f1_res = event_queue.top();
  event_queue.pop();
  ASSERT_TRUE(std::holds_alternative<AckFillOrderEvent>(f1_res.payload));
  auto fill1_final = std::get<AckFillOrderEvent>(f1_res.payload);
  EXPECT_EQ(fill1_final.order_id, 1);
  EXPECT_EQ(fill1_final.filled_qty, 5);

  // VO2 fully fills (10 qty)
  ASSERT_FALSE(event_queue.empty());
  auto f2_res = event_queue.top();
  event_queue.pop();
  ASSERT_TRUE(std::holds_alternative<AckFillOrderEvent>(f2_res.payload));
  auto fill2_final = std::get<AckFillOrderEvent>(f2_res.payload);
  EXPECT_EQ(fill2_final.order_id, 2);
  EXPECT_EQ(fill2_final.filled_qty, 10);
}
