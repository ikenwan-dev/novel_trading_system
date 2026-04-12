#include "LimitOrderBook/LimitOrderBookConcept.h"
#include "OrderManagementSystem/OrderManagementSystem.h"
#include "Performance/LatencyProfiler.h"
#include "Performance/TSC_Clock.h"
#include "RiskManager/Mbo/RiskResult.h"
#include <databento/record.hpp>

namespace backtesting_engine::mbo {
template <typename Derived> class MBOStrategyBase {
protected:
  OrderManagementSystem &oms_;
  performance::LatencyProfiler *profiler_;
  uint64_t current_start_tsc_{0};

public:
  explicit MBOStrategyBase(OrderManagementSystem &oms,
                           performance::LatencyProfiler *profiler = nullptr)
      : oms_(oms), profiler_(profiler) {}

  // called by event loop when the LOB updates
  template <LimitOrderBookConcept LOB>
  inline void on_book_update(int64_t timestamp_ns, uint64_t start_tsc,
                             const LOB &lob) {
    current_start_tsc_ = start_tsc;
    static_cast<Derived *>(this)->impl_on_book_update(timestamp_ns, lob);
  }

  inline void on_fill(const databento::MboMsg &order) {
    static_cast<Derived *>(this)->impl_on_fill(order);
  }

  inline void on_order_accepted(uint64_t order_id) {
    static_cast<Derived *>(this)->impl_on_order_accepted(order_id);
  }

  inline void on_order_canceled(uint64_t order_id) {
    static_cast<Derived *>(this)->impl_on_order_canceled(order_id);
  }

  inline void on_order_filled(uint64_t order_id, uint64_t filled_qty,
                              int64_t price) {
    static_cast<Derived *>(this)->impl_on_order_filled(order_id, filled_qty,
                                                       price);
  }

  inline std::pair<uint64_t, RiskResult> send_order(uint64_t timestamp_ns,
                                                    int64_t price, uint64_t qty,
                                                    databento::Side side) {
    auto res = oms_.create_order(timestamp_ns, price, qty, side);
    if (profiler_) {
      uint64_t end_tsc = performance::get_tsc();
      profiler_->record_latency(performance::Metric::TICK_TO_TRADE,
                                end_tsc - current_start_tsc_);
    }
    return res;
  }

  inline void cancel_order(int64_t timestamp_ns, uint64_t order_id) {
    oms_.cancel_order(timestamp_ns, order_id);
    if (profiler_) {
      uint64_t end_tsc = performance::get_tsc();
      profiler_->record_latency(performance::Metric::TICK_TO_TRADE,
                                end_tsc - current_start_tsc_);
    }
  }
};

} // namespace backtesting_engine::mbo