#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include <databento/record.hpp>

namespace backtesting_engine::mbo {
template <typename Derived> class MBOStrategyBase {
protected:
  OrderManagementSystem &oms_;
  // TODO: add risk engine reference
public:
  explicit MBOStrategyBase(OrderManagementSystem &oms) : oms_(oms) {}
  // called by event loop when the LOB updates
  inline void on_book_update(int64_t timestamp_ns, const DataBentoLOB &lob) {
    // Static cast delegates to the derived class at compile time.
    // The compiler sees right through this and inlines the call.
    static_cast<Derived *>(this)->impl_on_book_update(timestamp_ns, lob);
  }

  inline void on_fill(const databento::MboMsg &order) {
    static_cast<Derived *>(this)->impl_on_fill(order);
  }

  inline void send_order(uint64_t timestamp_ns, int64_t price, uint64_t qty,
                         databento::Side side) {
    // TODO: add risk engine check here
    oms_.create_order(timestamp_ns, price, qty, side);
  }

  inline void cancel_order(int64_t timestamp_ns, uint64_t order_id) {
    oms_.cancel_order(timestamp_ns, order_id);
  }
};

} // namespace backtesting_engine::mbo