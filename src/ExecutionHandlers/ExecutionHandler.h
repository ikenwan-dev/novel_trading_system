#pragma once
#include "Events/OrderEvent/OrderEvent.h"


namespace backtesting_engine {
class ExecutionHandler {
public:
  virtual ~ExecutionHandler() = default;
  virtual void on_order(const OrderEvent &order_event) = 0;
};
} // namespace backtesting_engine
