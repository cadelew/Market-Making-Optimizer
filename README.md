# Market Making Optimizer

A quantitative trading system implementing the Avellaneda-Stoikov algorithm for optimal bid/ask spread calculation in cryptocurrency market making.

## 🎯 Project Overview

This project combines high-performance C++ computational engines with modern web technologies to create a comprehensive market making system. The goal is to learn mathematical optimization, real-time data processing, and quantitative finance through hands-on implementation.

## 🏗️ Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   React Frontend │    │  FastAPI/Flask  │    │   C++ Engine    │
│   (Visualization)│◄──►│   (API Layer)   │◄──►│  (A-S Algorithm)│
└─────────────────┘    └─────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌─────────────────┐
                       │   PostgreSQL    │
                       │  (Data Storage) │
                       └─────────────────┘
```

## 📋 Development Phases

### 📘 Phase 1: Offline Simulation
- [x] Set up C++ project structure with CMake
- [x] Implement core data structures (Fill, Quote, Position)
- [ ] Implement Avellaneda-Stoikov algorithm in C++
- [ ] Add adaptive λ parameter adjustment
- [ ] Implement fee adjustment mechanisms
- [ ] Create Python/FastAPI wrapper for C++ engine
- [ ] Set up PostgreSQL database schema
- [ ] Build React dashboard for visualization
- [ ] Store simulation results in database
- [ ] Plot live PnL and spread analytics

### 🔁 Phase 2: Historical Data Replay
- [ ] Download historical OHLCV data (Binance API)
- [ ] Implement tick-by-tick data feeding
- [ ] Validate model behavior with real market volatility
- [ ] Add backtesting framework
- [ ] Performance analysis and optimization
- [ ] Risk metrics calculation

### 🌐 Phase 3: Live Data Integration
- [ ] Integrate Binance WebSocket streams
  - [ ] `@ticker` - Real-time price updates
  - [ ] `@depth` - Order book updates
  - [ ] `@trade` - Trade execution feed
- [ ] Implement dynamic spread updates
- [ ] Add real-time risk monitoring
- [ ] Deploy live trading dashboard
- [ ] Production monitoring and alerting

## 🛠️ Technology Stack

### Core Engine
- **C++17** - High-performance computational engine
- **CMake** - Build system
- **xtensor** - Linear algebra and numerical computing
- **Clangd** - Language server for development

### API & Backend
- **FastAPI/Flask** - Python web framework
- **PostgreSQL** - Primary database
- **SQLAlchemy** - ORM for database operations
- **WebSocket** - Real-time data streaming

### Frontend
- **React** - User interface framework
- **Chart.js/D3.js** - Data visualization
- **Material-UI** - UI components

### Data Sources
- **Binance API** - Live cryptocurrency data
- **Historical Data** - OHLCV time series
- **WebSocket Streams** - Real-time market feeds

## 📁 Project Structure

```
Market-Making-Optimizer/
├── include/                 # C++ header files
│   ├── AvellanedaStoikov.h
│   ├── Fill.h
│   ├── Quote.h
│   ├── Position.h
│   ├── MarketData.h
│   ├── Logger.h
│   └── PnLTracker.h
├── src/
│   ├── core/               # C++ implementation
│   │   ├── AvellanedaStoikov.cpp
│   │   ├── Fill.cpp
│   │   ├── Quote.cpp
│   │   ├── Position.cpp
│   │   ├── MarketData.cpp
│   │   ├── Logger.cpp
│   │   └── PnLTracker.cpp
│   ├── examples/           # Example programs
│   └── bindings/           # Python bindings
├── tests/                  # Unit tests
├── docs/                   # Documentation
├── api/                    # FastAPI application
├── frontend/               # React application
├── database/               # Database schemas and migrations
├── CMakeLists.txt
└── README.md
```

## 🔧 Getting Started

### Prerequisites
- C++17 compatible compiler (Clang, GCC, or MSVC)
- CMake 3.16+
- Python 3.8+
- Node.js 16+
- PostgreSQL 12+

### Building the C++ Engine
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Tests
```bash
cd build
ctest --verbose
```

## 📊 Key Features

### Market Making Algorithm
- **Avellaneda-Stoikov Implementation** - Optimal bid/ask spread calculation
- **Adaptive Parameters** - Dynamic adjustment based on market conditions
- **Risk Management** - Position sizing and inventory control
- **Fee Optimization** - Cost-aware spread adjustment

### Data Management
- **Real-time Processing** - Low-latency market data handling
- **Historical Backtesting** - Strategy validation on past data
- **Performance Analytics** - Comprehensive trading metrics

### Visualization
- **Live Dashboard** - Real-time PnL and spread monitoring
- **Historical Analysis** - Backtesting results visualization
- **Risk Metrics** - Position and exposure tracking

## 🎓 Learning Objectives

This project provides hands-on experience with:
- **Quantitative Finance** - Market making theory and practice
- **Mathematical Optimization** - Stochastic control and optimal stopping
- **High-Performance Computing** - C++ optimization techniques
- **Real-time Systems** - WebSocket integration and data streaming
- **Full-Stack Development** - End-to-end system architecture
- **Database Design** - Time-series data storage and analytics

## 📈 Target Cryptocurrency Pairs

- **Major Pairs**: BTC/USDT, ETH/USDT
- **Altcoins**: ADA/USDT, DOT/USDT, LINK/USDT
- **Cross Rates**: BTC/ETH, ETH/BTC

## 🔒 Risk Management

- **Position Limits** - Maximum inventory thresholds
- **Drawdown Controls** - Stop-loss mechanisms
- **Volatility Adjustments** - Dynamic parameter scaling
- **Fee Considerations** - Maker/taker fee optimization

## 📝 Development Notes

### Current Status
- ✅ Project structure established
- ✅ Core data structures implemented (Fill, Quote, Position)
- ✅ Build system configured
- ✅ Development environment set up with clangd

### Next Steps
1. Implement Avellaneda-Stoikov algorithm core
2. Add Python bindings for C++ engine
3. Set up FastAPI backend
4. Create basic React dashboard

## 🤝 Contributing

This is a personal learning project. Contributions and suggestions are welcome!

## 📚 References

- [Avellaneda & Stoikov (2008) - High-frequency trading in a limit order book](https://www.researchgate.net/publication/2407428)
- [Binance API Documentation](https://binance-docs.github.io/apidocs/)
- [Market Making Strategies](https://www.quantstart.com/articles/)

## 📄 License

MIT License - See LICENSE file for details.