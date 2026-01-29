#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"
#include "SharedMemory/IPCInteractor.h"

int main() {
  IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE> reader(
      "test_shm", false);
  DataBentoConsumer consumer(reader);
  consumer.consume();
  reader.unlink();
  return 0;
}