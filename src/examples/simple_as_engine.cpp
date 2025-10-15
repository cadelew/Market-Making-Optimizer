#define NOMINMAX // Prevent Windows min/max macros
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <libwebsockets.h>
#include <random>
#include <sstream>
#include <string>

// Core library includes
#include "AvellanedaStoikov.h"
#include "Fill.h"
#include "MarketData.h"
#include "PnLTracker.h"
#include "Position.h"
#include "Quote.h"
#include "Symbol.h"

using namespace mm;

// Generate a simple simulation ID
std::string generateSimulationId() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream oss;
  oss << "sim_" << std::put_time(std::gmtime(&time_t), "%Y%m%d_%H%M%S");
  oss << "_" << ms.count();
  return oss.str();
}

struct TradingData {
  std::string symbol;
  std::string simulation_id;
  double bid;
  double ask;
  double bid_qty;
  double ask_qty;
  int count = 0;
  std::chrono::steady_clock::time_point start_time;
  std::atomic<bool> should_stop{false};
  int duration_seconds = 120;

  // Core library components
  AvellanedaStoikov engine;
  PnLTracker pnl_tracker;
  int quote_count = 0;
  int fill_count = 0;

  // Volatility calculation
  std::vector<double> price_history;
  static const int VOLATILITY_WINDOW =
      50; // Reduced from 100 to 50 for faster adaptation
};

// Write market ticks to database
void writeMarketTickToDatabase(const TradingData *data) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

  double spread = data->ask - data->bid;
  double mid_price = (data->bid + data->ask) / 2.0;

  std::ostringstream cmd;
  cmd << std::fixed << std::setprecision(8);
  cmd << "docker exec -i timescaledb psql -U postgres -d postgres -c \""
      << "INSERT INTO market_ticks (time, symbol, bid, bid_size, ask, "
         "ask_size, spread, mid_price, simulation_id) "
      << "VALUES ('" << timestamp.str() << "', '" << data->symbol << "', "
      << data->bid << ", " << data->bid_qty << ", " << data->ask << ", "
      << data->ask_qty << ", " << spread << ", " << mid_price << ", '"
      << data->simulation_id << "');\" > nul 2>&1";

  system(cmd.str().c_str());
}

// Write A-S quotes to database
void writeQuoteToDatabase(const TradingData *data, const Quote &quote) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

  double mid_price = (data->bid + data->ask) / 2.0;

  // Get position from PnLTracker
  Position position = data->pnl_tracker.get_position(data->symbol);
  double avg_entry = position.quantity != 0 ? position.average_price : 0.0;

  std::ostringstream cmd;
  cmd << std::fixed << std::setprecision(8);
  cmd << "docker exec -i timescaledb psql -U postgres -d postgres -c \""
      << "INSERT INTO as_quotes (time, symbol, our_bid, our_ask, our_spread, "
         "spread_bps, market_mid, position, avg_entry_price, volatility, "
         "simulation_id) "
      << "VALUES ('" << timestamp.str() << "', '" << data->symbol << "', "
      << quote.bid_price << ", " << quote.ask_price << ", " << quote.spread()
      << ", " << quote.spread_bps() << ", " << mid_price << ", "
      << position.quantity << ", " << avg_entry << ", "
      << data->engine.get_volatility() << ", '" << data->simulation_id
      << "');\" > nul 2>&1";
  system(cmd.str().c_str());
}

// Write trading stats to database
void writeTradingStatsToDatabase(const TradingData *data) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

  Position position = data->pnl_tracker.get_position(data->symbol);
  double avg_entry = position.quantity != 0 ? position.average_price : 0.0;
  double fill_rate = (data->quote_count > 0)
                         ? (100.0 * data->fill_count / data->quote_count)
                         : 0.0;
  double total_pnl = data->pnl_tracker.get_total_pnl();

  std::ostringstream cmd;
  cmd << std::fixed << std::setprecision(8);
  cmd << "docker exec -i timescaledb psql -U postgres -d postgres -c \""
      << "INSERT INTO trading_stats (time, symbol, position, avg_entry_price, "
         "realized_pnl, unrealized_pnl, total_pnl, fill_count, quote_count, "
         "fill_rate, simulation_id) "
      << "VALUES ('" << timestamp.str() << "', '" << data->symbol << "', "
      << position.quantity << ", " << avg_entry << ", "
      << data->pnl_tracker.get_realized_pnl() << ", "
      << data->pnl_tracker.get_unrealized_pnl() << ", " << total_pnl << ", "
      << data->fill_count << ", " << data->quote_count << ", " << fill_rate
      << ", '" << data->simulation_id << "');\" > nul 2>&1";
  system(cmd.str().c_str());
}

