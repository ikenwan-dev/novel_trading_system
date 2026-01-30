#include "DataConsumers/DataBentoConsumer/DataBentoConsumer.h"
#include "SharedMemory/IPCInteractor.h"

int main() {
  std::cout << "Waiting for Producer to create SHM..." << std::endl;
  IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE> reader(
      "test_shm", false);
  while (!reader.is_initialized()) {
  }
  std::cout << "ring buffer initialized" << std::endl;
  DataBentoConsumer consumer(reader);
  consumer.consume();
  reader.unlink();
  return 0;
}