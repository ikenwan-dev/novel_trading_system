#pragma once
#include "Events/FillEvent/FillEvent.h"
#include <algorithm>


namespace backtesting_engine {
class TransactionCostModel {
public:
  virtual ~TransactionCostModel() = default;

  virtual double calculate_cost(const FillEvent &event) const = 0;
};

class FixedCommissionModel : public TransactionCostModel {
public:
  FixedCommissionModel(double commission_per_trade)
      : commision_per_trade_(commission_per_trade) {}

  double calculate_cost(const FillEvent &event) const override {
    return commision_per_trade_;
  }

private:
  const double commision_per_trade_;
};

class PershareCommissionModel : public TransactionCostModel {
public:
  PershareCommissionModel(double commission_per_share, double min_commission)
      : commission_per_share_(commission_per_share),
        min_commission_(min_commission) {}

  double calculate_cost(const FillEvent &event) const override {
    return std::max(event.quantity_ * commission_per_share_, min_commission_);
  }

private:
  const double commission_per_share_;
  const double min_commission_;
};
} // namespace backtesting_engine
