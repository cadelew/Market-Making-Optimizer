// Avellaneda-Stoikov Market Making Algorithm Implementation
#include "AvellanedaStoikov.h"
#include "LatencyBenchmark.h"
#include "MarketData.h"
#include <cmath>
// Optimized implementation with pre-computed constants and batch processing

namespace mm {

AvellanedaStoikov::AvellanedaStoikov() {
  gamma_ = 0.1;
  sigma_ = 0.05; // Increased from 0.02 to 0.05 (5% - more realistic for crypto)
  T_ = 60.0;
  kappa_ = 1.5;

  // Pre-compute expensive constants for performance
  update_precomputed_constants();
}

void AvellanedaStoikov::set_risk_aversion(double gamma) {
  gamma_ = gamma;
  update_precomputed_constants();
}
void AvellanedaStoikov::set_volatility(double sigma) {
  sigma_ = sigma;
  update_precomputed_constants();
}
void AvellanedaStoikov::set_time_horizon(double T) {
  T_ = T;
  update_precomputed_constants();
}
void AvellanedaStoikov::set_inventory_penalty(double kappa) {
  kappa_ = kappa;
  update_precomputed_constants();
}
double AvellanedaStoikov::get_risk_aversion() const { return gamma_; }
double AvellanedaStoikov::get_volatility() const { return sigma_; }
double AvellanedaStoikov::get_time_horizon() const { return T_; }
double AvellanedaStoikov::get_inventory_penalty() const { return kappa_; }

void AvellanedaStoikov::update_precomputed_constants() {
  // Pre-compute expensive constants for faster quote generation
  gamma_sigma_sq_ = gamma_ * sigma_ * sigma_;
  log_constant_ = std::log(1.0 + gamma_ / kappa_);
  gamma_inv_ = 2.0 / gamma_;
}

double AvellanedaStoikov::calculate_reservation_price(double mid_price,
                                                      double inventory) const {
  return mid_price - inventory * gamma_sigma_sq_ * T_;
}

double AvellanedaStoikov::calculate_optimal_spread(double mid_price,
                                                   double volatility,
                                                   double inventory) const {

  double vol = (volatility > 0) ? volatility : sigma_;

  // Use pre-computed constants for faster calculation
  return gamma_ * vol * vol * T_ + gamma_inv_ * log_constant_;
}

Quote AvellanedaStoikov::calculate_quotes(const MarketTick &tick,
                                          double inventory) {
  BENCHMARK_SCOPE("AvellanedaStoikov::calculate_quotes");

  double mid_price = tick.mid_price();

  // 2. Calculate reservation price
  double reservation_price = calculate_reservation_price(mid_price, inventory);

  // 3. Calculate optimal spread
  double optimal_spread =
      calculate_optimal_spread(mid_price, tick.volatility, inventory);

  // 4. Calculate bid and ask
  double bid_price = reservation_price - (optimal_spread / 2.0);
  double ask_price = reservation_price + (optimal_spread / 2.0);

  // Set quote sizes
  double bid_size = 1.0;
  double ask_size = 1.0;

  // 6. Return Quote object
  return Quote(tick.symbol, bid_price, ask_price, bid_size, ask_size, 0);
}

// Batch quote calculation for multiple ticks
// Uses optimized loop-based approach - already very fast!
std::vector<Quote> AvellanedaStoikov::calculate_quotes_batch(
    const std::vector<MarketTick> &ticks,
    const std::vector<double> &inventories) {
  BENCHMARK_SCOPE("AvellanedaStoikov::calculate_quotes_batch");

  size_t n = ticks.size();
  if (n != inventories.size()) {
    throw std::invalid_argument("Ticks and inventories must have same size");
  }

  // Use optimized loop-based approach with pre-computed constants
  std::vector<Quote> quotes;
  quotes.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    // Use the existing optimized single-tick calculation
    quotes.push_back(calculate_quotes(ticks[i], inventories[i]));
  }

  return quotes;
}

} // namespace mm
