#pragma once
#include "Events/OrderEvent/OrderEvent.h"

class ExecutionHandler {
public:
  virtual ~ExecutionHandler() = default;
  virtual void on_order(const OrderEvent &order_event) = 0;
};