// Create a new simulation session
void createSimulationSession(const TradingData *data, int duration_seconds) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

  // Create simple string for algorithm parameters
  std::ostringstream params_str;
  params_str << "gamma=" << data->engine.get_risk_aversion()
             << ",sigma=" << data->engine.get_volatility()
             << ",T=" << data->engine.get_time_horizon()
             << ",kappa=" << data->engine.get_inventory_penalty();

  std::ostringstream cmd;
  cmd << "docker exec -i timescaledb psql -U postgres -d postgres -c \""
      << "INSERT INTO simulation_sessions (simulation_id, start_time, "
         "duration_seconds, "
         "symbol, algorithm_params, status) "
      << "VALUES ('" << data->simulation_id << "', '" << timestamp.str()
      << "', " << duration_seconds << ", '" << data->symbol << "', '"
      << params_str.str() << "', 'running');\" > nul 2>&1";

  system(cmd.str().c_str());
}

// Update simulation session when completed
void updateSimulationSession(const TradingData *data,
                             const std::string &status) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

  Position position = data->pnl_tracker.get_position(data->symbol);

  // Create simple string for final stats
  std::ostringstream stats_str;
  stats_str << "total_pnl=" << data->pnl_tracker.get_total_pnl()
            << ",realized_pnl=" << data->pnl_tracker.get_realized_pnl()
            << ",unrealized_pnl=" << data->pnl_tracker.get_unrealized_pnl()
            << ",fill_count=" << data->fill_count
            << ",quote_count=" << data->quote_count
            << ",final_position=" << position.quantity;

  std::ostringstream cmd;
  cmd << "docker exec -i timescaledb psql -U postgres -d postgres -c \""
      << "UPDATE simulation_sessions SET end_time='" << timestamp.str()
      << "', final_stats='" << stats_str.str() << "', status='" << status
      << "' WHERE simulation_id='" << data->simulation_id << "';\" > nul 2>&1";
  system(cmd.str().c_str());
}

// Calculate volatility from price history
double calculateVolatility(const std::vector<double> &prices) {
  if (prices.size() < 2)
    return 0.02; // default

  std::vector<double> returns;
  for (size_t i = 1; i < prices.size(); i++) {
    returns.push_back(std::log(prices[i] / prices[i - 1]));
  }

  double mean = 0.0;
  for (double r : returns)
    mean += r;
  mean /= returns.size();

  double variance = 0.0;
  for (double r : returns) {
    variance += std::pow(r - mean, 2);
  }
  variance /= returns.size();

  return std::sqrt(variance) *
         std::sqrt(252.0 * 24.0 * 60.0 * 60.0); // annualized
}

