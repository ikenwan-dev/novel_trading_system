#include "Constants/Constants.h"
#include "Events/Mbo/MboEvent.h"
#include "LimitOrderBook/DataBentoLOB/DataBentoLOB.h"
#include "NetworkSimulator/NetworkSimulator.h"
#include "SharedMemory/IPCInteractor.h"
#include "databento/enums.hpp"
#include <algorithm>
#include <databento/record.hpp>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

using Interactor =
    common::IPCInteractor<databento::MboMsg, Constants::RING_BUFFER_SIZE>;

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
  // get_next_msg(reader, msg);
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

  std::deque<uint64_t> last_15_orders;
  std::unordered_map<uint64_t, std::vector<databento::MboMsg>> tracked_messages;

  std::cout << "Consuming...." << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    get_next_msg(reader, msg);

    if (msg == end_msg) {
      break;
    }
    action_counts[msg.action]++;

    if (msg.action == databento::Action::Add) {
      if (tracked_messages.find(msg.order_id) == tracked_messages.end()) {
        last_15_orders.push_back(msg.order_id);
        tracked_messages[msg.order_id] = std::vector<databento::MboMsg>(); // Initialize empty vector
        
        if (last_15_orders.size() > 15) {
          uint64_t oldest = last_15_orders.front();
          last_15_orders.pop_front();
          tracked_messages.erase(oldest);
        }
      }
    }

    if (tracked_messages.find(msg.order_id) != tracked_messages.end()) {
      tracked_messages[msg.order_id].push_back(msg);
    }

    lob_.update_book(msg);
    num_messages++;
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto max_it = std::max_element(
      message_counts.begin(), message_counts.end(),
      [](const auto &a, const auto &b) { return a.second < b.second; });

  for (const auto &action : action_counts) {
    std::cout << (char)action.first << " : " << action.second << std::endl;
  }

  std::cout << "\n--- Last 15 Created Orders and their Messages ---"
            << std::endl;
  for (uint64_t order_id : last_15_orders) {
    std::cout << "Order ID: " << order_id << std::endl;
    for (const auto &m : tracked_messages[order_id]) {
      std::cout << "  " << m << std::endl;
    }
  }
  std::cout << "-----------------------------------------------" << std::endl;
  ////// test code end

  std::cout << "lob bbo: " << lob_.get_bbo().first << " @ "
            << lob_.get_bbo().second << std::endl;
  std::cout << "Total Volume in LOB: " << lob_.get_total_volume() << std::endl;
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