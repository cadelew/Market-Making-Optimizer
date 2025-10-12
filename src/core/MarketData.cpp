// TODO: Implement MarketData functionality if needed
#include "MarketData.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <xtensor.hpp>

namespace mm {

void MarketDataManager::add_tick(const MarketTick &tick) {
  tick_history_[tick.symbol].push_back(tick);
  if (tick_history_[tick.symbol].size() > max_history_size_) {
    tick_history_[tick.symbol].erase(tick_history_[tick.symbol].begin());
  }
}

double MarketDataManager::calculate_volatility(const std::string &symbol,
                                               int window_size) {
  auto &ticks = tick_history_[symbol];

  if (ticks.size() < 2)
    return 0.0;

  int count = std::min(window_size, (int)ticks.size());

  // Extract prices from the last 'count' ticks
  std::vector<double> prices;
  int start_idx = ticks.size() - count;
  for (int i = start_idx; i < (int)ticks.size(); i++) {
    prices.push_back(ticks[i].mid_price());
  }

  // Calculate log returns: ln(price[i] / price[i-1])
  std::vector<double> returns;
  for (size_t i = 1; i < prices.size(); i++) {
    returns.push_back(std::log(prices[i] / prices[i - 1]));
  }

  // Calculate mean of returns
  double mean = 0.0;
  for (double r : returns) {
    mean += r;
  }
  mean /= returns.size();

  // Calculate variance (standard deviation squared)
  double variance = 0.0;
  for (double r : returns) {
    variance += std::pow(r - mean, 2);
  }
  variance /= returns.size();

  // Standard deviation (volatility)
  double std_dev = std::sqrt(variance);

  // Annualize (assuming 1 tick per second, adjust as needed)
  // 252 trading days * 24 hours * 60 minutes * 60 seconds
  double annualized_vol = std_dev * std::sqrt(252.0 * 24.0 * 60.0 * 60.0);

  return annualized_vol;
}

double MarketDataManager::get_vwap(const std::string &symbol, int window_size) {
  auto &ticks = tick_history_[symbol];

  if (ticks.empty())
    return 0.0;

  int count = std::min(window_size, (int)ticks.size());
  int start_idx = ticks.size() - count;

  double sum_price_volume = 0.0;
  double sum_volume = 0.0;

  for (int i = start_idx; i < (int)ticks.size(); i++) {
    double price = ticks[i].mid_price();
    double volume = ticks[i].volume;
    sum_price_volume += price * volume;
    sum_volume += volume;
  }

  if (sum_volume == 0.0)
    return 0.0;

  return sum_price_volume / sum_volume;
}

MarketTick MarketDataManager::get_latest_tick(const std::string &symbol) {
  auto &ticks = tick_history_[symbol];

  if (ticks.empty()) {
    return MarketTick(); // Return default-constructed tick
  }

  return ticks.back(); // Return most recent tick
}

std::vector<MarketTick>
MarketDataManager::get_recent_ticks(const std::string &symbol, int count) {
  auto &ticks = tick_history_[symbol];

  if (ticks.empty()) {
    return std::vector<MarketTick>();
  }

  int actual_count = std::min(count, (int)ticks.size());
  int start_idx = ticks.size() - actual_count;

  return std::vector<MarketTick>(ticks.begin() + start_idx, ticks.end());
}

} // namespace mm