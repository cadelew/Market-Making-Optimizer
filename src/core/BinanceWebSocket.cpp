#include "BinanceWebSocket.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

namespace mm {

BinanceWebSocket::BinanceWebSocket(const std::string& symbol) 
    : symbol_(to_lowercase(symbol))
    , url_("wss://stream.binance.com:9443/ws/" + symbol_ + "@bookTicker")
    , connected_(false)
    , should_stop_(false)
#ifdef _WIN32
    , socket_(INVALID_SOCKET)
#else
    , socket_(-1)
#endif
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

BinanceWebSocket::~BinanceWebSocket() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool BinanceWebSocket::connect() {
    if (connected_) {
        return true;
    }
    
    // Parse URL
    std::string host = "stream.binance.com";
    int port = 9443;
    
    // Create socket
#ifdef _WIN32
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        Logger::error("Failed to create socket");
        return false;
    }
#else
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        Logger::error("Failed to create socket");
        return false;
    }
#endif
    
    // Connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Resolve hostname
    struct hostent* host_entry = gethostbyname(host.c_str());
    if (!host_entry) {
        Logger::error("Failed to resolve hostname: " + host);
        return false;
    }
    
    memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    
    if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::error("Failed to connect to " + host + ":" + std::to_string(port));
        return false;
    }
    
    Logger::info("Connected to Binance WebSocket: " + url_);
    
    // Perform WebSocket handshake
    if (!perform_handshake()) {
        Logger::error("WebSocket handshake failed");
        return false;
    }
    
    connected_ = true;
    should_stop_ = false;
    
    // Start receive thread
    receive_thread_ = std::thread(&BinanceWebSocket::receive_loop, this);
    
    return true;
}

void BinanceWebSocket::disconnect() {
    if (!connected_) {
        return;
    }
    
    should_stop_ = true;
    connected_ = false;
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
#ifdef _WIN32
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
#else
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
#endif
    
    Logger::info("Disconnected from Binance WebSocket");
}

MarketTick BinanceWebSocket::receive_tick() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    // Wait for data
    while (tick_queue_.empty() && connected_) {
        // Simple busy wait - in production you'd use condition_variable
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        lock.lock();
    }
    
    if (!tick_queue_.empty()) {
        MarketTick tick = tick_queue_.front();
        tick_queue_.pop();
        return tick;
    }
    
    // Return dummy tick if disconnected
    return MarketTick(symbol_, 0.0, 0.0, 0.0, 0.0);
}

bool BinanceWebSocket::try_receive_tick(MarketTick& tick) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (!tick_queue_.empty()) {
        tick = tick_queue_.front();
        tick_queue_.pop();
        return true;
    }
    
    return false;
}

std::string BinanceWebSocket::get_status() const {
    if (connected_) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return "Connected - Queue size: " + std::to_string(tick_queue_.size());
    }
    return "Disconnected";
}

std::string BinanceWebSocket::create_websocket_key() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::string key(16, 0);
    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<char>(dis(gen));
    }
    
    return base64_encode(key);
}

std::string BinanceWebSocket::base64_encode(const std::string& input) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t value = 0;
        int bits = 0;
        
        for (int j = 0; j < 3 && i + j < input.size(); ++j) {
            value = (value << 8) | static_cast<unsigned char>(input[i + j]);
            bits += 8;
        }
        
        for (int j = 0; j < 4; ++j) {
            if (j * 6 < bits) {
                result += chars[(value >> (18 - j * 6)) & 0x3F];
            } else {
                result += '=';
            }
        }
    }
    
    return result;
}

bool BinanceWebSocket::perform_handshake() {
    std::string ws_key = create_websocket_key();
    std::string path = "/ws/" + symbol_ + "@bookTicker";
    
    std::stringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: stream.binance.com:9443\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << ws_key << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "Sec-WebSocket-Protocol: chat, superchat\r\n";
    request << "\r\n";
    
    std::string request_str = request.str();
    
    // Send handshake
    int sent = send(socket_, request_str.c_str(), request_str.length(), 0);
    if (sent != request_str.length()) {
        Logger::error("Failed to send WebSocket handshake");
        return false;
    }
    
    // Receive response
    char buffer[4096];
    int received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        Logger::error("Failed to receive handshake response");
        return false;
    }
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    // Check for successful upgrade
    if (response.find("101 Switching Protocols") == std::string::npos) {
        Logger::error("WebSocket handshake failed: " + response.substr(0, 200));
        return false;
    }
    
    Logger::info("WebSocket handshake successful");
    return true;
}

