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
    // proccess events in NetWorkSimulator that occured before current msg
    // timestamp ie fills to the OMS, OMS Order creation to exchange sim,
    // exchange sim Order ack/fill/cancel back to OMS

    // run exchange simulator matching engine and push fills to network
    // simulator

    lob_.update_book(msg);

    // run strategy and push order creations to netowrk simulator

    if (msg == end_msg) {
      break;
    }
    num_messages++;
  }
}