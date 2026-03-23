#include "DataHandlers/DataHandler.h"
#include "Events/OrderEvent/OrderEvent.h"
#include "ExecutionHandlers/ExecutionHandler.h"
#include "ExecutionHandlers/TransactionCostModel.h"
#include "ThreadSafeQueue/ThreadSafeQueue.h"
#include <memory>

// for bar data engine
namespace backtesting_engine {
class SimulatedExecutionHandler : public ExecutionHandler {
public:
  SimulatedExecutionHandler(
      ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
      DataHandler &data_handler,
      std::unique_ptr<TransactionCostModel> transaction_cost_model)
      : event_queue_(event_queue), data_handler_(data_handler),
        transaction_cost_model_(std::move(transaction_cost_model)) {}

  // TODO: this doesn't consider bid ask spread
  void on_order(const OrderEvent &order_event) override;

private:
  std::unique_ptr<TransactionCostModel> transaction_cost_model_;
  ThreadSafeQueue<std::shared_ptr<Event>> &event_queue_;
  DataHandler &data_handler_;
};
} // namespace backtesting_engine
