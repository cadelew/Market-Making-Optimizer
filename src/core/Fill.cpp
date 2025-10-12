#include "Fill.h"
#include <string>

namespace mm {
double Fill::get_notional_value() const { return price * size; }

double Fill::get_net_amount() const {
  if (is_buy) {
    return -(get_notional_value() + fees);
  } else {
    return get_notional_value() - fees;
  }
}

double Fill::get_fee_rate() const {

  double notional = get_notional_value();
  if (notional > 0) {
    return fees / notional;
  } else {
    return 0.0;
  }
}

bool Fill::is_valid() const {
  return (!symbol.empty() && price > 0 && size > 0 && order_id > 0 &&
          fees >= 0);
}

std::string Fill::to_string() const {
  return "Fill{" + symbol + " " + get_side() + " " + std::to_string(size) +
         "@" + std::to_string(price) + " id:" + std::to_string(order_id) + "}";
}

double Fill::get_slippage_bps(double reference_price) const {
  if (reference_price <= 0)
    return 0.0;
  return std::abs(price - reference_price) / reference_price * 10000;
}

double Fill::get_effective_spread(double reference_price) const {
  return std::abs(price - reference_price) * 2;
}

} // namespace mm