#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include <gtest/gtest.h>
#include <memory>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

class DataBentoLOBTest : public ::testing::Test {
protected:
  std::unique_ptr<DataBentoLOB> lob_ptr;
  DataBentoLOB& lob;

  DataBentoLOBTest() 
      : lob_ptr(std::make_unique<DataBentoLOB>()), 
        lob(*lob_ptr) {}

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

TEST_F(DataBentoLOBTest, InitialState) {
  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DataBentoLOBTest, AddBuyOrder) {
  auto msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                        databento::Action::Add, databento::Side::Bid);
  lob.update_book(msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 100 * databento::kFixedPriceScale);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DataBentoLOBTest, AddSellOrder) {
  auto msg = create_msg(2, 110 * databento::kFixedPriceScale, 10,
                        databento::Action::Add, databento::Side::Ask);
  lob.update_book(msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, 110 * databento::kFixedPriceScale);
}

TEST_F(DataBentoLOBTest, AddBidAndAsk) {
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

TEST_F(DataBentoLOBTest, ModifyOrderPrice) {
  // Add original order
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  // Modify price to 105
  auto mod_msg = create_msg(1, 105 * databento::kFixedPriceScale, 10,
                            databento::Action::Modify, databento::Side::Bid);
  lob.update_book(mod_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 105 * databento::kFixedPriceScale);
}

TEST_F(DataBentoLOBTest, ModifyOrderSize) {
  // Add original order
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  // Modify size (price stays same)
  // Note: DataBentoLOB::modify_order logic uses msg.price to update order if
  // changed. If price is same, it just updates size. We can't easily check size
  // via get_bbo, but we can check price is still there.
  auto mod_msg = create_msg(1, 100 * databento::kFixedPriceScale, 20,
                            databento::Action::Modify, databento::Side::Bid);
  lob.update_book(mod_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, 100 * databento::kFixedPriceScale);
}

TEST_F(DataBentoLOBTest, CancelOrder) {
  // Add order
  auto add_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                            databento::Action::Add, databento::Side::Bid);
  lob.update_book(add_msg);

  // Cancel order
  // Note: Cancel message usually has the same price/side as the order being
  // cancelled But strictly speaking, LOB might look up by Order ID.
  // DataBentoLOB::cancel_order uses Order ID to find it.
  auto cancel_msg = create_msg(1, 100 * databento::kFixedPriceScale, 10,
                               databento::Action::Cancel, databento::Side::Bid);
  lob.update_book(cancel_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
}

TEST_F(DataBentoLOBTest, ClearBook) {
  // Add orders
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(2, 110 * databento::kFixedPriceScale, 10,
                             databento::Action::Add, databento::Side::Ask));

  // Clear
  databento::MboMsg clear_msg{};
  clear_msg.action = databento::Action::Clear;
  lob.update_book(clear_msg);

  auto bbo = lob.get_bbo();
  EXPECT_EQ(bbo.first, databento::kUndefPrice);
  EXPECT_EQ(bbo.second, databento::kUndefPrice);
}

TEST_F(DataBentoLOBTest, AddOrderQuantity) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 15,
                             databento::Action::Add, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            15);

  lob.update_book(create_msg(2, 100 * databento::kFixedPriceScale, 20,
                             databento::Action::Add, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            35);
}

TEST_F(DataBentoLOBTest, CancelOrderQuantity) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 15,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 15,
                             databento::Action::Cancel, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            0);
}

TEST_F(DataBentoLOBTest, ModifyOrderDecreaseQuantity) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 20,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 8,
                             databento::Action::Modify, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            8);
}

TEST_F(DataBentoLOBTest, ModifyOrderIncreaseQuantity) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 20,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 30,
                             databento::Action::Modify, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            30);
}

TEST_F(DataBentoLOBTest, ModifyOrderPriceChange) {
  lob.update_book(create_msg(1, 100 * databento::kFixedPriceScale, 20,
                             databento::Action::Add, databento::Side::Bid));
  lob.update_book(create_msg(1, 105 * databento::kFixedPriceScale, 20,
                             databento::Action::Modify, databento::Side::Bid));
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              100 * databento::kFixedPriceScale),
            0);
  EXPECT_EQ(lob.get_level_qty(databento::Side::Bid,
                              105 * databento::kFixedPriceScale),
            20);
}
