#include "Position.h"
#include "Fill.h"
#include "LatencyBenchmark.h"
#include <cmath>
#include <string>

namespace mm {

void Position::update_position(const Fill &fill) {
  BENCHMARK_SCOPE("Position::update_position");

  double old_quantity = quantity;
  double old_avg_price = average_price;

  // Update quantity based on buy/sell
  if (fill.is_buy_fill()) {
    quantity += fill.size;
  } else {
    quantity -= fill.size;
  }

  // Update average price and realized P&L
  if (old_quantity == 0) {
    // Opening a new position
    average_price = fill.price;
  } else if ((old_quantity > 0 && fill.is_buy_fill()) ||
             (old_quantity < 0 && !fill.is_buy_fill())) {
    // Adding to existing position - weighted average
    average_price =
        (std::abs(old_quantity) * old_avg_price + fill.size * fill.price) /
        std::abs(quantity);
  } else {
    // Reducing or flipping position
    double closed_size = std::min(std::abs(old_quantity), fill.size);

    // Calculate realized P&L for the closed portion
    if (old_quantity > 0) {
      // Closing long position with sell
      realized_pnl += closed_size * (fill.price - old_avg_price);
    } else {
      // Closing short position with buy
      realized_pnl += closed_size * (old_avg_price - fill.price);
    }

    // If flipping position, update average price for new direction
    if (std::abs(old_quantity) < fill.size) {
      average_price = fill.price;
    }
    // Otherwise keep the old average price
  }
}

void Position::update_unrealized_pnl(double current_price) {
  if (quantity > 0) {
    // Long position
    unrealized_pnl = quantity * (current_price - average_price);
  } else if (quantity < 0) {
    // Short position
    unrealized_pnl = std::abs(quantity) * (average_price - current_price);
  } else {
    // Flat position
    unrealized_pnl = 0.0;
  }
}

double Position::get_position_value(double current_price) const {
  return quantity * current_price;
}

double Position::get_exposure() const {
  return std::abs(quantity * average_price);
}

std::string Position::to_string() const {
  std::string direction = is_long() ? "LONG" : (is_short() ? "SHORT" : "FLAT");
  return "Position{" + symbol + " " + direction +
         " qty=" + std::to_string(quantity) + " avg=$" +
         std::to_string(average_price) + " realized=$" +
         std::to_string(realized_pnl) + " unrealized=$" +
         std::to_string(unrealized_pnl) + " total=$" +
         std::to_string(get_total_pnl()) + "}";
}

} // namespace mm
