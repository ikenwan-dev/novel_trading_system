#include "OrderManagementSystem/OrderManagementSystem.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "RiskManager/Mbo/MBORiskManager.h"
#include "Events/Mbo/MboEvent.h"
#include <gtest/gtest.h>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

class OrderManagementSystemTest : public ::testing::Test {
protected:
  EventQueue queue;
  NetworkSimulator sim;
  MBORiskManager risk;
  OrderManagementSystem oms;

  OrderManagementSystemTest() : queue(), sim(queue, 0, 0), risk(1000000, 1000000), oms(sim, risk, 100) {}
};

TEST_F(OrderManagementSystemTest, CreateAndGetOrder) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
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
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.ack_create_order(order_id);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::LIVE);
}

TEST_F(OrderManagementSystemTest, FillOrderPartial) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.ack_create_order(order_id);
  oms.ack_fill_order(order_id, 40, 150);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.filled_qty, 40);
  EXPECT_EQ(order.status,
            OrderManagementSystem::OMSOrderStatus::PARTIALLY_FILLED);
}

TEST_F(OrderManagementSystemTest, FillOrderComplete) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.ack_create_order(order_id);
  oms.ack_fill_order(order_id, 100, 150);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.filled_qty, 100);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::FILLED);
}

TEST_F(OrderManagementSystemTest, FillOrderThrowsOnOverfill) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.ack_create_order(order_id);
  EXPECT_THROW(oms.ack_fill_order(order_id, 150, 150), std::runtime_error);
}

TEST_F(OrderManagementSystemTest, CancelOrder) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.cancel_order(0, order_id);
  oms.ack_cancel_order(order_id);

  const auto &order = oms.get_order(order_id);
  EXPECT_EQ(order.status, OrderManagementSystem::OMSOrderStatus::CANCELLED);
}

TEST_F(OrderManagementSystemTest, CancelIgnoresOnAlreadyFilled) {
  auto order_id = oms.create_order(0, 150, 100, databento::Side::Bid).first;
  oms.ack_create_order(order_id);
  oms.ack_fill_order(order_id, 100, 150);

  EXPECT_NO_THROW(oms.cancel_order(0, order_id));
  EXPECT_EQ(oms.get_order(order_id).status, OrderManagementSystem::OMSOrderStatus::FILLED);
}

TEST_F(OrderManagementSystemTest, MaxOrdersCapacityThrows) {
  EventQueue small_queue;
  NetworkSimulator small_sim(small_queue, 0, 0);
  MBORiskManager small_risk(1000000, 1000000);
  OrderManagementSystem small_oms(small_sim, small_risk, 1);
  small_oms.create_order(0, 150, 100, databento::Side::Bid);

  EXPECT_THROW(small_oms.create_order(0, 150, 100, databento::Side::Bid),
               std::runtime_error);
}
