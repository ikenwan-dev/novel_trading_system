#pragma once
#include "Events/Bar/OrderEvent/OrderEvent.h"


namespace backtesting_engine::bar {
class ExecutionHandler {
public:
  virtual ~ExecutionHandler() = default;
  virtual void on_order(const OrderEvent &order_event) = 0;
};
} // namespace backtesting_engine::bar
