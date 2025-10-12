#pragma once
#include "Fill.h"
#include <string>


// Symbol, quantity d, avgprice d, realizedPNL d, unrealized pnl d, total pnl d
// methods, is long (quantity > 0), is short (quantity < 0), is flat (quantity
// == 0), get total pnl (realized + unrealized), get position voalue (quantity *
// current pice)

namespace mm {

struct Position {
  std::string symbol;

  double quantity;
  double average_price;
  double realized_pnl;
  double unrealized_pnl;

  Position(const std::string &sym, double avg_price, double real_pnl,
           double unreal_pnl)
      : symbol(sym), average_price(avg_price), realized_pnl(real_pnl),
        unrealized_pnl(unreal_pnl) {
    quantity = 0.0; // Initialize quantity
  }

  Position() = default;

  bool is_long() const { return (quantity > 0); }
  bool is_short() const { return (quantity < 0); }
  bool is_flat() const { return (quantity == 0); }
  double get_total_pnl() const { return (realized_pnl + unrealized_pnl); }
  double get_position_value() const { return (quantity * average_price); }

  // Methods implemented in Position.cpp
  void update_position(const Fill &fill);
  void update_unrealized_pnl(double current_price);
  double get_position_value(double current_price) const;
  double get_exposure() const;
  std::string to_string() const;
};
} // namespace mm
