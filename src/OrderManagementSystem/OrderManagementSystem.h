#pragma once

#include <cstddef>
#include <cstdint>
#include <databento/enums.hpp>
#include <vector>

namespace backtesting_engine {
class OrderManagementSystem {
  using OrderID = uint64_t;

public:
  OrderManagementSystem(std::size_t max_orders);

  enum class OMSOrderStatus {
    PENDING,
    LIVE,
    PARTIALLY_FILLED,
    FILLED,
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
  OrderID create_order(int64_t price, uint64_t qty, databento::Side side);
  void ack_order(OrderID order_id);
  void fill_order(OrderID order_id, uint64_t filled_qty);
  void cancel_order(OrderID order_id);
  const OMSOrder &get_order(OrderID order_id) const;

private:
  std::size_t max_orders_ = 100000;
  std::size_t next_order_id_ = 0;
  std::vector<OMSOrder> orders_;
  int64_t holdings_ = 0;
  int64_t cash_ = 0;

  // TODO: add reference to network simulator once implemented
};
} // namespace backtesting_engine
