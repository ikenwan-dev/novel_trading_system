#include "DataConsumers/DataBentoIPCConsumer/DataBentoIPCConsumer.h"
#include "../MBODataConsumerConcept.h"

namespace backtesting_engine::mbo {
DataBentoIPCConsumer::DataBentoIPCConsumer(Interactor &reader)
    : reader_(reader), lob_() {}

bool DataBentoIPCConsumer::try_poll(databento::MboMsg &out_msg) {
  return reader_.pop(out_msg);
}
static_assert(DataConsumerConcept<DataBentoIPCConsumer>, "DataBentoIPCConsumer fails to implement DataConsumerConcept!");
} // namespace backtesting_engine::mbo
