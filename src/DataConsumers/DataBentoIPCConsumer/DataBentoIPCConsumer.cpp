#include "DataConsumers/DataBentoIPCConsumer/DataBentoIPCConsumer.h"

namespace backtesting_engine::mbo {
DataBentoIPCConsumer::DataBentoIPCConsumer(Interactor &reader)
    : reader_(reader), lob_() {}

bool DataBentoIPCConsumer::try_poll(databento::MboMsg &out_msg) {
  return reader_.pop(out_msg);
}
} // namespace backtesting_engine::mbo
