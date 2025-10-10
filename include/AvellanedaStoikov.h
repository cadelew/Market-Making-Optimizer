#pragma once
#include <string>
#include "MarketData.h"
#include "Quote.h"

namespace mm {

class AvellanedaStoikov {
public:
    AvellanedaStoikov();
    
    Quote calculate_quotes(const MarketTick& tick, double inventory);
    
    void set_risk_aversion(double gamma);
    void set_volatility(double sigma);
    void set_time_horizon(double T);
    void set_inventory_penalty(double kappa);
    
    double get_risk_aversion() const;
    double get_volatility() const;
    double get_time_horizon() const;
    double get_inventory_penalty() const;
    
private:
    double gamma_;      // risk aversion parameter
    double sigma_;      // volatility
    double T_;          // time horizon (seconds)
    double kappa_;      // inventory penalty parameter
    
    double calculate_optimal_spread(double mid_price, double volatility, double inventory) const;
    double calculate_inventory_bias(double inventory) const;
    double calculate_reservation_price(double mid_price, double inventory) const;
};

} // namespace mm