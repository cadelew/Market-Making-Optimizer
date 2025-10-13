#include "AvellanedaStoikov.h"
#include "MarketSimulator.h"
#include "PnLTracker.h"
#include <iostream>


int main() {
  std::cout << "=== Market Making Simulation Test ===" << std::endl;
  std::cout << "Testing Avellaneda-Stoikov algorithm profitability\n"
            << std::endl;

  // Configure simulation
  mm::SimulationConfig config;
  config.symbol = "BTCUSDT";
  config.initial_price = 45000.0;
  config.volatility = 0.025;      // 2.5% annual volatility
  config.spread_bps = 5.0;        // Market spread: 5 bps
  config.num_ticks = 10000;       // Simulate 10,000 ticks (~3 hours of data)
  config.time_step_seconds = 1.0; // 1 tick per second
  config.fill_probability = 0.3;  // 30% base fill rate
  config.aggressive_fill_bonus = 0.5; // Bonus for aggressive quotes

  // Create simulator
  mm::MarketSimulator simulator(config);

  // Create algorithm with default parameters
  mm::AvellanedaStoikov algo;
  std::cout << "Algorithm Parameters:" << std::endl;
  std::cout << "  Risk Aversion (gamma): " << algo.get_risk_aversion()
            << std::endl;
  std::cout << "  Volatility (sigma): " << algo.get_volatility() << std::endl;
  std::cout << "  Time Horizon (T): " << algo.get_time_horizon() << " seconds"
            << std::endl;
  std::cout << "  Inventory Penalty (kappa): " << algo.get_inventory_penalty()
            << "\n"
            << std::endl;

  // Create P&L tracker
  mm::PnLTracker tracker;

  // Run simulation
  std::cout << "Starting simulation..." << std::endl;
  mm::SimulationStats results = simulator.run_simulation(algo, tracker);

  // Display results
  std::cout << results.to_string() << std::endl;

  // Final portfolio state
  std::cout << "\n=== Final Portfolio State ===" << std::endl;
  std::cout << tracker.get_summary() << std::endl;

  // Analysis
  std::cout << "\n=== Strategy Analysis ===" << std::endl;
  if (results.final_pnl > 0) {
    std::cout << "✅ PROFITABLE - Strategy made money!" << std::endl;
  } else {
    std::cout << "❌ UNPROFITABLE - Strategy lost money!" << std::endl;
  }

  double pnl_per_tick = results.final_pnl / results.total_ticks;
  std::cout << "P&L per tick: $" << pnl_per_tick << std::endl;

  if (results.total_fills > 0) {
    double pnl_per_trade = results.final_pnl / results.total_fills;
    std::cout << "P&L per trade: $" << pnl_per_trade << std::endl;
  }

  std::cout << "\n✅ Simulation complete!" << std::endl;
  return 0;
}
