#pragma once
#include "Fill.h"
#include "Position.h"
#include "Symbol.h"
#include <array>
#include <string>

namespace mm {
class PnLTracker {
public:
  // Constructor
  PnLTracker();

  // Update methods
  void update_fill(const Fill &fill);
  void update_market_price(const std::string &symbol, double price);
  void update_market_price(Symbol symbol, double price);

  // Getter methods
  double get_realized_pnl() const;
  double get_unrealized_pnl() const;
  double get_total_pnl() const;
  double get_sharpe_ratio() const;
  double get_max_drawdown() const;
  double get_win_rate() const;
  int get_total_trades() const;
  std::string get_summary() const;

  Position get_position(const std::string &symbol) const;
  Position &get_position(Symbol symbol);
  const Position &get_position(Symbol symbol) const;
  double get_position_pnl(const std::string &symbol) const;

private:
  double realized_pnl_;
  double unrealized_pnl_;

  // Fixed-size array for O(1) access - zero hashing overhead!
  std::array<Position, SYMBOL_COUNT> positions_;
};
} // namespace mm