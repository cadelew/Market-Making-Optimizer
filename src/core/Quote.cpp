#include "Quote.h"
#include <algorithm>
#include <iostream>
#include <vector>

namespace mm {

double Quote::get_notional_value() const {
  return (bid_price * bid_size + ask_price * ask_size) / 2.0;
}

double Quote::get_quote_age_seconds() const {
  auto now = std::chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
  return duration.count() / 1000.0;
}

bool Quote::is_competitive(const Quote &market_quote) const {
  return is_better_bid(market_quote.bid_price) ||
         is_better_ask(market_quote.ask_price);
}

bool Quote::is_better_bid(double market_bid) const {
  return bid_price > market_bid;
}

bool Quote::is_better_ask(double market_ask) const {
  return ask_price < market_ask;
}

std::string Quote::to_string() const {
  return "Quote{" + symbol + " bid=" + std::to_string(bid_price) + "@" +
         std::to_string(bid_size) + " ask=" + std::to_string(ask_price) + "@" +
         std::to_string(ask_size) + " id:" + std::to_string(order_id) + "}";
}


} // namespace mm
