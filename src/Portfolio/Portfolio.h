#pragma once

#include "Events/MarketEvent/MarketEvent.h"
#include "Events/FillEvent/FillEvent.h"
#include "DataHandlers/DataHandler.h"
#include <string>
#include <map>


class Portfolio {
    public:
    Portfolio(DataHandler& data_handler, double initial_capital);

    // For updating portfolio holdngs, cash, etc
    void on_fill(const FillEvent& event);

    // For updating unrealized P/L
    void on_market_data(const MarketEvent& event);

    double get_total_value() const;

    double get_cash() const { return cash_;}

    private:
    DataHandler& data_handler_;
    double initial_capital_;
    double cash_;

    // maps from ticker to share quantities
    std::map<std::string, int> holdings_;

    struct Position {
        double market_value = 0.0;
        double cost_basis = 0.0;  
    };
    std::map<std::string, Position> positions_;
};

    