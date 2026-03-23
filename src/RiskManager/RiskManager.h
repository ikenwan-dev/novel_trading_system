#pragma once

#include "Events/Bar/BarEvent.h"
#include "Events/Bar/SignalEvent/SignalEvent.h"
#include "Portfolio/Portfolio.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <memory>

namespace backtesting_engine::bar {
class RiskManager {
public:
  RiskManager(Portfolio &portfolio,
              common::ThreadSafeQueue<std::shared_ptr<Event>>& queue)
      : portfolio_(portfolio), queue_(queue) {}

  void on_signal(const SignalEvent &event);

private:
  Portfolio &portfolio_;
  common::ThreadSafeQueue<std::shared_ptr<Event>> &queue_;
};
} // namespace backtesting_engine::bar
