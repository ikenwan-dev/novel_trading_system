#include "Events/FillEvent/FillEvent.h"
#include "Events/OrderEvent/OrderEvent.h"
#include "Portfolio/Portfolio.h"
#include "gtest/gtest.h"

class MockDataHandler : public DataHandler {
  void update() override {}
  bool is_running() const override { return false; }
  std::optional<Bar>
  get_latest_price_info(const std::string &ticker) const override {
    return std::nullopt;
  }
};

TEST(PortfolioTest, HandlesBuyFill) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  FillEvent fill_event("AAPL", std::chrono::system_clock::now(),
                       OrderDirection::BUY, 100.0, 150, 5);

  portfolio.on_fill(fill_event);

  double exepected = 100000.0 - (150 * 100) - 5;
  ASSERT_DOUBLE_EQ(portfolio.get_cash(), exepected);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, 100);
}