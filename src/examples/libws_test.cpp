#include <atomic>
#include <chrono>
#include <iostream>
#include <libwebsockets.h>
#include <string>

struct TickData {
  std::string symbol;
  double bid;
  double ask;
  double bid_qty;
  double ask_qty;
  int count = 0;
  std::chrono::steady_clock::time_point start_time;
  std::atomic<bool> should_stop{false};
};

static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
  auto *tick_data = (TickData *)lws_context_user(lws_get_context(wsi));

  switch (reason) {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    std::cout << "✅ Connected to Binance!" << std::endl;
    std::cout << "Receiving market data..." << std::endl;
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE: {
    std::string message((char *)in, len);
    tick_data->count++;

    // Simple JSON parsing
    size_t s_pos = message.find("\"s\":\"");
    if (s_pos != std::string::npos) {
      s_pos += 5;
      size_t s_end = message.find("\"", s_pos);
      tick_data->symbol = message.substr(s_pos, s_end - s_pos);
    }

    size_t b_pos = message.find("\"b\":\"");
    if (b_pos != std::string::npos) {
      b_pos += 5;
      size_t b_end = message.find("\"", b_pos);
      tick_data->bid = std::stod(message.substr(b_pos, b_end - b_pos));
    }

    size_t a_pos = message.find("\"a\":\"");
    if (a_pos != std::string::npos) {
      a_pos += 5;
      size_t a_end = message.find("\"", a_pos);
      tick_data->ask = std::stod(message.substr(a_pos, a_end - a_pos));
    }

    if (tick_data->count % 100 == 0) {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(
          now - tick_data->start_time);

      std::cout << "Tick #" << tick_data->count << " - " << tick_data->symbol
                << " Bid: $" << tick_data->bid << " Ask: $" << tick_data->ask
                << " Spread: $" << (tick_data->ask - tick_data->bid) << " ("
                << duration.count() << "s)" << std::endl;
    }

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                         tick_data->start_time)
            .count() >= 30) {
      tick_data->should_stop = true;
      lws_cancel_service(lws_get_context(wsi));
    }
  } break;

  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    std::cout << "❌ Connection error: " << (in ? (char *)in : "unknown")
              << std::endl;
    tick_data->should_stop = true;
    break;

  case LWS_CALLBACK_CLOSED:
    std::cout << "Disconnected" << std::endl;
    tick_data->should_stop = true;
    break;

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

int main() {
  std::cout << "=== libwebsockets Binance Client Test ===" << std::endl;

  TickData tick_data;
  tick_data.start_time = std::chrono::steady_clock::now();

  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.user = &tick_data;

  struct lws_context *context = lws_create_context(&info);
  if (!context) {
    std::cerr << "Failed to create context" << std::endl;
    return 1;
  }

  struct lws_client_connect_info ccinfo;
  memset(&ccinfo, 0, sizeof(ccinfo));

  ccinfo.context = context;
  ccinfo.address = "data-stream.binance.vision"; // Try the data-only endpoint
  ccinfo.port = 443;
  ccinfo.path = "/ws/btcusdt@bookTicker";
  ccinfo.host = "data-stream.binance.vision";
  ccinfo.origin = "https://www.binance.com";
  ccinfo.protocol = protocols[0].name;
  ccinfo.ssl_connection = LCCSCF_USE_SSL;

  struct lws *wsi = lws_client_connect_via_info(&ccinfo);
  if (!wsi) {
    std::cerr << "Failed to connect" << std::endl;
    lws_context_destroy(context);
    return 1;
  }

  // Event loop
  while (!tick_data.should_stop) {
    lws_service(context, 50);
  }

  // Print summary
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      end_time - tick_data.start_time);

  std::cout << "\n=== Test Complete ===" << std::endl;
  std::cout << "Total ticks received: " << tick_data.count << std::endl;
  std::cout << "Duration: " << duration.count() << " seconds" << std::endl;
  if (duration.count() > 0) {
    std::cout << "Ticks per second: " << (tick_data.count / duration.count())
              << std::endl;
  }

  lws_context_destroy(context);

  return 0;
}
