#include "Constants/Constants.h"
#include "Events/Event.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "SharedMemory/IPCInteractor.h"
#include <databento/record.hpp>

using namespace backtesting_engine;

using Interactor =
    IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

databento::MboMsg get_next_msg(Interactor &reader, databento::MboMsg &msg) {
  while (!reader.pop(msg)) {
  }
  return msg;
}

int main() {
  std::cout << "Waiting for Producer to create SHM..." << std::endl;
  Interactor reader("test_shm", false);
  while (!reader.is_initialized()) { // spin while producer creates shm
  }
  std::cout << "ring buffer initialized" << std::endl;

  databento::MboMsg msg;
  databento::MboMsg end_msg{};
  get_next_msg(reader, msg);
  bool has_more_messages = msg != end_msg;

  DataBentoLOB lob_;
  EventQueue queue;
  NetworkSimulator simulator(queue, Constants::OUTBOUND_LATENCY,
                             Constants::INBOUND_LATENCY);
  long long num_messages = 0;
  std::cout << "Consuming..." << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  while (has_more_messages || queue.size() > 0) {
  }
  // while (true) {
  //   get_next_msg(reader, msg);

  //   if (msg == end_msg) {
  //     break;
  //   }
  //   lob_.update_book(msg);
  //   num_messages++;
  // }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Consumed " << num_messages << " messages" << std::endl;
  std::chrono::duration<double> diff = end - start;
  double seconds = diff.count();
  auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  // 4. Output results
  std::cout << "Processing time: " << seconds << " s" << std::endl;
  std::cout << "Total messages: " << num_messages << std::endl;

  if (num_messages > 0) {
    std::cout << "Average latency: " << (nanoseconds / num_messages) << " ns"
              << std::endl;
    std::cout << "Throughput: " << (uint64_t)(num_messages / seconds)
              << " msg/s" << std::endl;
  }
  reader.unlink();
  return 0;
}