#pragma once
#include "MarketData.h"
#include "Quote.h"
#include "Fill.h"
#include "AvellanedaStoikov.h"
#include "PnLTracker.h"
#include <vector>
#include <random>
#include <chrono>

namespace mm {

// Configuration for market simulation
struct SimulationConfig {
    std::string symbol = "BTCUSDT";
    double initial_price = 45000.0;
    double volatility = 0.025;           // 2.5% daily volatility
    double tick_size = 0.01;             // Minimum price increment
    double spread_bps = 5.0;             // Market spread in basis points
    
    int num_ticks = 10000;               // Number of market updates to simulate
    double time_step_seconds = 1.0;      // Time between ticks
    
    // Fill probability parameters
    double fill_probability = 0.3;       // Base probability of getting filled
    double aggressive_fill_bonus = 0.5;  // Bonus for aggressive quotes
};

// Statistics about simulation run
struct SimulationStats {
    int total_ticks = 0;
    int total_fills = 0;
    int buy_fills = 0;
    int sell_fills = 0;
    
    double final_pnl = 0.0;
    double max_pnl = 0.0;
    double min_pnl = 0.0;
    double max_drawdown = 0.0;
    
    double final_position = 0.0;
    double max_position = 0.0;
    double min_position = 0.0;
    
    double total_fees_paid = 0.0;
    double total_volume = 0.0;
    
    std::vector<double> pnl_history;
    std::vector<double> position_history;
    std::vector<double> price_history;
    
    std::string to_string() const;
};

class MarketSimulator {
public:
    MarketSimulator(const SimulationConfig& config);
    
    // Run simulation and return statistics
    SimulationStats run_simulation(AvellanedaStoikov& algo, PnLTracker& tracker);
    
    // Generate next market tick with random walk
    MarketTick generate_next_tick();
    
    // Simulate if our quote gets filled
    bool should_fill_quote(const Quote& our_quote, const MarketTick& market_tick, bool is_buy);
    
    // Create a simulated fill
    Fill create_fill(const Quote& quote, bool is_buy, double fill_price);
    
    // Reset simulator
    void reset();
    
private:
    SimulationConfig config_;
    
    // Current state
    double current_price_;
    int current_tick_;
    
    // Random number generation
    std::mt19937 rng_;
    std::normal_distribution<double> price_change_dist_;
    std::uniform_real_distribution<double> uniform_dist_;
    
    // ID generators
    long next_order_id_ = 1;
};

} // namespace mm
