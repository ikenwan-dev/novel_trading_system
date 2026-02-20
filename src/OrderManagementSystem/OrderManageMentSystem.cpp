#include "OrderManagementSystem/OrderManagementSystem.h"
#include <cstdint>
#include <stdexcept>

OrderManagementSystem::OrderManagementSystem(std::size_t max_orders)
    : max_orders_(max_orders) {
  orders_.resize(max_orders);
}

OrderManagementSystem::OrderID
OrderManagementSystem::create_order(int64_t price, uint64_t qty,
                                    databento::Side side) {
  if (next_order_id_ == max_orders_) {
    throw std::runtime_error("OrderManagementSystem is full");
  }
  OMSOrder &new_order = orders_[next_order_id_];
  new_order.order_id = next_order_id_;
  new_order.price = price;
  new_order.qty = qty;
  new_order.filled_qty = 0;
  new_order.side = side;
  new_order.status = OMSOrderStatus::PENDING;
  // call network sim
  return next_order_id_++;
}

// to be called by strategy
void OrderManagementSystem::ack_order(OrderID order_id) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  orders_[order_id].status = OMSOrderStatus::LIVE;
}

// to be called by network simulator
void OrderManagementSystem::fill_order(OrderID order_id, uint64_t filled_qty) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }

  OMSOrder &order = orders_[order_id];
  uint64_t remaining_qty = order.qty - order.filled_qty;
  if (filled_qty > remaining_qty) {
    throw std::runtime_error("Fill quantity exceeds remaining order quantity");
  }

  order.filled_qty += filled_qty;
  if (order.filled_qty == order.qty) {
    order.status = OMSOrderStatus::FILLED;
  } else {
    order.status = OMSOrderStatus::PARTIALLY_FILLED;
  }

  if (order.side == databento::Side::Bid) {
    holdings_ += filled_qty;
    cash_ -= filled_qty * order.price;
  } else {
    holdings_ -= filled_qty;
    cash_ += filled_qty * order.price;
  }
  // Notify strategy
}

void OrderManagementSystem::cancel_order(OrderID order_id) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  OMSOrder &order = orders_[order_id];
  if (order.status == OMSOrderStatus::FILLED ||
      order.status == OMSOrderStatus::CANCELLED) {
    throw std::runtime_error("Order is already filled or cancelled");
  }
  order.status = OMSOrderStatus::CANCELLED;
  // notify strategy
}

const OrderManagementSystem::OMSOrder &
OrderManagementSystem::get_order(OrderID order_id) const {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  return orders_[order_id];
}
