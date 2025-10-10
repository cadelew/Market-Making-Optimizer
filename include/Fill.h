#pragma once
#include <string>
#include <chrono>

namespace mm {

    struct Fill {
        std::chrono::system_clock::time_point timestamp;
        std::string symbol;
        bool is_buy;
        double price;
        double size;
        long order_id;
        double fees;
    
        Fill(const std::string& sym, bool buy, double price, double size, long id, double fees)
            : symbol(sym), is_buy(buy), price(price), size(size), order_id(id), fees(fees) {
            timestamp = std::chrono::system_clock::now();
        }
        

        Fill() = default;
        
        std::string get_side() const {
            return is_buy ? "buy" : "sell";
        }
        
        bool is_buy_fill() const {
            return is_buy;
        }
    };
}