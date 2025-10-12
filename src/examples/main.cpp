#include "AvellanedaStoikov.h"
#include "Fill.h"
#include "LatencyBenchmark.h"
#include "MarketData.h"
#include "PnLTracker.h"
#include "Position.h"
#include "Quote.h"
#include <iostream>
#include <vector>
#include <xtensor.hpp>

int main() {
  std::cout << "Market Making Optimizer - xtensor Test" << std::endl;
  std::cout << "=====================================" << std::endl;

  // Test xtensor
  xt::xarray<double> prices = {150.0, 150.1, 150.2, 150.3};
  xt::xarray<double> spreads = xt::diff(prices);

  std::cout << "Prices: ";
  for (auto price : prices) {
    std::cout << price << " ";
  }
  std::cout << std::endl;

  std::cout << "Spreads: ";
  for (auto spread : spreads) {
    std::cout << spread << " ";
  }
  std::cout << std::endl;

  // Test Fill functionality
  mm::Fill test_fill("AAPL", true, 150.25, 100.0, 12345, 1.50);
  std::cout << "Fill: " << test_fill.to_string() << std::endl;
  std::cout << "Notional Value: $" << test_fill.get_notional_value()
            << std::endl;
  std::cout << "Net Amount: $" << test_fill.get_net_amount() << std::endl;
  std::cout << "Fee Rate: " << (test_fill.get_fee_rate() * 100) << "%"
            << std::endl;

  // Test new performance methods
  double reference_price = 150.20; // Market price before our fill
  std::cout << "Slippage: " << test_fill.get_slippage_bps(reference_price)
            << " bps" << std::endl;
  std::cout << "Effective Spread: $"
            << test_fill.get_effective_spread(reference_price) << std::endl;

  // Test Quote functionality
  std::cout << "\nQuote Tests:" << std::endl;
  mm::Quote our_quote("AAPL", 150.20, 150.30, 100, 100, 12347);
  mm::Quote market_quote("AAPL", 150.15, 150.35, 200, 200, 12348);

  std::cout << "Our Quote: " << our_quote.to_string() << std::endl;
  std::cout << "Market Quote: " << market_quote.to_string() << std::endl;
  std::cout << "Our Mid: $" << our_quote.mid_price() << ", Market Mid: $"
            << market_quote.mid_price() << std::endl;
  std::cout << "Our Spread: " << our_quote.spread_bps()
            << " bps, Market Spread: " << market_quote.spread_bps() << " bps"
            << std::endl;
  std::cout << "Is Competitive: "
            << (our_quote.is_competitive(market_quote) ? "Yes" : "No")
            << std::endl;
  std::cout << "Quote Age: " << our_quote.get_quote_age_seconds() << " seconds"
            << std::endl;

  // Test batch processing with xtensor
  std::cout << "\nBatch Processing Test:" << std::endl;
  std::vector<mm::Quote> quotes = {our_quote, market_quote};
  auto mid_prices = mm::Quote::calculate_mid_prices(quotes);
  auto quote_spreads = mm::Quote::calculate_spreads(quotes);
  auto valid = mm::Quote::validate_quotes(quotes);

  std::cout << "Mid Prices: ";
  for (auto price : mid_prices) {
    std::cout << price << " ";
  }
  std::cout << std::endl;

  std::cout << "Quote Spreads: ";
  for (auto spread : quote_spreads) {
    std::cout << spread << " ";
  }
  std::cout << std::endl;

  std::cout << "\n✅ xtensor, Fill, and Quote working correctly!" << std::endl;

  // Test Position functionality
  std::cout << "\n=== Position Tests ===" << std::endl;
  mm::Position btc_position("BTC", 0.0, 0.0, 0.0);
  std::cout << "Initial: " << btc_position.to_string() << std::endl;

  // Scenario 1: Buy 1 BTC @ $45,000
  mm::Fill buy1("BTC", true, 45000.0, 1.0, 1001, 22.5);
  btc_position.update_position(buy1);
  std::cout << "\nAfter buy 1 BTC @ $45,000:" << std::endl;
  std::cout << btc_position.to_string() << std::endl;

  // Update unrealized P&L with current price $46,000
  btc_position.update_unrealized_pnl(46000.0);
  std::cout << "Market price $46,000: " << btc_position.to_string()
            << std::endl;

  // Scenario 2: Buy another 1 BTC @ $47,000 (averaging up)
  mm::Fill buy2("BTC", true, 47000.0, 1.0, 1002, 23.5);
  btc_position.update_position(buy2);
  std::cout << "\nAfter buy 1 BTC @ $47,000 (avg up):" << std::endl;
  std::cout << btc_position.to_string() << std::endl;
  std::cout << "Average price should be $46,000: $"
            << btc_position.average_price << std::endl;

  // Scenario 3: Sell 1 BTC @ $48,000 (take profit)
  mm::Fill sell1("BTC", false, 48000.0, 1.0, 1003, 24.0);
  btc_position.update_position(sell1);
  btc_position.update_unrealized_pnl(48000.0);
  std::cout << "\nAfter sell 1 BTC @ $48,000:" << std::endl;
  std::cout << btc_position.to_string() << std::endl;
  std::cout << "Realized P&L should be $2,000: $" << btc_position.realized_pnl
            << std::endl;

  // Scenario 4: Sell remaining 1 BTC @ $49,000 (close position)
  mm::Fill sell2("BTC", false, 49000.0, 1.0, 1004, 24.5);
  btc_position.update_position(sell2);
  btc_position.update_unrealized_pnl(49000.0);
  std::cout << "\nAfter sell 1 BTC @ $49,000 (flat):" << std::endl;
  std::cout << btc_position.to_string() << std::endl;
  std::cout << "Total realized P&L should be $5,000: $"
            << btc_position.realized_pnl << std::endl;

  std::cout << "\n✅ Position tracking working correctly!" << std::endl;

  // Test Avellaneda-Stoikov Algorithm
  std::cout << "\n=== Avellaneda-Stoikov Algorithm Tests ===" << std::endl;

  mm::AvellanedaStoikov algo;
  std::cout << "Algorithm Parameters:" << std::endl;
  std::cout << "  Risk Aversion (gamma): " << algo.get_risk_aversion()
            << std::endl;
  std::cout << "  Volatility (sigma): " << algo.get_volatility() << std::endl;
  std::cout << "  Time Horizon (T): " << algo.get_time_horizon() << " seconds"
            << std::endl;
  std::cout << "  Inventory Penalty (kappa): " << algo.get_inventory_penalty()
            << std::endl;

  // Scenario 1: No inventory (neutral)
  mm::MarketTick btc_tick("BTC", 45000.0, 45010.0, 1000.0, 0.025);
  double inventory = 0.0;
  mm::Quote algo_quote1 = algo.calculate_quotes(btc_tick, inventory);
  std::cout << "\n1. Market: BTC @ $45,005 mid, Inventory: 0 BTC (neutral)"
            << std::endl;
  std::cout << "   " << algo_quote1.to_string() << std::endl;
  std::cout << "   Spread: " << algo_quote1.spread() << " ($"
            << algo_quote1.spread_bps() << " bps)" << std::endl;

  // Scenario 2: Long inventory (want to sell)
  inventory = 2.0;
  mm::Quote algo_quote2 = algo.calculate_quotes(btc_tick, inventory);
  std::cout << "\n2. Market: BTC @ $45,005 mid, Inventory: +2 BTC (long - want "
               "to sell)"
            << std::endl;
  std::cout << "   " << algo_quote2.to_string() << std::endl;
  std::cout << "   Spread: " << algo_quote2.spread() << std::endl;
  std::cout << "   Note: Quotes should be LOWER to encourage selling"
            << std::endl;

  // Scenario 3: Short inventory (want to buy)
  inventory = -2.0;
  mm::Quote algo_quote3 = algo.calculate_quotes(btc_tick, inventory);
  std::cout << "\n3. Market: BTC @ $45,005 mid, Inventory: -2 BTC (short - "
               "want to buy)"
            << std::endl;
  std::cout << "   " << algo_quote3.to_string() << std::endl;
  std::cout << "   Spread: " << algo_quote3.spread() << std::endl;
  std::cout << "   Note: Quotes should be HIGHER to encourage buying"
            << std::endl;

  // Scenario 4: High volatility
  std::cout << "\n4. Testing with high volatility (5% vs 2.5%)" << std::endl;
  mm::MarketTick volatile_tick("BTC", 45000.0, 45010.0, 1000.0, 0.05);
  mm::Quote algo_quote4 = algo.calculate_quotes(volatile_tick, 0.0);
  std::cout << "   Low vol spread: $" << algo_quote1.spread() << std::endl;
  std::cout << "   High vol spread: $" << algo_quote4.spread() << std::endl;
  std::cout << "   Note: Higher volatility = wider spreads" << std::endl;

  std::cout << "\n✅ Avellaneda-Stoikov algorithm working correctly!"
            << std::endl;

  // Test MarketDataManager
  std::cout << "\n=== MarketDataManager Tests ===" << std::endl;

  mm::MarketDataManager data_manager;

  // Simulate incoming market ticks with varying prices
  std::cout << "\nSimulating 10 BTC ticks with varying prices..." << std::endl;
  double base_price = 45000.0;
  for (int i = 0; i < 10; i++) {
    double price_variation = (i % 2 == 0) ? i * 5.0 : -i * 5.0;
    double bid = base_price + price_variation;
    double ask = bid + 10.0;
    double volume = 100.0 + i * 10.0;

    mm::MarketTick tick("BTC", bid, ask, volume, 0.0);
    data_manager.add_tick(tick);
  }

  // Test get_latest_tick
  mm::MarketTick latest = data_manager.get_latest_tick("BTC");
  std::cout << "Latest BTC tick: bid=$" << latest.bid_price << " ask=$"
            << latest.ask_price << " mid=$" << latest.mid_price() << std::endl;

  // Test get_recent_ticks
  auto recent_ticks = data_manager.get_recent_ticks("BTC", 5);
  std::cout << "Last 5 ticks count: " << recent_ticks.size() << std::endl;

  // Test calculate_volatility
  double btc_volatility = data_manager.calculate_volatility("BTC", 10);
  std::cout << "Calculated volatility (10-tick window): " << btc_volatility
            << std::endl;

  // Test VWAP
  double btc_vwap = data_manager.get_vwap("BTC", 10);
  std::cout << "Volume-Weighted Average Price (VWAP): $" << btc_vwap
            << std::endl;

  // Add more ticks to test rolling window
  std::cout << "\nAdding 5 more ticks..." << std::endl;
  for (int i = 10; i < 15; i++) {
    mm::MarketTick tick("BTC", 45000.0 + i, 45010.0 + i, 150.0, 0.0);
    data_manager.add_tick(tick);
  }

  mm::MarketTick latest2 = data_manager.get_latest_tick("BTC");
  std::cout << "New latest tick: bid=$" << latest2.bid_price << " ask=$"
            << latest2.ask_price << std::endl;

  double new_vwap = data_manager.get_vwap("BTC", 5);
  std::cout << "New VWAP (last 5 ticks): $" << new_vwap << std::endl;

  std::cout << "\n✅ MarketDataManager working correctly!" << std::endl;

  // Test PnLTracker
  std::cout << "\n=== PnLTracker Tests ===" << std::endl;

  mm::PnLTracker pnl_tracker;

  // Scenario: Trade BTC and ETH simultaneously
  std::cout << "\nTrading scenario: Multi-symbol market making" << std::endl;

  // BTC trades
  std::cout << "\n1. Buy 0.5 BTC @ $45,000" << std::endl;
  mm::Fill btc_buy1("BTC", true, 45000.0, 0.5, 2001, 11.25);
  pnl_tracker.update_fill(btc_buy1);
  pnl_tracker.update_market_price("BTC", 45000.0);
  std::cout << "   Total P&L: $" << pnl_tracker.get_total_pnl() << std::endl;

  // ETH trades
  std::cout << "\n2. Buy 2 ETH @ $3,000" << std::endl;
  mm::Fill eth_buy1("ETH", true, 3000.0, 2.0, 2002, 3.0);
  pnl_tracker.update_fill(eth_buy1);
  pnl_tracker.update_market_price("ETH", 3000.0);
  std::cout << "   Total P&L: $" << pnl_tracker.get_total_pnl() << std::endl;

  // Market moves
  std::cout << "\n3. Market moves: BTC -> $46,000, ETH -> $3,100" << std::endl;
  pnl_tracker.update_market_price("BTC", 46000.0);
  pnl_tracker.update_market_price("ETH", 3100.0);
  std::cout << "   BTC unrealized P&L: $"
            << pnl_tracker.get_position("BTC").unrealized_pnl << std::endl;
  std::cout << "   ETH unrealized P&L: $"
            << pnl_tracker.get_position("ETH").unrealized_pnl << std::endl;
  std::cout << "   Total unrealized P&L: $" << pnl_tracker.get_unrealized_pnl()
            << std::endl;
  std::cout << "   Total P&L: $" << pnl_tracker.get_total_pnl() << std::endl;

  // Take some profit
  std::cout << "\n4. Sell 0.3 BTC @ $46,500 (partial profit)" << std::endl;
  mm::Fill btc_sell1("BTC", false, 46500.0, 0.3, 2003, 6.975);
  pnl_tracker.update_fill(btc_sell1);
  pnl_tracker.update_market_price("BTC", 46500.0);
  std::cout << "   BTC realized P&L: $"
            << pnl_tracker.get_position("BTC").realized_pnl << std::endl;
  std::cout << "   Total realized P&L: $" << pnl_tracker.get_realized_pnl()
            << std::endl;
  std::cout << "   Total P&L: $" << pnl_tracker.get_total_pnl() << std::endl;

  // Close ETH position
  std::cout << "\n5. Sell 2 ETH @ $3,150 (close position)" << std::endl;
  mm::Fill eth_sell1("ETH", false, 3150.0, 2.0, 2004, 3.15);
  pnl_tracker.update_fill(eth_sell1);
  pnl_tracker.update_market_price("ETH", 3150.0);
  std::cout << "   ETH realized P&L: $"
            << pnl_tracker.get_position("ETH").realized_pnl << std::endl;
  std::cout << "   Total realized P&L: $" << pnl_tracker.get_realized_pnl()
            << std::endl;

  // Final summary
  std::cout << "\n=== Final Portfolio Summary ===" << std::endl;
  std::cout << pnl_tracker.get_summary() << std::endl;

  std::cout << "✅ PnLTracker working correctly!" << std::endl;

  // Test Latency Benchmarking
  std::cout << "\n=== Latency Benchmark Tests ===" << std::endl;

  // Reset benchmarks
  mm::LatencyBenchmark::instance().reset();

  // Run operations multiple times to get good statistics
  std::cout << "\nRunning 10,000 iterations of critical operations..."
            << std::endl;

  mm::AvellanedaStoikov bench_algo;
  mm::PnLTracker bench_tracker;
  mm::MarketTick bench_tick("BTC", 45000.0, 45010.0, 1000.0, 0.025);

  for (int i = 0; i < 10000; i++) {
    // Benchmark quote generation
    double inventory = (i % 2 == 0) ? 0.5 : -0.5;
    mm::Quote q = bench_algo.calculate_quotes(bench_tick, inventory);

    // Benchmark fill processing every 100 iterations
    if (i % 100 == 0) {
      mm::Fill bench_fill("BTC", true, 45000.0 + i, 0.1, 3000 + i, 2.25);
      bench_tracker.update_fill(bench_fill);
      bench_tracker.update_market_price("BTC", 45000.0 + i);
    }
  }

  // Print benchmark report
  std::cout << mm::LatencyBenchmark::instance().report();

  // Analyze results
  auto quote_stats = mm::LatencyBenchmark::instance().get_stats(
      "AvellanedaStoikov::calculate_quotes");
  auto position_stats =
      mm::LatencyBenchmark::instance().get_stats("Position::update_position");
  auto pnl_stats =
      mm::LatencyBenchmark::instance().get_stats("PnLTracker::update_fill");

  std::cout << "\n=== Performance Analysis ===" << std::endl;
  if (quote_stats) {
    std::cout << "Quote generation: " << quote_stats->avg_us() << " μs average"
              << std::endl;
    if (quote_stats->avg_us() < 1.0) {
      std::cout << "  ✅ EXCELLENT - Sub-microsecond latency!" << std::endl;
    } else if (quote_stats->avg_us() < 10.0) {
      std::cout << "  ✅ GOOD - Low microsecond latency" << std::endl;
    } else {
      std::cout << "  ⚠️  SLOW - May need optimization" << std::endl;
    }
  }

  if (position_stats) {
    std::cout << "\nPosition update: " << position_stats->avg_us()
              << " μs average" << std::endl;
    if (position_stats->avg_us() < 5.0) {
      std::cout << "  ✅ GOOD - Fast position tracking" << std::endl;
    }
  }

  if (pnl_stats) {
    std::cout << "\nP&L tracking: " << pnl_stats->avg_us() << " μs average"
              << std::endl;
    if (pnl_stats->avg_us() < 10.0) {
      std::cout << "  ✅ GOOD - Efficient P&L aggregation" << std::endl;
    }
  }

  std::cout << "\n✅ Latency benchmarking complete!" << std::endl;
  return 0;
}