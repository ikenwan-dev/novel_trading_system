#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"

DataBentoConsumer::DataBentoConsumer(Interactor &reader) : reader_(reader) {}

void DataBentoConsumer::consume() {
  std::cout << "Consuming...\n";
  databento::MboMsg msg;
  databento::MboMsg end_msg{};
  long long num_messages = 0;
  while (true) {
    if (!reader_.pop(msg)) {
      continue;
    }
    // 1. this is where we will implement the logic to update the limit order
    // book and strategy etc
    if (msg == end_msg) {
      break;
    }
    num_messages++;
  }
}