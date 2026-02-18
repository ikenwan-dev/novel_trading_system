#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"

DataBentoConsumer::DataBentoConsumer(Interactor &reader)
    : reader_(reader), lob_() {}

void DataBentoConsumer::consume() {
  std::cout << "Consuming...\n";
  databento::MboMsg msg;
  databento::MboMsg end_msg{};
  long long num_messages = 0;
  while (true) {
    if (!reader_.pop(msg)) {
      continue;
    }
    lob_.update_book(msg);
    // todo send message to market maker strategy

    // update OMS

    if (msg == end_msg) {
      break;
    }
    num_messages++;
  }
}