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
  template <typename LevelIterator>
  void fill_orders_at_price_level(LevelIterator &levels_it,
                                  uint64_t &fill_qty_left,
                                  uint64_t timestamp_ns,
                                  int64_t actual_fill_price);

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
  // using VirtualPriceLevel = std::vector<VirtualOrder>;
  struct VirtualPriceLevel {
    std::vector<VirtualOrder> orders;
    uint64_t total_qty_ahead = 0;
  };
  using VirtualPriceLevels = std::map<int64_t, VirtualPriceLevel>;
  VirtualPriceLevels open_buy_orders_;  // key is price
  VirtualPriceLevels open_sell_orders_; // key is price

  VirtualPriceLevels &get_side(databento::Side side);

  VirtualPriceLevels::iterator get_price_level(VirtualPriceLevels &levels,
                                               int64_t price);

  std::vector<VirtualOrder>::iterator get_order(VirtualPriceLevel &level,
                                                uint64_t order_id);
  std::unordered_map<uint64_t, VirtualOrderMetaData>::iterator
  get_order_metadata(uint64_t order_id);

  void on_create_order(uint64_t timestamp_ns, const CreateOrderEvent &event);
  void on_cancel_order(uint64_t timestamp_ns, const CancelOrderEvent &event);
};

template <typename LevelIterator>
void VirtualExchange::fill_orders_at_price_level(LevelIterator &levels_it,
                                                 uint64_t &fill_qty_left,
                                                 uint64_t timestamp_ns,
                                                 int64_t actual_fill_price) {
  auto level_it = levels_it->second.orders.begin();
  while (level_it != levels_it->second.orders.end() && fill_qty_left) {
    bool left_over = level_it->qty_ahead < fill_qty_left;
    if (left_over) {
      fill_qty_left -= level_it->qty_ahead;
      levels_it->second.total_qty_ahead -= level_it->qty_ahead;
      level_it->qty_ahead = 0;
    } else {
      level_it->qty_ahead -= fill_qty_left;
      levels_it->second.total_qty_ahead -= fill_qty_left;
      fill_qty_left = 0;
    }

    uint64_t filled_qty = 0;
    if (level_it->qty_ahead == 0 && fill_qty_left) {
      if (level_it->remaining_qty >= fill_qty_left) {
        filled_qty = fill_qty_left;
        level_it->remaining_qty -= fill_qty_left;
        fill_qty_left = 0;
      } else {
        filled_qty = level_it->remaining_qty;
        fill_qty_left -= level_it->remaining_qty;
        level_it->remaining_qty = 0;
      }
    }
    if (filled_qty) {
      simulator_.send_inbound_event(
          {timestamp_ns, AckFillOrderEvent{level_it->order_id, filled_qty, actual_fill_price}});
    }

    // --- ADD ERASURE LOGIC ---
    if (level_it->remaining_qty == 0) {
      order_metadata_.erase(level_it->order_id);
      level_it = levels_it->second.orders.erase(level_it);
      // TODO: test this to see performance benefits
      //  if (levels_it->second.orders.empty()) {
      //    // (Optional logic if you want to cleanly remove the price level
      //    // when empty,
      //    //  though technically keeping an empty level around isn't hurting
      //    //  much)
      //  }
    } else {
      level_it++; // Only increment if we didn't erase!
    }
  }
}

} // namespace backtesting_engine