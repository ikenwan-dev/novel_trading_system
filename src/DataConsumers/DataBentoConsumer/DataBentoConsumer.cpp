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

    if (num_messages % 1000000 == 0) {
      std::cout << "Num messages: " << num_messages << '\n';
      std::cout << msg << '\n' << '\n';
    }
    if (msg == end_msg) {
      break;
    }
    num_messages++;
  }
}