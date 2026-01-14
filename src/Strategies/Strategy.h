#pragma once
#include "Events/MarketEvent/MarketEvent.h"

class Strategy {
public:
  virtual ~Strategy() = default;

  // Pure virtual function to process market data
  // To be called by main event loop and will possibly produce signal events
  virtual void on_market_data(const MarketEvent &event) = 0;
};