static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
  auto *data = (TradingData *)lws_context_user(lws_get_context(wsi));

  switch (reason) {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    std::cout << "✅ Connected to Binance!" << std::endl;
    std::cout << "Starting A-S Market Making Engine..." << std::endl;
    std::cout << "\nAlgorithm Parameters:" << std::endl;
    std::cout << "  Risk Aversion (gamma): " << data->engine.get_risk_aversion()
              << std::endl;
    std::cout << "  Volatility (sigma): " << data->engine.get_volatility()
              << std::endl;
    std::cout << "  Time Horizon (T): " << data->engine.get_time_horizon()
              << " seconds" << std::endl;
    std::cout << "  Inventory Penalty (kappa): "
              << data->engine.get_inventory_penalty() << std::endl;
    std::cout << "\n" << std::endl;

    // Create simulation session in database
    createSimulationSession(data, data->duration_seconds);
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE: {
    std::string message((char *)in, len);
    data->count++;

    // Parse JSON
    size_t s_pos = message.find("\"s\":\"");
    if (s_pos != std::string::npos) {
      s_pos += 5;
      size_t s_end = message.find("\"", s_pos);
      data->symbol = message.substr(s_pos, s_end - s_pos);
    }

    size_t b_pos = message.find("\"b\":\"");
    if (b_pos != std::string::npos) {
      b_pos += 5;
      size_t b_end = message.find("\"", b_pos);
      data->bid = std::stod(message.substr(b_pos, b_end - b_pos));
    }

    size_t a_pos = message.find("\"a\":\"");
    if (a_pos != std::string::npos) {
      a_pos += 5;
      size_t a_end = message.find("\"", a_pos);
      data->ask = std::stod(message.substr(a_pos, a_end - a_pos));
    }

    size_t B_pos = message.find("\"B\":\"");
    if (B_pos != std::string::npos) {
      B_pos += 5;
      size_t B_end = message.find("\"", B_pos);
      data->bid_qty = std::stod(message.substr(B_pos, B_end - B_pos));
    }

    size_t A_pos = message.find("\"A\":\"");
    if (A_pos != std::string::npos) {
      A_pos += 5;
      size_t A_end = message.find("\"", A_pos);
      data->ask_qty = std::stod(message.substr(A_pos, A_end - A_pos));
    }

    // Add price to history for volatility calculation
    double mid_price = (data->bid + data->ask) / 2.0;
    data->price_history.push_back(mid_price);
    if (data->price_history.size() > TradingData::VOLATILITY_WINDOW) {
      data->price_history.erase(data->price_history.begin());
    }

    // Update volatility every 20 ticks (more responsive)
    if (data->count % 20 == 0 && data->count >= 20) {
      double volatility = calculateVolatility(data->price_history);
      if (volatility > 0.0) {
        data->engine.set_volatility(volatility);
      }
    }

    // Generate quotes every 10 ticks using A-S algorithm
    if (data->count % 10 == 0) {
      // Create MarketTick object for core library
      MarketTick tick(data->symbol, data->bid, data->ask, 0.0,
                      data->engine.get_volatility());

      // Get current position
      Position position = data->pnl_tracker.get_position(data->symbol);

      // Calculate quotes using core A-S engine
      Quote quote = data->engine.calculate_quotes(tick, position.quantity);

      if (quote.is_valid()) {
        data->quote_count++;

        // REALISTIC MARKET MAKING FILL SIMULATION
        double random_val = (double)rand() / RAND_MAX;

        // Check if our quotes are competitive (within 0.1% of best bid/ask)
        bool bid_competitive =
            std::abs(quote.bid_price - data->bid) / data->bid < 0.001;
        bool ask_competitive =
            std::abs(quote.ask_price - data->ask) / data->ask < 0.001;

        // Simulate passive maker fills (5% probability)
        double fill_quantity = 0.01; // 0.01 BTC fills

        if (bid_competitive && random_val < 0.05) {
          // PASSIVE BID FILL: Someone sold to us at our bid price
          double maker_rebate =
              quote.bid_price * fill_quantity * 0.0001; // -0.01%

          // Create Fill object for core library
          Fill fill(data->symbol, true, quote.bid_price, fill_quantity,
                    quote.order_id, -maker_rebate); // Negative fee = rebate

          // Update P&L tracker with the fill
          data->pnl_tracker.update_fill(fill);
          data->fill_count++;
        }

        if (ask_competitive && random_val > 0.95) {
          // PASSIVE ASK FILL: Someone bought from us at our ask price
          double maker_rebate =
              quote.ask_price * fill_quantity * 0.0001; // -0.01%

          // Create Fill object for core library
          Fill fill(data->symbol, false, quote.ask_price, fill_quantity,
                    quote.order_id, -maker_rebate); // Negative fee = rebate

          // Update P&L tracker with the fill
          data->pnl_tracker.update_fill(fill);
          data->fill_count++;
        }

        // Update unrealized P&L with current market price
        data->pnl_tracker.update_market_price(data->symbol, mid_price);
      }
    }

    // Write to database every 10 ticks
    if (data->count % 10 == 0) {
      writeMarketTickToDatabase(data);

      // Also write quote and trading stats if we generated a quote
      if (data->quote_count > 0) {
        MarketTick tick(data->symbol, data->bid, data->ask, 0.0,
                        data->engine.get_volatility());
        Position position = data->pnl_tracker.get_position(data->symbol);
        Quote quote = data->engine.calculate_quotes(tick, position.quantity);
        writeQuoteToDatabase(data, quote);
        writeTradingStatsToDatabase(data);
      }
    }

    if (data->count % 100 == 0) {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(
          now - data->start_time);

      double mid_price = (data->bid + data->ask) / 2.0;
      Position position = data->pnl_tracker.get_position(data->symbol);
      MarketTick tick(data->symbol, data->bid, data->ask, 0.0,
                      data->engine.get_volatility());
      Quote quote = data->engine.calculate_quotes(tick, position.quantity);

      std::cout << "\n=== A-S Algorithm Status (t=" << duration.count()
                << "s) ===" << std::endl;
      std::cout << "Market: " << data->symbol << " Mid: $" << std::fixed
                << std::setprecision(2) << mid_price << std::endl;
      std::cout << "Our Quotes: Bid: $" << std::fixed << std::setprecision(2)
                << quote.bid_price << " Ask: $" << std::fixed
                << std::setprecision(2) << quote.ask_price << " Spread: $"
                << std::fixed << std::setprecision(2) << quote.spread() << " ("
                << std::fixed << std::setprecision(2) << quote.spread_bps()
                << " bps)" << std::endl;
      std::cout << "Position: " << position.quantity << " BTC";
      if (position.quantity != 0) {
        std::cout << " (Avg Entry: $" << std::fixed << std::setprecision(2)
                  << position.average_price << ")";
      }
      std::cout << std::endl;
      std::cout << "P&L: $" << std::fixed << std::setprecision(2)
                << data->pnl_tracker.get_total_pnl() << " (Realized: $"
                << std::fixed << std::setprecision(2)
                << data->pnl_tracker.get_realized_pnl() << ", Unrealized: $"
                << std::fixed << std::setprecision(2)
                << data->pnl_tracker.get_unrealized_pnl() << ")" << std::endl;
      std::cout << "Fills: " << data->fill_count
                << " / Quotes: " << data->quote_count;
      if (data->quote_count > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1)
                  << (100.0 * data->fill_count / data->quote_count) << "%)";
      }
      std::cout << " | Ticks: " << data->count << std::endl;
      std::cout << "Volatility: " << data->engine.get_volatility()
                << " [DB WRITTEN]" << std::endl;
    }

    // Duration check is handled in main loop to avoid race conditions
  } break;

  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
    std::cout << "❌ Connection error: " << (in ? (char *)in : "unknown")
              << std::endl;
    auto error_time = std::chrono::steady_clock::now();
    auto error_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        error_time - data->start_time);
    std::cout << "🔍 Connection error at " << error_elapsed.count()
              << "s (duration was " << data->duration_seconds << "s)"
              << std::endl;
    data->should_stop = true;
    break;
  }

  case LWS_CALLBACK_CLOSED: {
    std::cout << "Disconnected" << std::endl;
    auto close_time = std::chrono::steady_clock::now();
    auto close_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        close_time - data->start_time);
    std::cout << "🔍 Connection closed at " << close_elapsed.count()
              << "s (duration was " << data->duration_seconds << "s)"
              << std::endl;
    data->should_stop = true;
    break;
  }

  default:
    break;
  }

  return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "binance-protocol",
        callback_websocket,
        0,
        4096,
    },
    {NULL, NULL, 0, 0} /* terminator */
};

