#pragma once
#include <string>
#include <string_view>

namespace mm {

// Enum for supported trading symbols
enum class Symbol : uint8_t {
  BTC = 0,
  ETH = 1,
  SOL = 2,
  BNB = 3,
  UNKNOWN = 255
};

// Number of valid symbols
constexpr size_t SYMBOL_COUNT = 4;

// Convert string to Symbol enum
inline Symbol string_to_symbol(const std::string_view &symbol_str) {
  if (symbol_str == "BTCUSDT" || symbol_str == "BTC")
    return Symbol::BTC;
  if (symbol_str == "ETHUSDT" || symbol_str == "ETH")
    return Symbol::ETH;
  if (symbol_str == "SOLUSDT" || symbol_str == "SOL")
    return Symbol::SOL;
  if (symbol_str == "BNBUSDT" || symbol_str == "BNB")
    return Symbol::BNB;
  return Symbol::UNKNOWN;
}

// Convert Symbol enum to string
inline std::string symbol_to_string(Symbol sym) {
  switch (sym) {
  case Symbol::BTC:
    return "BTCUSDT";
  case Symbol::ETH:
    return "ETHUSDT";
  case Symbol::SOL:
    return "SOLUSDT";
  case Symbol::BNB:
    return "BNBUSDT";
  default:
    return "UNKNOWN";
  }
}

// Get short name
inline std::string symbol_to_short_string(Symbol sym) {
  switch (sym) {
  case Symbol::BTC:
    return "BTC";
  case Symbol::ETH:
    return "ETH";
  case Symbol::SOL:
    return "SOL";
  case Symbol::BNB:
    return "BNB";
  default:
    return "UNKNOWN";
  }
}

} // namespace mm
