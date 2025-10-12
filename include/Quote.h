#pragma once
#include <chrono>
#include <string>
#include <xtensor.hpp>

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
  Quote(const std::string &sym, double bid, double ask, double bid_sz,
        double ask_sz, long id)
      : symbol(sym), bid_price(bid), ask_price(ask), bid_size(bid_sz),
        ask_size(ask_sz), order_id(id) {
    timestamp = std::chrono::system_clock::now();
  }

  Quote() = default;

  double mid_price() const { return (bid_price + ask_price) / 2.0; }
  double spread() const { return ask_price - bid_price; }
  double spread_bps() const { return (spread() / mid_price()) * 10000; }
  bool is_valid() const {
    return (bid_price > 0 && ask_price > 0 && bid_size > 0 && ask_size > 0);
  }

  // Additional methods (implemented in Quote.cpp)
  double get_notional_value() const;
  double get_quote_age_seconds() const;
  bool is_competitive(const Quote &market_quote) const;
  bool is_better_bid(double market_bid) const;
  bool is_better_ask(double market_ask) const;
  std::string to_string() const;

  // Static methods for batch processing with xtensor
  static xt::xarray<double>
  calculate_mid_prices(const std::vector<Quote> &quotes);
  static xt::xarray<double> calculate_spreads(const std::vector<Quote> &quotes);
  static xt::xarray<bool> validate_quotes(const std::vector<Quote> &quotes);
};

} // namespace mm