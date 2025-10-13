#include "MarketSimulator.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace mm {

MarketSimulator::MarketSimulator(const SimulationConfig &config)
    : config_(config), current_price_(config.initial_price), current_tick_(0),
      rng_(std::random_device{}()),
      price_change_dist_(
          0.0, // Mean = 0 (no drift, pure random walk)
          config.volatility /
              std::sqrt(252.0 * 24.0 * 3600.0 / config.time_step_seconds)),
      uniform_dist_(0.0, 1.0) {}

MarketTick MarketSimulator::generate_next_tick() {
  double price_change = price_change_dist_(rng_);
  current_price_ += price_change;

  double spread_dollars = current_price_ * (config_.spread_bps / 10000.0);
  double bid_price = current_price_ - (spread_dollars / 2.0);
  double ask_price = current_price_ + (spread_dollars / 2.0);

  double volume = 100.0 + (uniform_dist_(rng_) * 100.0);
  current_tick_++;

  return MarketTick(config_.symbol, bid_price, ask_price, volume,
                    config_.volatility);
}

bool MarketSimulator::should_fill_quote(const Quote &our_quote,
                                        const MarketTick &market_tick,
                                        bool is_buy) {
  // Get our price and market price
  double our_price = is_buy ? our_quote.bid_price : our_quote.ask_price;
  double market_price = is_buy ? market_tick.bid_price : market_tick.ask_price;

  // Calculate how aggressive our quote is (positive = more aggressive)
  double price_diff =
      is_buy ? (our_price - market_price) : (market_price - our_price);
  double aggressiveness = price_diff / market_price;

  // Calculate fill probability
  double base_prob = config_.fill_probability;
  double aggressive_bonus =
      aggressiveness > 0 ? aggressiveness * config_.aggressive_fill_bonus : 0.0;
  double fill_prob = base_prob + aggressive_bonus;

  // Cap probability between 0 and 1
  fill_prob = std::max(0.0, std::min(1.0, fill_prob));

  // Random dice roll
  return uniform_dist_(rng_) < fill_prob;
}

Fill MarketSimulator::create_fill(const Quote &quote, bool is_buy,
                                  double fill_price) {
  double fill_size = is_buy ? quote.bid_size : quote.ask_size;
  double fees = fill_price * fill_size * 0.001; // 0.1% fee (10 basis points)

  return Fill(quote.symbol, is_buy, fill_price, fill_size, next_order_id_++,
              fees);
}

void MarketSimulator::reset() {
  current_price_ = config_.initial_price;
  current_tick_ = 0;
  next_order_id_ = 1;
}

SimulationStats MarketSimulator::run_simulation(AvellanedaStoikov &algo,
                                                PnLTracker &tracker) {
  SimulationStats stats;
  reset();

  std::cout << "Running simulation: " << config_.num_ticks << " ticks for "
            << config_.symbol << std::endl;

  for (int i = 0; i < config_.num_ticks; i++) {
    // 1. Generate market tick
    MarketTick tick = generate_next_tick();

    // 2. Get current position
    Position pos = tracker.get_position(config_.symbol);
    double inventory = pos.quantity;

    // 3. Ask algorithm for quotes
    Quote our_quote = algo.calculate_quotes(tick, inventory);

    // 4. Check if orders get filled
    if (should_fill_quote(our_quote, tick, true)) {
      // Buy order filled!
      Fill fill = create_fill(our_quote, true, our_quote.bid_price);
      tracker.update_fill(fill);
      stats.buy_fills++;
      stats.total_fills++;
      stats.total_fees_paid += fill.fees;
      stats.total_volume += fill.get_notional_value();
    }

    if (should_fill_quote(our_quote, tick, false)) {
      // Sell order filled!
      Fill fill = create_fill(our_quote, false, our_quote.ask_price);
      tracker.update_fill(fill);
      stats.sell_fills++;
      stats.total_fills++;
      stats.total_fees_paid += fill.fees;
      stats.total_volume += fill.get_notional_value();
    }

    // 5. Update market price for P&L
    tracker.update_market_price(config_.symbol, tick.mid_price());

    // 6. Get updated position after fills
    pos = tracker.get_position(config_.symbol);

    // 7. Record statistics
    double current_pnl = tracker.get_total_pnl();
    stats.pnl_history.push_back(current_pnl);
    stats.position_history.push_back(pos.quantity);
    stats.price_history.push_back(tick.mid_price());

    // Track extremes
    if (current_pnl > stats.max_pnl)
      stats.max_pnl = current_pnl;
    if (current_pnl < stats.min_pnl)
      stats.min_pnl = current_pnl;
    if (pos.quantity > stats.max_position)
      stats.max_position = pos.quantity;
    if (pos.quantity < stats.min_position)
      stats.min_position = pos.quantity;

    // Calculate drawdown
    if (stats.max_pnl > 0) {
      double drawdown = (stats.max_pnl - current_pnl) / stats.max_pnl;
      if (drawdown > stats.max_drawdown)
        stats.max_drawdown = drawdown;
    }

    // Progress indicator (every 1000 ticks)
    if (i > 0 && i % 1000 == 0) {
      std::cout << "  Progress: " << i << "/" << config_.num_ticks
                << " ticks, P&L: $" << current_pnl << std::endl;
    }
  }

  // Calculate final statistics
  stats.total_ticks = config_.num_ticks;
  stats.final_pnl = tracker.get_total_pnl();
  stats.final_position = tracker.get_position(config_.symbol).quantity;

  return stats;
}

std::string SimulationStats::to_string() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2);
  oss << "\n=== Simulation Results ===\n";
  oss << "Total Ticks:        " << total_ticks << "\n";
  oss << "Total Fills:        " << total_fills << " ("
      << (total_fills * 100.0 / total_ticks) << "% fill rate)\n";
  oss << "  Buy Fills:        " << buy_fills << "\n";
  oss << "  Sell Fills:       " << sell_fills << "\n\n";

  oss << "P&L Performance:\n";
  oss << "  Final P&L:        $" << final_pnl << "\n";
  oss << "  Max P&L:          $" << max_pnl << "\n";
  oss << "  Min P&L:          $" << min_pnl << "\n";
  oss << "  Max Drawdown:     " << (max_drawdown * 100.0) << "%\n\n";

  oss << "Position Stats:\n";
  oss << "  Final Position:   " << final_position << "\n";
  oss << "  Max Position:     " << max_position << "\n";
  oss << "  Min Position:     " << min_position << "\n\n";

  oss << "Trading Stats:\n";
  oss << "  Total Volume:     $" << total_volume << "\n";
  oss << "  Total Fees Paid:  $" << total_fees_paid << "\n";
  oss << "  (Note: Fees already deducted from P&L above)\n";

  return oss.str();
}

} // namespace mm
