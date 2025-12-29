#include "DataHandlers/DataHandler.h"
#include "Events/OrderEvent/OrderEvent.h"
#include "ExecutionHandler.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"

class SimulatedExecutionHandler : public ExecutionHandler {
public:
  SimulatedExecutionHandler(
      ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
      DataHandler &data_handler)
      : event_queue_(event_queue), data_handler_(data_handler) {}
  void on_order(const OrderEvent &order_event) override;

private:
  ThreadSafeQueue<std::shared_ptr<Event>> &event_queue_;
  DataHandler &data_handler_;
};