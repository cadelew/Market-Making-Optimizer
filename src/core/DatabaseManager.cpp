#include "DatabaseManager.h"

namespace mm {

DatabaseManager::DatabaseManager(const std::string &connection_string)
    : conn_(connection_string) {
  // Connection happens automatically in pqxx::connection constructor
}

void DatabaseManager::insert_market_tick(const std::string &symbol,
                                         const std::string &timestamp,
                                         double bid, double bid_size,
                                         double ask, double ask_size,
                                         double spread, double mid_price) {
  pqxx::work tx(conn_);
  tx.exec_params("INSERT INTO market_ticks (time, symbol, bid, bid_size, ask, "
                 "ask_size, spread, mid_price) "
                 "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                 timestamp, symbol, bid, bid_size, ask, ask_size, spread,
                 mid_price);
  tx.commit();
}

void DatabaseManager::insert_quote(const std::string &symbol) {
  // TODO: Implement later
}

void DatabaseManager::insert_fill(const std::string &symbol) {
  // TODO: Implement later
}

void DatabaseManager::get_latest_price(const std::string &symbol, double &bid,
                                       double &ask) {
  // TODO: Implement later
}

} // namespace mm