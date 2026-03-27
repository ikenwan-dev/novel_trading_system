#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "RiskManager/Mbo/RiskResult.h"
#include <databento/record.hpp>

namespace backtesting_engine::mbo {
template <typename Derived> class MBOStrategyBase {
protected:
  OrderManagementSystem &oms_;
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

  inline void on_order_rejected(uint64_t order_id, RiskResult reason) {
    static_cast<Derived *>(this)->impl_on_order_rejected(order_id, reason);
  }

  inline void send_order(uint64_t timestamp_ns, int64_t price, uint64_t qty,
                         databento::Side side) {
    oms_.create_order(timestamp_ns, price, qty, side);
  }

  inline void cancel_order(int64_t timestamp_ns, uint64_t order_id) {
    oms_.cancel_order(timestamp_ns, order_id);
  }
};

} // namespace backtesting_engine::mbo