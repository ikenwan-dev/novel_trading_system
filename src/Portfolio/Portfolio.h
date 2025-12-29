#pragma once

#include "DataHandlers/DataHandler.h"
#include "Events/FillEvent/FillEvent.h"
#include "Events/MarketEvent/MarketEvent.h"
#include <map>
#include <string>

class Portfolio {
public:
  Portfolio(DataHandler &data_handler, double initial_capital)
      : data_handler_(data_handler), initial_capital_(initial_capital),
        cash_(initial_capital), peak_equity_(initial_capital) {}

  // For updating portfolio holdngs, cash, etc
  void on_fill(const FillEvent &event);

  // For updating unrealized P/L
  void on_market_data(const MarketEvent &event);

  double get_total_value() const;

  double get_unrealized_pnl() const;

  double get_cash() const { return cash_; }

  double get_current_drawdown() const {
    if (peak_equity_ <= 0.0) {
      return 0.0;
    }
    return (peak_equity_ - get_total_value()) / peak_equity_;
  }

  double get_latest_price(const std::string &ticker) const {
    return data_handler_.get_latest_price(ticker);
  }

private:
  DataHandler &data_handler_;
  double initial_capital_;
  double cash_;
  double peak_equity_;

  // maps from ticker to share quantities
  std::map<std::string, int> holdings_;

  struct Position {
    double market_value = 0.0;
    double cost_basis = 0.0;
  };
  std::map<std::string, Position> positions_;
};
