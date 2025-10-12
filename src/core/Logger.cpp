#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

namespace mm {

Logger::Logger() {
    std::cout << "Logger initialized" << std::endl;
}

void Logger::log_info(const std::string& message) {
    log_with_timestamp("INFO", message);
}

void Logger::log_error(const std::string& message) {
    log_with_timestamp("ERROR", message);
}

void Logger::log_quote_placed(const Quote& quote) {
    std::string msg = "Quote placed: " + quote.symbol + 
                     " bid=" + std::to_string(quote.bid_price) + 
                     " ask=" + std::to_string(quote.ask_price);
    log_with_timestamp("TRADE", msg);
}

void Logger::log_fill(const Fill& fill) {
    std::string side = fill.is_buy ? "BUY" : "SELL";
    std::string msg = "Fill: " + fill.symbol + " " + side + 
                     " " + std::to_string(fill.size) + 
                     " @ " + std::to_string(fill.price);
    log_with_timestamp("FILL", msg);
}

void Logger::log_with_timestamp(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[" << level << "] " 
              << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
              << " - " << message << std::endl;
}

} // namespace mm