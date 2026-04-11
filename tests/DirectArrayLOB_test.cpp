#include "LimitOrderBook/DataBentoLOB/DirectArrayLOB.h"
#include <gtest/gtest.h>
#include <memory>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

class DirectArrayLOBTest : public ::testing::Test {
protected:
  std::unique_ptr<DirectArrayLOB> lob_ptr;
  DirectArrayLOB &lob;

  // 1-cent tick size scaled for Databento FixedPriceScale (1e9)
  // 0.01 * 1,000,000,000 = 10,000,000
  static constexpr int64_t TICK_SIZE = 10000000;

  DirectArrayLOBTest()
      : lob_ptr(std::make_unique<DirectArrayLOB>(TICK_SIZE)), lob(*lob_ptr) {}

  // Helper to create a dummy MboMsg
  databento::MboMsg create_msg(uint64_t order_id, int64_t price, uint32_t size,
                               databento::Action action, databento::Side side,
                               databento::FlagSet flags = {}) {
    databento::MboMsg msg{};
    msg.order_id = order_id;
    msg.price = price;
    msg.size = size;
    msg.action = action;
    msg.side = side;
    msg.flags = flags;
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch());
    msg.ts_recv = databento::UnixNanos{duration};
    return msg;
  }
};

TEST_F(DirectArrayLOBTest, InitialState) {
  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DirectArrayLOBTest, AddBuyOrder) {
  auto msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                        databento::Action::Add, databento::Side::Bid);
  lob.update_book(msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 100 * databento::kFixedPriceScale);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DirectArrayLOBTest, AddSellOrder) {
  auto msg = create_msg(2, 110 * databento::kFixedPriceScale, 10,
                        databento::Action::Add, databento::Side::Ask);
  lob.update_book(msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, 110 * databento::kFixedPriceScale);
}

TEST_F(DirectArrayLOBTest, AddBidAndAsk) {
  auto buy_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  auto sell_msg = create_msg(2, 110 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Ask);

  lob.update_book(buy_msg);
  lob.update_book(sell_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 100 * databento::kFixedPriceScale);
  EXPECT_EQ(bbo.second, 110 * databento::kFixedPriceScale);
}

TEST_F(DirectArrayLOBTest, ModifyOrderPrice) {
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  auto mod_msg = create_msg(1, 105 * databento::kFixedPriceScale, 10,
                            databento::Action::Modify, databento::Side::Bid);
  lob.update_book(mod_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 105 * databento::kFixedPriceScale);
}

TEST_F(DirectArrayLOBTest, ModifyOrderSize) {
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  auto mod_msg = create_msg(1, 100 * databento::kFixedPriceScale, 20,
                            databento::Action::Modify, databento::Side::Bid);
  lob.update_book(mod_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 100 * databento::kFixedPriceScale);
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            20);
}

TEST_F(DirectArrayLOBTest, CancelOrder) {
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  auto cancel_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                               databento::Action::Cancel, databento::Side::Bid);
  lob.update_book(cancel_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
}

TEST_F(DirectArrayLOBTest, ClearBook) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(2, 110 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Ask));

  databento::MboMsg clear_msg{};
  clear_msg.action = databento::Action::Clear;
  lob.update_book(clear_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DirectArrayLOBTest, TopOfBookLinearDepletion) {
  // Ensure the linear while loop successfully finds the next BBO when top of
  // book is completely removed.
  lob.update_book(create_msg(10, 101 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(11, 100 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Bid));

  EXPECT_EQ(lob.get_bbo().first, 101 * databento::kFixedPriceScale);

  // Cancel the highest bid (101)
  lob.update_book(create_msg(10, 101 * databento::kFixedPriceScale, 10,
                             databento::Action::Cancel, databento::Side::Bid));

  // Best bid must successfully fall down to 100.
  EXPECT_EQ(lob.get_bbo().first, 100 * databento::kFixedPriceScale);
}

TEST_F(DirectArrayLOBTest, MaskIndexAccuracy) {
  // Add a very high price that tests the mask truncation and memory bounding
  int64_t base_price = 100 * databento::kFixedPriceScale;
  int64_t extreme_price = base_price +
                          (DirectArrayLOB::LOB_CAPACITY * TICK_SIZE) +
                          (10 * TICK_SIZE);

  lob.update_book(create_msg(99, extreme_price, 50, databento::Action::Add,
                             databento::Side::Bid));

  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid, extreme_price), 50);
}
