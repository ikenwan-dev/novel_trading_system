#include "Constants/Constants.h"
#include "Events/Event.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "SharedMemory/IPCInteractor.h"
#include "databento/enums.hpp"
#include <algorithm>
#include <databento/record.hpp>
#include <map>

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
  ////// test code start
  std::vector<databento::MboMsg> trade_messages;
  std::vector<databento::MboMsg> fill_messages;
  std::map<uint64_t, long long> message_counts;
  std::unordered_map<databento::Action, long long> action_counts;
  databento::MboMsg last_msg;
  long long num_trades = 0;
  long long num_fills = 0;
  long long out_of_order_trades = 0;
  std::cout << "Consuming...." << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    get_next_msg(reader, msg);

    if (msg == end_msg) {
      break;
    }
    action_counts[msg.action]++;
    // if (msg.order_id) {
    //   message_counts[msg.order_id]++;
    // }
    // if (msg.action == databento::action::Fill && num_fills < 10) {
    //   fill_messages.push_back(msg);
    //   num_fills++;
    // } else if (msg.action == databento::action::Trade && num_trades < 10) {
    //   trade_messages.push_back(msg);
    //   num_trades++;
    // }
    lob_.update_book(msg);
    num_messages++;
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto max_it = std::max_element(
      message_counts.begin(), message_counts.end(),
      [](const auto &a, const auto &b) { return a.second < b.second; });

  for (const auto &action : action_counts) {
    std::cout << action.first << " : " << action.second << std::endl;
  }
  // std::cout << "order with most messages: " << max_it->first << " with "
  //           << max_it->second << " messages" << std::endl;
  // for (const auto &trade : trade_messages) {
  //   std::cout << trade << std::endl;
  // }
  // for (const auto &fill : fill_messages) {
  //   std::cout << fill << std::endl;
  // }
  ////// test code end

  std::cout << "lob bbo: " << lob_.get_bbo().first << " @ "
            << lob_.get_bbo().second << std::endl;
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