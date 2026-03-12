#pragma once
#include "Events/Event.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include <databento/record.hpp>
#include <map>

namespace backtesting_engine {
class VirtualExchange {
public:
  VirtualExchange(NetworkSimulator &simulator, DataBentoLOB &lob)
      : simulator(simulator), lob_(lob) {}

  void on_create_order(const CreateOrderEvent &event);

  void on_cancel_order(const CancelOrderEvent &event);

  void on_fill(const databento::MboMsg &msg);

private:
  struct VirtualOrder {
    uint64_t order_id;
    uint64_t remaining_qty;
    uint64_t original_qty;
  };
  NetworkSimulator &simulator;
  DataBentoLOB &lob_;
  std::map<int64_t, std::vector<VirtualOrder>> open_buy_orders_; // key is price
  std::map<int64_t, std::vector<VirtualOrder>>
      open_sell_orders_; // key is price
};
} // namespace backtesting_engine