#pragma once

#include "NetworkSimulator/NetworkSimulator.h"
#include <cstddef>
#include <cstdint>
#include <databento/enums.hpp>
#include <vector>

namespace backtesting_engine::mbo {
class OrderManagementSystem {
  using OrderID = uint64_t;

public:
  OrderManagementSystem(NetworkSimulator &simulator, std::size_t max_orders);

  enum class OMSOrderStatus {
    PENDING,
    LIVE,
    PARTIALLY_FILLED,
    FILLED,
    PENDING_CANCEL,
    CANCELLED,
    REJECTED,
    EXPIRED,
    UNKNOWN
  };
  struct OMSOrder {
    uint64_t order_id;
    int64_t price;
    uint64_t qty;
    uint64_t filled_qty;
    databento::Side side;
    OMSOrderStatus status;
  };
  OrderID create_order(uint64_t timestamp_ns, int64_t price, uint64_t qty,
                       databento::Side side);
  void ack_create_order(OrderID order_id);
  void ack_fill_order(OrderID order_id, uint64_t filled_qty, int64_t price);
  void cancel_order(uint64_t timestamp_ns, OrderID order_id);
  void ack_cancel_order(OrderID order_id);
  const OMSOrder &get_order(OrderID order_id) const;

private:
  std::size_t max_orders_;
  std::size_t next_order_id_ = 0;
  std::vector<OMSOrder> orders_;
  NetworkSimulator &simulator_;
  int64_t holdings_ = 0;
  int64_t cash_ = 0;
};
} // namespace backtesting_engine::mbo
