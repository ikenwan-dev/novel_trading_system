#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include <databento/record.hpp>

namespace backtesting_engine::mbo {
template <typename Derived> class MBOStrategyBase {
public:
  // The hot path: called by your event loop when the LOB updates
  inline void on_book_update(int64_t timestamp_ns, const DataBentoLOB &lob) {
    // Static cast delegates to the derived class at compile time.
    // The compiler sees right through this and inlines the call.
    static_cast<Derived *>(this)->impl_on_book_update(timestamp_ns, lob);
  }

  inline void on_fill(const databento::MboMsg &order) {
    static_cast<Derived *>(this)->impl_on_fill(order);
  }

  // Common utility functions shared across all strategies
  // (e.g., interacting with the OMS or Risk Engine)
  inline void send_order(uint64_t timestamp_ns, int64_t price, uint64_t qty,
                         databento::Side side) {
    // Send to OMS / Network Simulator
  }

  inline void cancel_order(int64_t timestamp_ns, uint64_t order_id) {
    // Send cancel request
  }
};

} // namespace backtesting_engine::mbo