#pragma once
#include <cstdint>
#include <databento/enums.hpp>
#include <databento/historical.hpp>
#include <databento/record.hpp>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace backtesting_engine {
class DataBentoLOB {
public:
  void update_book(const databento::MboMsg &msg);
  std::pair<int64_t, int64_t> get_bbo() const;
  uint32_t get_level_qty(databento::Side side, int64_t price) const;
  // add some functions for retrieiving bbo, price, levels, possibly orders
  // ahead  etc

private:
  struct Order {
    int64_t price;
    databento::Side side;
  };
  struct PriceLevel {
    uint32_t total_qty = 0;
    std::vector<databento::MboMsg> messages;
  };
  using Orders = std::unordered_map<uint64_t, Order>; // map of order_id to
                                                      // basic order info
  using PriceLevels =
      std::map<int64_t, PriceLevel>; // maps price to list of order messages
  Orders orders_;
  PriceLevels bids_;
  PriceLevels asks_;

  void add_order(const databento::MboMsg &msg);
  void cancel_order(const databento::MboMsg &msg);
  void modify_order(const databento::MboMsg &msg);
  void clear_book();
  PriceLevels &get_side(databento::Side side);
  const PriceLevels &get_side(databento::Side side) const;
  PriceLevels::iterator get_price_level(PriceLevels &price_levels,
                                        int64_t price);
  Order &get_order(uint64_t order_id);
  std::vector<databento::MboMsg>::iterator get_order_message(uint64_t order_id,
                                                             PriceLevel &level);
};
} // namespace backtesting_engine
