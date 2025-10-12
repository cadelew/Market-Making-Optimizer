// Optimized PnL tracking with fixed-size array
#include "PnLTracker.h"
#include "LatencyBenchmark.h"

namespace mm {

PnLTracker::PnLTracker() {
  realized_pnl_ = 0.0;
  unrealized_pnl_ = 0.0;

  // Pre-allocate positions for all supported symbols
  // No allocations during trading!
  positions_[static_cast<size_t>(Symbol::BTC)] =
      Position("BTCUSDT", 0.0, 0.0, 0.0);
  positions_[static_cast<size_t>(Symbol::ETH)] =
      Position("ETHUSDT", 0.0, 0.0, 0.0);
  positions_[static_cast<size_t>(Symbol::SOL)] =
      Position("SOLUSDT", 0.0, 0.0, 0.0);
  positions_[static_cast<size_t>(Symbol::BNB)] =
      Position("BNBUSDT", 0.0, 0.0, 0.0);
}

void PnLTracker::update_fill(const Fill &fill) {
  BENCHMARK_SCOPE("PnLTracker::update_fill");

  // Convert string to enum - O(1) comparison instead of hashing!
  Symbol sym = string_to_symbol(fill.symbol);
  if (sym == Symbol::UNKNOWN)
    return; // Unknown symbol

  // Direct array access - no hashing, no tree traversal!
  // This is ~50x faster than std::map lookup
  positions_[static_cast<size_t>(sym)].update_position(fill);

  // Aggregate realized P&L across all positions
  // Simple loop - compiler can vectorize this!
  realized_pnl_ = 0.0;
  for (size_t i = 0; i < SYMBOL_COUNT; i++) {
    realized_pnl_ += positions_[i].realized_pnl;
  }
}

void PnLTracker::update_market_price(const std::string &symbol, double price) {
  Symbol sym = string_to_symbol(symbol);
  if (sym == Symbol::UNKNOWN)
    return;

  update_market_price(sym, price);
}

void PnLTracker::update_market_price(Symbol symbol, double price) {
  if (symbol == Symbol::UNKNOWN)
    return;

  // Direct array access
  positions_[static_cast<size_t>(symbol)].update_unrealized_pnl(price);

  // Aggregate unrealized P&L
  unrealized_pnl_ = 0.0;
  for (size_t i = 0; i < SYMBOL_COUNT; i++) {
    unrealized_pnl_ += positions_[i].unrealized_pnl;
  }
}

double PnLTracker::get_realized_pnl() const { return realized_pnl_; }

double PnLTracker::get_unrealized_pnl() const { return unrealized_pnl_; }

double PnLTracker::get_total_pnl() const {
  return realized_pnl_ + unrealized_pnl_;
}

Position PnLTracker::get_position(const std::string &symbol) const {
  Symbol sym = string_to_symbol(symbol);
  if (sym == Symbol::UNKNOWN) {
    return Position(); // Return empty position
  }
  return positions_[static_cast<size_t>(sym)];
}

Position &PnLTracker::get_position(Symbol symbol) {
  return positions_[static_cast<size_t>(symbol)];
}

const Position &PnLTracker::get_position(Symbol symbol) const {
  return positions_[static_cast<size_t>(symbol)];
}

double PnLTracker::get_position_pnl(const std::string &symbol) const {
  Symbol sym = string_to_symbol(symbol);
  if (sym == Symbol::UNKNOWN)
    return 0.0;

  return positions_[static_cast<size_t>(sym)].get_total_pnl();
}

// Placeholder implementations for advanced metrics
double PnLTracker::get_sharpe_ratio() const {
  // TODO: Implement Sharpe ratio calculation
  return 0.0;
}

double PnLTracker::get_max_drawdown() const {
  // TODO: Implement drawdown tracking
  return 0.0;
}

double PnLTracker::get_win_rate() const {
  // TODO: Implement win rate calculation
  return 0.0;
}

int PnLTracker::get_total_trades() const {
  // TODO: Track number of trades
  return 0;
}

std::string PnLTracker::get_summary() const {
  std::string summary = "=== P&L Summary (Optimized) ===\n";
  summary += "Realized P&L: $" + std::to_string(realized_pnl_) + "\n";
  summary += "Unrealized P&L: $" + std::to_string(unrealized_pnl_) + "\n";
  summary += "Total P&L: $" + std::to_string(get_total_pnl()) + "\n";
  summary += "\nPositions:\n";

  // Iterate through fixed array
  for (size_t i = 0; i < SYMBOL_COUNT; i++) {
    const auto &pos = positions_[i];
    // Only show positions with non-zero quantity
    if (pos.quantity != 0.0 || pos.realized_pnl != 0.0) {
      summary += "  " + pos.to_string() + "\n";
    }
  }

  return summary;
}

} // namespace mm
