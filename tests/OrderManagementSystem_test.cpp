#include "OrderManagementSystem/OrderManagementSystem.h"
#include <gtest/gtest.h>

class OrderManagementSystemTest : public ::testing::Test {
protected:
  OrderManagementSystem oms;

  OrderManagementSystemTest() : oms(100) {}
};

TEST_F(OrderManagementSystemTest, CreateAndGetOrder) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  EXPECT_EQ(order_id, 0);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.order_id, 0);
  EXPECT_EQ(order.price, 150);
  EXPECT_EQ(order.qty, 100);
  EXPECT_EQ(order.filled_qty, 0);
  EXPECT_EQ(order.side, databento::Side::Bid);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::PENDING);
}

TEST_F(OrderManagementSystemTest, AckOrder) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  oms.ack_order(order_id);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::LIVE);
}

TEST_F(OrderManagementSystemTest, FillOrderPartial) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  oms.fill_order(order_id, 40);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.filled_qty, 40);
  EXPECT_EQ(order.status,
            OrderManagementSystem::OMSOrderStatus::PARTIALLY_FILLED);
}

TEST_F(OrderManagementSystemTest, FillOrderComplete) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  oms.fill_order(order_id, 100);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.filled_qty, 100);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::FILLED);
}

TEST_F(OrderManagementSystemTest, FillOrderThrowsOnOverfill) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  EXPECT_THROW(oms.fill_order(order_id, 150), std::runtime_error);
}

TEST_F(OrderManagementSystemTest, CancelOrder) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  oms.cancel_order(order_id);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::CANCELLED);
}

TEST_F(OrderManagementSystemTest, CancelThrowsOnAlreadyFilled) {
  auto order_id = oms.create_order(150, 100, databento::Side::Bid);
  oms.fill_order(order_id, 100);

  EXPECT_THROW(oms.cancel_order(order_id), std::runtime_error);
}

TEST_F(OrderManagementSystemTest, MaxOrdersCapacityThrows) {
  OrderManagementSystem small_oms(1);
  small_oms.create_order(150, 100, databento::Side::Bid);

  EXPECT_THROW(small_oms.create_order(150, 100, databento::Side::Bid),
               std::runtime_error);
}
