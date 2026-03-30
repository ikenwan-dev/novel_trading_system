#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"


namespace backtesting_engine::mbo {
DataBentoConsumer::DataBentoConsumer(Interactor &reader)
    : reader_(reader), lob_() {}

bool DataBentoConsumer::try_poll(databento::MboMsg &out_msg) {
  return reader_.pop(out_msg);
}
} // namespace backtesting_engine::mbo
