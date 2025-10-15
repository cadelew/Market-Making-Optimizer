#pragma once
#include <pqxx/pqxx>
#include <string>

namespace mm {
class DatabaseManager {
public:
  DatabaseManager(const std::string &connection_string);

  void insert_market_tick(const std::string &symbol,
                          const std::string &timestamp, double bid,
                          double bid_size, double ask, double ask_size,
                          double spread = 0.0, double mid_price = 0.0);

  void insert_quote(const std::string &symbol);
  void insert_fill(const std::string &symbol);
  void get_latest_price(const std::string &symbol, double &bid, double &ask);

private:
  pqxx::connection conn_;
};
} // namespace mm
