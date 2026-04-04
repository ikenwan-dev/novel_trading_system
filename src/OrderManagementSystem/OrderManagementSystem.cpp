#include "OrderManagementSystem/OrderManagementSystem.h"
#include "Events/Mbo/MboEvent.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>

namespace backtesting_engine::mbo {
OrderManagementSystem::OrderManagementSystem(NetworkSimulator &simulator,
                                             MBORiskManager &risk_manager,
                                             std::size_t max_orders)
    : simulator_(simulator), risk_manager_(risk_manager),
      max_orders_(max_orders) {
  // Initialize orders vector
  orders_.resize(max_orders);
}

std::pair<OrderManagementSystem::OrderID, RiskResult>
OrderManagementSystem::create_order(uint64_t timestamp_ns, int64_t price,
                                    uint64_t qty, databento::Side side) {
  if (next_order_id_ == max_orders_) {
    throw std::runtime_error("OrderManagementSystem is full");
  }

  RiskResult risk_status = risk_manager_.check_order(side, qty, price);
  if (risk_status != RiskResult::APPROVED) {
    uint64_t rejected_id = next_order_id_++;
    OMSOrder &new_order = orders_[rejected_id];
    new_order.order_id = rejected_id;
    new_order.price = price;
    new_order.qty = qty;
    new_order.filled_qty = 0;
    new_order.side = side;
    new_order.status = OMSOrderStatus::REJECTED;

    return {rejected_id, risk_status};
  }

  uint64_t new_id = next_order_id_++;
  OMSOrder &new_order = orders_[new_id];
  new_order.order_id = new_id;
  new_order.price = price;
  new_order.qty = qty;
  new_order.filled_qty = 0;
  new_order.side = side;
  new_order.status = OMSOrderStatus::PENDING;
  // call network sim
  simulator_.send_outbound_event(
      {timestamp_ns, CreateOrderEvent{new_order.order_id, new_order.price,
                                      new_order.qty, new_order.side}});
  return {new_id, RiskResult::APPROVED};
}

// to be called by network simulator/ vx
void OrderManagementSystem::ack_create_order(OrderID order_id) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  orders_[order_id].status = OMSOrderStatus::LIVE;
}

// to be called by network simulator/ vx
void OrderManagementSystem::ack_fill_order(OrderID order_id,
                                           uint64_t filled_qty, int64_t price) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }

  OMSOrder &order = orders_[order_id];
  if (order.status != OMSOrderStatus::LIVE &&
      order.status != OMSOrderStatus::PARTIALLY_FILLED &&
      order.status != OMSOrderStatus::PENDING_CANCEL) {
    throw std::runtime_error("Order is not live or partially filled");
  }

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
    cash_ -= filled_qty * price;
  } else {
    holdings_ -= filled_qty;
    cash_ += filled_qty * price;
  }

  risk_manager_.on_fill(order.side, filled_qty, price);
}

void OrderManagementSystem::cancel_order(uint64_t timestamp_ns,
                                         OrderID order_id) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  OMSOrder &order = orders_[order_id];
  if (order.status == OMSOrderStatus::FILLED ||
      order.status == OMSOrderStatus::PENDING_CANCEL ||
      order.status == OMSOrderStatus::CANCELLED) {
    // Ignore: Race condition where we try to cancel an order that was just
    // filled or already cancelled while the command was in flight.
    return;
  }
  order.status = OMSOrderStatus::PENDING_CANCEL;
  simulator_.send_outbound_event(
      {timestamp_ns, CancelOrderEvent{order.order_id}});
}

void OrderManagementSystem::ack_cancel_order(OrderID order_id) {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  if (orders_[order_id].status != OMSOrderStatus::FILLED) {
    orders_[order_id].status = OMSOrderStatus::CANCELLED;
  }
}

const OrderManagementSystem::OMSOrder &
OrderManagementSystem::get_order(OrderID order_id) const {
  if (order_id >= next_order_id_) {
    throw std::runtime_error("Invalid order id");
  }
  return orders_[order_id];
}

int64_t OrderManagementSystem::get_mtm_equity(int64_t current_mid_price) const {
  return cash_ + (holdings_ * current_mid_price);
}

void OrderManagementSystem::print_summary(int64_t current_mid_price) const {
  std::cout << "\n============================================\n";
  std::cout << "         OMS TRADING SUMMARY                \n";
  std::cout << "============================================\n";

  std::cout << "[Portfolio]\n";
  std::cout << "  Holdings:   " << holdings_ << "\n";
  std::cout << "  Cash:       $" << std::fixed << std::setprecision(2)
            << static_cast<double>(cash_) / 1e9 << "\n";

  if (current_mid_price != 0) {
    double mtm_equity = static_cast<double>(get_mtm_equity(current_mid_price)) / 1e9;
    std::cout << "  MTM Equity: $" << std::fixed << std::setprecision(2)
              << mtm_equity << " (based on mid: $" 
              << static_cast<double>(current_mid_price) / 1e9 << ")\n";
  } else {
    std::cout << "  MTM Equity: N/A (no valid mid price recorded)\n";
  }

  std::cout << "\n[Order Status Counts]\n";

  std::map<OMSOrderStatus, std::size_t> counts;
  for (std::size_t i = 0; i < next_order_id_; ++i) {
    counts[orders_[i].status]++;
  }

  auto to_string = [](OMSOrderStatus status) {
    switch (status) {
    case OMSOrderStatus::UNKNOWN:
      return "UNKNOWN";
    case OMSOrderStatus::PENDING:
      return "PENDING";
    case OMSOrderStatus::LIVE:
      return "LIVE";
    case OMSOrderStatus::PARTIALLY_FILLED:
      return "PARTIALLY_FILLED";
    case OMSOrderStatus::FILLED:
      return "FILLED";
    case OMSOrderStatus::PENDING_CANCEL:
      return "PENDING_CANCEL";
    case OMSOrderStatus::CANCELLED:
      return "CANCELLED";
    case OMSOrderStatus::REJECTED:
      return "REJECTED";
    case OMSOrderStatus::EXPIRED:
      return "EXPIRED";
    default:
      return "INVALID";
    }
  };

  for (const auto &[status, count] : counts) {
    std::cout << "  " << std::left << std::setw(17) << to_string(status) 
              << ": " << count << "\n";
  }
  std::cout << "============================================\n";
}

} // namespace backtesting_engine::mbo
