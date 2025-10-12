#pragma once
#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace mm {
struct MarketTick {
  std::chrono::system_clock::time_point timestamp;
  std::string symbol;

  double bid_price;
  double ask_price;

  double volatility;
  double volume;

  MarketTick(const std::string &sym, double bid, double ask, double vol,
             double volatility)
      : symbol(sym), bid_price(bid), ask_price(ask), volume(vol),
        volatility(volatility) {
    timestamp = std::chrono::system_clock::now();
  }

  // Default constructor
  MarketTick() = default;

  double mid_price() const { return (bid_price + ask_price) / 2.0; }
  double spread() const { return ask_price - bid_price; }
  double spread_bps() const { return (spread() / mid_price()) * 10000; }
};

class MarketDataManager {
public:
  void add_tick(const MarketTick &tick);
  double calculate_volatility(const std::string &symbol, int window_size);
  double get_vwap(const std::string &symbol, int window_size);
  MarketTick get_latest_tick(const std::string &symbol);
  std::vector<MarketTick> get_recent_ticks(const std::string &symbol,
                                           int count);

private:
  std::map<std::string, std::vector<MarketTick>> tick_history_;
  int max_history_size_ = 1000;
};

} // namespace mm