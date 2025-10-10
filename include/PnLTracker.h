#pragma once
#include <string>
#include <map>
#include "Fill.h"
#include "Position.h"

namespace mm {
    class PnLTracker {
    public:
        // Constructor
        PnLTracker();
        
        // Update methods
        void update_fill(const Fill& fill);
        void update_market_price(const std::string& symbol, double price);
        
        // Getter methods
        double get_realized_pnl() const;
        double get_unrealized_pnl() const;
        double get_total_pnl() const;
        
    private:
        double realized_pnl_;
        double unrealized_pnl_;
        std::map<std::string, Position> positions_;  // symbol -> position
    };
}