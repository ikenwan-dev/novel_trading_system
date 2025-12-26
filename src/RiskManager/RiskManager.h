#pragma once

#include "Events/Event.h"
#include "Events/SignalEvent/SignalEvent.h"
#include "Portfolio/Portfolio.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <memory>
class RiskManager {
public:
  RiskManager(Portfolio &portfolio,
              ThreadSafeQueue<std::shared_ptr<Event>>& queue)
      : portfolio_(portfolio), queue_(queue) {}

  void on_signal(const SignalEvent &event);

private:
  Portfolio &portfolio_;
  ThreadSafeQueue<std::shared_ptr<Event>> &queue_;
};