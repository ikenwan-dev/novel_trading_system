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

TEST(PortfolioTest, HandlesSellFill) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  // Buy 100 shares
  FillEvent buy_event("AAPL", std::chrono::system_clock::now(),
                      OrderDirection::BUY, 100.0, 150, 5);
  portfolio.on_fill(buy_event);

  // Sell 50 shares
  FillEvent sell_event("AAPL", std::chrono::system_clock::now(),
                       OrderDirection::SELL, 50.0, 160, 5);
  portfolio.on_fill(sell_event);

  // Expected cash: 100k - (150*100 + 5) + (160*50 - 5)
  // = 100000 - 15005 + 7995 = 92990
  double expected_cash =
      100000.0 - (150.0 * 100.0 + 5.0) + (160.0 * 50.0 - 5.0);
  ASSERT_DOUBLE_EQ(portfolio.get_cash(), expected_cash);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, 50);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").market_value, 50 * 160);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").cost_basis,
            (100 * 150 + 5) - (50 * 150 + 2.5));
}

TEST(PortfolioTest, HandlesShortFill) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  // Short 100 shares
  FillEvent short_event("AAPL", std::chrono::system_clock::now(),
                        OrderDirection::SELL, 100.0, 150, 5);
  portfolio.on_fill(short_event);

  // Expected cash: 100k + (150*100 - 5)
  double expected_cash = 100000.0 + (150.0 * 100.0 - 5.0);
  ASSERT_DOUBLE_EQ(portfolio.get_cash(), expected_cash);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, -100);
}

TEST(PortfolioTest, HandlesCoverFill) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  // Short 100 shares
  FillEvent short_event("AAPL", std::chrono::system_clock::now(),
                        OrderDirection::SELL, 100.0, 150, 5);
  portfolio.on_fill(short_event);

  // Cover 50 shares
  FillEvent cover_event("AAPL", std::chrono::system_clock::now(),
                        OrderDirection::BUY, 50.0, 140, 5);
  portfolio.on_fill(cover_event);

  // Expected cash: (100k + (150*100 - 5)) - (140*50 + 5)
  double expected_cash =
      (100000.0 + (150.0 * 100.0 - 5.0)) - (140.0 * 50.0 + 5.0);
  ASSERT_DOUBLE_EQ(portfolio.get_cash(), expected_cash);
  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, -50);
}

TEST(PortfolioTest, HandlesPositionFlip) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  // Buy 100 shares
  FillEvent buy_event("AAPL", std::chrono::system_clock::now(),
                      OrderDirection::BUY, 100.0, 150, 5);
  portfolio.on_fill(buy_event);

  // Sell 200 shares (Flip to Short 100)
  FillEvent sell_event("AAPL", std::chrono::system_clock::now(),
                       OrderDirection::SELL, 200.0, 160, 5);
  portfolio.on_fill(sell_event);

  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, -100);

  // Verify Logic:
  // 1. Initial Cash: 100,000 - 15,005 = 84,995
  // 2. Sell 200 @ 160 (Comm 5). Proceeds = 32,000 - 5 = 31,995
  // Final Cash = 84,995 + 31,995 = 116,990
  double expected_cash = 116990.0;
  ASSERT_DOUBLE_EQ(portfolio.get_cash(), expected_cash);
}

TEST(PortfolioTest, HandlesMultipleFills) {
  MockDataHandler data_handler;
  Portfolio portfolio(data_handler, 100000.0);

  // Buy 100 @ 100
  FillEvent buy1("AAPL", std::chrono::system_clock::now(), OrderDirection::BUY,
                 100.0, 100.0, 5.0);
  portfolio.on_fill(buy1);

  // Buy 100 @ 200
  FillEvent buy2("AAPL", std::chrono::system_clock::now(), OrderDirection::BUY,
                 100.0, 200.0, 5.0);
  portfolio.on_fill(buy2);

  // Total Quantity: 200
  ASSERT_EQ(portfolio.all_positions().at("AAPL").quantity, 200);

  // Total Cost Basis: (100*100+5) + (100*200+5) = 10005 + 20005 = 30010
  // Average Price: 30010 / 200 = 150.05
  // (Note: Portfolio implementation tracks total cost basis)
  // pos.cost_basis += trade_cost; So cost basis should be 30010.

  auto aapl_pos = portfolio.all_positions().at("AAPL");
  ASSERT_DOUBLE_EQ(aapl_pos.cost_basis, 30010.0);
}