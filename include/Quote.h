#pragma once
#include <string>
#include <chrono> 

namespace mm {
    
    struct Quote {
        std::chrono::system_clock::time_point timestamp;
        std::string symbol;

        double bid_price;
        double ask_price;
        double bid_size;
        double ask_size;

        long order_id;

        // Constructor
        Quote(const std::string& sym, double bid, double ask, double bid_sz, double ask_sz, long id)
            : symbol(sym), bid_price(bid), ask_price(ask), bid_size(bid_sz), ask_size(ask_sz), order_id(id) {
            timestamp = std::chrono::system_clock::now();
        }

        Quote() = default;

        double mid_price() const { return (bid_price + ask_price) / 2.0; }
        double spread() const { return ask_price - bid_price; }
        double spread_bps() const { return (spread() / mid_price()) * 10000; }
        bool is_valid() const { return (bid_price > 0 && ask_price > 0 && bid_size > 0 && ask_size > 0); }
    
        

    };

}