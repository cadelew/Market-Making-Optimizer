// TODO: Implement the Avellaneda-Stoikov algorithm
#include "AvellanedaStoikov.h"
#include "LatencyBenchmark.h"
#include "MarketData.h"
#include <cmath>


namespace mm {

AvellanedaStoikov::AvellanedaStoikov() {
  gamma_ = 0.1;
  sigma_ = 0.02;
  T_ = 60.0;
  kappa_ = 1.5;
}

void AvellanedaStoikov::set_risk_aversion(double gamma) { gamma_ = gamma; }
void AvellanedaStoikov::set_volatility(double sigma) { sigma_ = sigma; }
void AvellanedaStoikov::set_time_horizon(double T) { T_ = T; }
void AvellanedaStoikov::set_inventory_penalty(double kappa) { kappa_ = kappa; }
double AvellanedaStoikov::get_risk_aversion() const { return gamma_; }
double AvellanedaStoikov::get_volatility() const { return sigma_; }
double AvellanedaStoikov::get_time_horizon() const { return T_; }
double AvellanedaStoikov::get_inventory_penalty() const { return kappa_; }

double AvellanedaStoikov::calculate_reservation_price(double mid_price,
                                                      double inventory) const {
  return mid_price - inventory * gamma_ * sigma_ * sigma_ * T_;
}

double AvellanedaStoikov::calculate_optimal_spread(double mid_price,
                                                   double volatility,
                                                   double inventory) const {

  double vol = (volatility > 0) ? volatility : sigma_;

  return gamma_ * vol * vol * T_ +
         (2.0 / gamma_) * std::log(1.0 + gamma_ / kappa_);
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

  // 5. Set reasonable quote sizes (start simple, can optimize later)
  double bid_size = 1.0; // Try 0.1 or 1.0
  double ask_size = 1.0; // Same as bid_size

  // 6. Return Quote object
  return Quote(tick.symbol, bid_price, ask_price, bid_size, ask_size, 0);
}

} // namespace mm