int main(int argc, char *argv[]) {
  std::cout << "=== Simple A-S Market Making Engine ===" << std::endl;

  // Parse command line arguments
  int duration_seconds = 120; // Default 2 minutes
  if (argc > 1) {
    try {
      duration_seconds = std::stoi(argv[1]);
      if (duration_seconds <= 0) {
        std::cerr << "❌ Duration must be positive. Using default 120 seconds."
                  << std::endl;
        duration_seconds = 120;
      }
    } catch (const std::exception &e) {
      std::cerr << "❌ Invalid duration argument. Using default 120 seconds."
                << std::endl;
      duration_seconds = 120;
    }
  }

  std::cout << "Simulation Duration: " << duration_seconds << " seconds"
            << std::endl;
  std::cout << "Connecting to Binance for BTCUSDT..." << std::endl;

  TradingData trading_data;
  trading_data.symbol = "BTCUSDT";
  trading_data.simulation_id = generateSimulationId();
  trading_data.start_time = std::chrono::steady_clock::now();
  trading_data.duration_seconds = duration_seconds;

  std::cout << "Simulation ID: " << trading_data.simulation_id << std::endl;

  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.user = &trading_data;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

  struct lws_context *context = lws_create_context(&info);
  if (!context) {
    std::cerr << "❌ Failed to create libwebsockets context" << std::endl;
    return 1;
  }

  struct lws_client_connect_info ccinfo;
  memset(&ccinfo, 0, sizeof(ccinfo));
  ccinfo.context = context;
  ccinfo.address = "data-stream.binance.vision";
  ccinfo.port = 443;
  ccinfo.path = "/ws/btcusdt@bookTicker";
  ccinfo.host = "data-stream.binance.vision";
  ccinfo.origin = "data-stream.binance.vision";
  ccinfo.protocol = "wss";
  ccinfo.ssl_connection =
      LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
  ccinfo.local_protocol_name = "ws";
  ccinfo.pwsi = nullptr;

  struct lws *wsi = lws_client_connect_via_info(&ccinfo);
  if (!wsi) {
    std::cerr << "❌ Failed to connect to Binance" << std::endl;
    lws_context_destroy(context);
    return 1;
  }

  while (!trading_data.should_stop) {
    lws_service(context, 50);

    // Check if simulation duration has elapsed
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        current_time - trading_data.start_time);

    if (elapsed.count() >= trading_data.duration_seconds) {
      std::cout << "\n⏰ Simulation duration (" << trading_data.duration_seconds
                << "s) reached. Stopping..." << std::endl;
      std::cout << "🔍 Actual elapsed time: " << elapsed.count() << "s"
                << std::endl;
      trading_data.should_stop = true;
      updateSimulationSession(&trading_data, "completed");
    }
  }

  // Print final summary
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      end_time - trading_data.start_time);

  std::cout << "\n=== A-S Algorithm Test Complete ===" << std::endl;
  std::cout << "Total ticks received: " << trading_data.count << std::endl;
  std::cout << "Total quotes generated: " << trading_data.quote_count
            << std::endl;
  std::cout << "Total fills simulated: " << trading_data.fill_count
            << std::endl;
  std::cout << "Duration: " << duration.count() << " seconds" << std::endl;
  if (duration.count() > 0) {
    std::cout << "Ticks per second: " << (trading_data.count / duration.count())
              << std::endl;
  }

  if (trading_data.quote_count > 0) {
    std::cout << "Fill Rate: "
              << (100.0 * trading_data.fill_count / trading_data.quote_count)
              << "%" << std::endl;
  }

  std::cout << "Realized P&L: $" << trading_data.pnl_tracker.get_realized_pnl()
            << std::endl;
  std::cout << "Unrealized P&L: $"
            << trading_data.pnl_tracker.get_unrealized_pnl() << std::endl;
  std::cout << "Total P&L: $" << trading_data.pnl_tracker.get_total_pnl()
            << std::endl;

  double total_pnl = trading_data.pnl_tracker.get_total_pnl();
  if (total_pnl > 0) {
    std::cout << "✅ PROFITABLE! Made $" << total_pnl << std::endl;
  } else if (total_pnl < 0) {
    std::cout << "❌ LOSS: $" << total_pnl << std::endl;
  } else {
    std::cout << "⚖️  Break even" << std::endl;
  }

  lws_context_destroy(context);

  return 0;
}