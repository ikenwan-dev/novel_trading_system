#include "HistoricCSVDataHandler.h"
#include "Events/MarketEvent/MarketEvent.h"
#include "csv.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

std::chrono::system_clock::time_point
HistoricCSVDataHandler::parse_stooq_datetime(const std::string &dateStr,
                                             const std::string &timeStr) {
  using namespace std::chrono;

  if (dateStr.size() != 8 || timeStr.size() != 6) {
    throw std::runtime_error("Bad date/time format: " + dateStr + " " +
                             timeStr);
  }

  int y = std::stoi(dateStr.substr(0, 4));
  unsigned m = static_cast<unsigned>(std::stoi(dateStr.substr(4, 2)));
  unsigned d = static_cast<unsigned>(std::stoi(dateStr.substr(6, 2)));

  int hour = std::stoi(timeStr.substr(0, 2));
  int minute = std::stoi(timeStr.substr(2, 2));
  int second = std::stoi(timeStr.substr(4, 2));

  // Build a calendar date
  year_month_day ymd{year{y} / month{m} / day{d}};
  if (!ymd.ok()) {
    throw std::runtime_error("Invalid date in data: " + dateStr);
  }

  // Convert to days since epoch
  sys_days sd{ymd}; // time_point<system_clock, days>

  // Add intraday time
  auto tp = sd + hours{hour} + minutes{minute} + seconds{second};

  // Convert to system_clock::time_point (duration conversion is automatic)
  return system_clock::time_point{tp};
}

std::vector<Bar>
HistoricCSVDataHandler::load_stooq_file(const std::string &filename) {
  std::vector<Bar> bars;

  // The file extension (.txt vs .csv) does not matter — it's just text.
  // Make sure the header line matches these names (case-sensitive):
  // Ticker,Per,Date,Time,Open,High,Low,Close,Vol,Openint
  io::CSVReader<10, io::trim_chars<' '>, io::no_quote_escape<','>> in(filename);

  in.read_header(io::ignore_extra_column, "<TICKER>", "<PER>", "<DATE>",
                 "<TIME>", "<OPEN>", "<HIGH>", "<LOW>", "<CLOSE>", "<VOL>",
                 "<OPENINT>");

  std::string ticker;
  std::string perStr;
  std::string dateStr;
  std::string timeStr;
  double open, high, low, close;
  double vol;
  double openInt;

  while (in.read_row(ticker, perStr, dateStr, timeStr, open, high, low, close,
                     vol, openInt)) {

    if (perStr.empty()) {
      std::cout << "Skipping row with empty PER\n";
      continue; // malformed row
    }

    Bar bar;
    bar.symbol = ticker;
    bar.timestamp =
        HistoricCSVDataHandler::parse_stooq_datetime(dateStr, timeStr);
    bar.open = open;
    bar.high = high;
    bar.low = low;
    bar.close = close;
    bar.volume = vol;

    bars.push_back(std::move(bar));
  }

  return bars;
}

HistoricCSVDataHandler::HistoricCSVDataHandler(
    ThreadSafeQueue<std::shared_ptr<Event>> &event_queue,
    const std::map<std::string, std::string> &csv_files)
    : event_queue_(event_queue) {
  load_all_data(csv_files);
}

void HistoricCSVDataHandler::load_all_data(
    const std::map<std::string, std::string> &csv_files) {
  // TODO: Add parameter detailing which load function to call, if we ever want
  // to add more data sources
  for (const auto &entry : csv_files) {
    all_data_[entry.first] = load_stooq_file(entry.second);
    current_index_[entry.first] = 0;
    std::sort(all_data_[entry.first].begin(), all_data_[entry.first].end());
  }
}

void HistoricCSVDataHandler::update() {
  std::string next_ticker = "";
  auto earliest_time = std::chrono::system_clock::time_point::max();
  for (const auto &[ticker, index] : current_index_) {
    if (index < all_data_[ticker].size()) {
      if (all_data_[ticker][index].timestamp < earliest_time) {
        earliest_time = all_data_[ticker][index].timestamp;
        next_ticker = ticker;
      }
    }
  }
  if (!next_ticker.empty()) {
    const auto &bar = all_data_[next_ticker][current_index_[next_ticker]];
    auto market_event =
        std::make_shared<MarketEvent>(next_ticker, bar.timestamp, bar.open,
                                      bar.close, bar.high, bar.low, bar.volume);
    event_queue_.push(market_event);
    current_index_[next_ticker]++;
  } else {
    is_running_ = false;
  }
}

bool HistoricCSVDataHandler::is_running() const { return is_running_; }

std::optional<Bar>
HistoricCSVDataHandler::get_latest_price_info(const std::string &ticker) const {
  auto it = current_index_.find(ticker);
  if (it == current_index_.end()) {
    std::cout << "No price info found for " << ticker << std::endl;
    return std::nullopt;
  }
  if (it->second >= all_data_.at(ticker).size()) {
    std::cout << "No more historical data for " << ticker
              << "Defaulting to last known price." << std::endl;
    return all_data_.at(ticker)[it->second - 1];
  }
  return all_data_.at(ticker)[it->second];
}