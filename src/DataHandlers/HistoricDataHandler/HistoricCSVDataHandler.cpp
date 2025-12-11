#include "HistoricCSVDataHandler.h"
#include "../../../third_party/fast_cpp_csv_parser/csv.h"
#include <chrono>
#include <stdexcept>
#include <string>

std::chrono::system_clock::time_point
parse_stooq_datetime(const std::string &dateStr, const std::string &timeStr) {
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

std::vector<Bar> load_stooq_file(const std::string &filename) {
  std::vector<Bar> bars;

  // The file extension (.txt vs .csv) does not matter — it's just text.
  // Make sure the header line matches these names (case-sensitive):
  // Ticker,Per,Date,Time,Open,High,Low,Close,Vol,Openint
  io::CSVReader<10, io::trim_chars<' '>, io::no_quote_escape<','>> in(filename);

  in.read_header(io::ignore_extra_column, "TICKER", "PER", "DATE", "TIME",
                 "OPEN", "HIGH", "LOW", "CLOSE", "VOL", "OPENINT");

  std::string ticker;
  std::string perStr;
  std::string dateStr;
  std::string timeStr;
  double open, high, low, close;
  long long vol;
  long long openInt;

  while (in.read_row(ticker, perStr, dateStr, timeStr, open, high, low, close,
                     vol, openInt)) {

    if (perStr.empty()) {
      continue; // malformed row
    }

    Bar bar;
    bar.symbol = ticker;
    bar.timestamp = parse_stooq_datetime(dateStr, timeStr);
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

void HistoricCSVDataHandler::update() {}
