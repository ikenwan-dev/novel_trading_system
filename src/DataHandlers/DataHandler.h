#pragma once
#include "DataTypes/DataTypes.h"
#include <optional>
#include <string>

class DataHandler {
public:
  virtual ~DataHandler() = default;

  // Will be called by the event loop and will produce and push MarketEvents
  // onto the queue
  virtual void update() = 0;

  // A way to check if the DataHandler is still running
  virtual bool is_running() const = 0;

  // A way to get the latest price for a ticker
  virtual std::optional<Bar>
  get_latest_price_info(const std::string &ticker) const = 0;
};