bool BinanceWebSocket::send_frame(const std::string& message) {
    // Simple text frame (unmasked)
    std::vector<uint8_t> frame;
    
    // FIN + opcode (text frame)
    frame.push_back(0x81);
    
    // Payload length
    if (message.length() < 126) {
        frame.push_back(message.length());
    } else if (message.length() < 65536) {
        frame.push_back(126);
        frame.push_back((message.length() >> 8) & 0xFF);
        frame.push_back(message.length() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((message.length() >> (i * 8)) & 0xFF);
        }
    }
    
    // Payload
    frame.insert(frame.end(), message.begin(), message.end());
    
    int sent = send(socket_, (const char*)frame.data(), frame.size(), 0);
    return sent == frame.size();
}

std::string BinanceWebSocket::receive_frame() {
    char buffer[4096];
    int received = recv(socket_, buffer, sizeof(buffer), 0);
    
    if (received <= 0) {
        return "";
    }
    
    // Simple frame parsing (assumes small frames)
    if (received < 2) {
        return "";
    }
    
    uint8_t* data = (uint8_t*)buffer;
    bool fin = (data[0] & 0x80) != 0;
    uint8_t opcode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;
    
    int header_len = 2;
    
    if (payload_len == 126) {
        payload_len = (data[2] << 8) | data[3];
        header_len = 4;
    } else if (payload_len == 127) {
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[2 + i];
        }
        header_len = 10;
    }
    
    if (masked) {
        // Skip mask key (4 bytes)
        header_len += 4;
    }
    
    if (header_len + payload_len > received) {
        Logger::error("Incomplete frame received");
        return "";
    }
    
    std::string payload(data + header_len, data + header_len + payload_len);
    
    if (masked) {
        uint8_t* mask_key = data + header_len - 4;
        for (size_t i = 0; i < payload.length(); ++i) {
            payload[i] ^= mask_key[i % 4];
        }
    }
    
    return payload;
}

MarketTick BinanceWebSocket::parse_book_ticker(const nlohmann::json& data) {
    try {
        std::string symbol = data["s"];
        double bid = std::stod(data["b"].get<std::string>());
        double ask = std::stod(data["a"].get<std::string>());
        double bid_qty = std::stod(data["B"].get<std::string>());
        double ask_qty = std::stod(data["A"].get<std::string>());
        
        // Calculate average quantity
        double avg_qty = (bid_qty + ask_qty) / 2.0;
        
        // For now, set volatility to 0 (could calculate from recent price changes)
        double volatility = 0.0;
        
        return MarketTick(symbol, bid, ask, avg_qty, volatility);
    } catch (const std::exception& e) {
        Logger::error("Failed to parse book ticker: " + std::string(e.what()));
        return MarketTick(symbol_, 0.0, 0.0, 0.0, 0.0);
    }
}

void BinanceWebSocket::receive_loop() {
    while (!should_stop_ && connected_) {
        std::string frame_data = receive_frame();
        
        if (frame_data.empty()) {
            if (connected_) {
                Logger::error("Failed to receive WebSocket frame");
                connected_ = false;
            }
            break;
        }
        
        try {
            nlohmann::json data = nlohmann::json::parse(frame_data);
            MarketTick tick = parse_book_ticker(data);
            
            // Add to queue
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                tick_queue_.push(tick);
                
                // Limit queue size
                if (tick_queue_.size() > 1000) {
                    tick_queue_.pop();
                }
            }
            
        } catch (const std::exception& e) {
            Logger::error("Failed to parse JSON: " + std::string(e.what()));
        }
    }
}

std::string BinanceWebSocket::to_lowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

} // namespace mm
