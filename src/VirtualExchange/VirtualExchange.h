#pragma once
#include "Events/Event.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include <databento/record.hpp>
#include <map>
#include <unordered_map>

namespace backtesting_engine {
class VirtualExchange {
public:
  VirtualExchange(NetworkSimulator &simulator, DataBentoLOB &lob)
      : simulator_(simulator), lob_(lob) {}
  void on_fill(const databento::MboMsg &msg);

  void on_update(const EventV2 &event);

private:
  struct VirtualOrderMetaData {
    databento::Side side;
    int64_t price;
  };
  struct VirtualOrder {
    uint64_t order_id;
    uint64_t qty_ahead;
    uint64_t remaining_qty;
    uint64_t original_qty;
  };
  std::unordered_map<uint64_t, VirtualOrderMetaData> order_metadata_;
  NetworkSimulator &simulator_;
  DataBentoLOB &lob_;
  using VirtualPriceLevel = std::vector<VirtualOrder>;
  using VirtualPriceLevels = std::map<int64_t, VirtualPriceLevel>;
  VirtualPriceLevels open_buy_orders_;  // key is price
  VirtualPriceLevels open_sell_orders_; // key is price

  VirtualPriceLevels &get_side(databento::Side side);

  VirtualPriceLevels::iterator get_price_level(VirtualPriceLevels &levels,
                                               int64_t price);

  VirtualPriceLevel::iterator get_order(VirtualPriceLevel &level,
                                        uint64_t order_id);
  std::unordered_map<uint64_t, VirtualOrderMetaData>::iterator
  get_order_metadata(uint64_t order_id);

  void on_create_order(uint64_t timestamp_ns, const CreateOrderEvent &event);
  void on_cancel_order(uint64_t timestamp_ns, const CancelOrderEvent &event);
};
} // namespace backtesting_engine