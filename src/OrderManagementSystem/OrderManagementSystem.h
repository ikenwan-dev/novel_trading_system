#pragma once

#include "NetworkSimulator/NetworkSimulator.h"
#include "RiskManager/Mbo/MBORiskManager.h"
#include <cstddef>
#include <cstdint>
#include <databento/enums.hpp>
#include <utility>
#include <vector>

namespace backtesting_engine::mbo {
class OrderManagementSystem {
  using OrderID = uint64_t;

public:
  OrderManagementSystem(NetworkSimulator &simulator,
                        MBORiskManager &risk_manager, std::size_t max_orders);

  enum class OMSOrderStatus : uint8_t {
    UNKNOWN = 0,
    PENDING,
    LIVE,
    PARTIALLY_FILLED,
    FILLED,
    PENDING_CANCEL,
    CANCELLED,
    REJECTED,
    EXPIRED
  };
  struct OMSOrder {
    uint64_t order_id{0};
    int64_t price{0};
    uint64_t qty{0};
    uint64_t filled_qty{0};
    databento::Side side{databento::Side::None};
    OMSOrderStatus status{OMSOrderStatus::UNKNOWN};
  };
  std::pair<OrderID, RiskResult> create_order(uint64_t timestamp_ns,
                                              int64_t price, uint64_t qty,
                                              databento::Side side);
  void ack_create_order(OrderID order_id);
  void ack_fill_order(OrderID order_id, uint64_t filled_qty, int64_t price);
  void cancel_order(uint64_t timestamp_ns, OrderID order_id);
  void ack_cancel_order(OrderID order_id);
  const OMSOrder &get_order(OrderID order_id) const;
  int64_t get_holdings() const { return holdings_; }
  int64_t get_cash() const { return cash_; }
  int64_t get_mtm_equity(int64_t current_mid_price) const;
  void print_order_status_counts() const;

private:
  std::size_t max_orders_;
  std::size_t next_order_id_ = 0;
  std::vector<OMSOrder> orders_;
  NetworkSimulator &simulator_;
  MBORiskManager &risk_manager_;
  int64_t holdings_ = 0;
  int64_t cash_ = 0;
};
} // namespace backtesting_engine::mbo
