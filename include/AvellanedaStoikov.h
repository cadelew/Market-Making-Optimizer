#pragma once
#include "MarketData.h"
#include "Quote.h"
#include <string>
#include <vector>
// Optimized implementation with pre-computed constants and batch processing

namespace mm {

class AvellanedaStoikov {
public:
  AvellanedaStoikov();

  Quote calculate_quotes(const MarketTick &tick, double inventory);

  // Xtensor-optimized batch processing for multiple ticks
  std::vector<Quote>
  calculate_quotes_batch(const std::vector<MarketTick> &ticks,
                         const std::vector<double> &inventories);

  void set_risk_aversion(double gamma);
  void set_volatility(double sigma);
  void set_time_horizon(double T);
  void set_inventory_penalty(double kappa);

  double get_risk_aversion() const;
  double get_volatility() const;
  double get_time_horizon() const;
  double get_inventory_penalty() const;

private:
  double gamma_; // risk aversion parameter
  double sigma_; // volatility
  double T_;     // time horizon (seconds)
  double kappa_; // inventory penalty parameter

  // Pre-computed constants for performance optimization
  double gamma_sigma_sq_; // gamma * sigma^2
  double log_constant_;   // log(1 + gamma/kappa)
  double gamma_inv_;      // 2.0 / gamma

  void update_precomputed_constants();
  double calculate_optimal_spread(double mid_price, double volatility,
                                  double inventory) const;
  double calculate_inventory_bias(double inventory) const;
  double calculate_reservation_price(double mid_price, double inventory) const;
};

} // namespace mm