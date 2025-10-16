# Market-Making Optimizer

A high-performance C++ market-making system implementing the Avellaneda-Stoikov algorithm with real-time optimization and risk management.

## Performance Metrics

- **Quote Generation Latency**: 10.97μs
- **Theoretical Throughput**: 91,199 quotes/second
- **End-to-End Latency**: ~16-20ms (production mode)
- **Database Efficiency**: 90% reduction in write operations via batching

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Binance       │    │   C++ Engine     │    │   PostgreSQL    │
│   WebSocket     │───▶│                 │───▶│   + TimescaleDB │
│   Data Stream   │    │                  │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │                        │
                                ▼                        ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │   FastAPI        │    │   Web Dashboard │
                       │   Backend        │◀───│   (Real-time)   │
                       └──────────────────┘    └─────────────────┘
```

## Key Features

### Core Algorithm
- **Avellaneda-Stoikov Implementation**: Optimized market-making strategy
- **Pre-computed Constants**: 15% faster quote generation through mathematical optimization
- **Live EWMA Volatility**: Real-time volatility estimation with minimum floor
- **Risk Controls**: Inventory limits, spread widening, P&L kill switches

### Real-time Dashboard
![Dashboard Overview](screenshots/Screenshot%202025-10-15%20161858.png)
*Real-time trading dashboard showing live market data, order book, and performance metrics*

### Performance Optimizations
- **Database Batching**: 50-record batches reduce I/O overhead by 90%
- **Fast JSON Parser**: Custom parser with validation (10/10 accuracy)
- **Compiler Optimizations**: AVX2, /O2 flags for maximum performance
- **Memory Management**: Efficient data structures and object pooling

### Real-time Monitoring
- **Latency Tracking**: Comprehensive percentile analysis (P50, P90, P95, P99)
- **Performance Metrics**: Quote generation, end-to-end processing times
- **Risk Monitoring**: Position tracking, P&L analysis, fill rates
- **Web Dashboard**: Real-time visualization with TradingView-style charts

![Performance Metrics](screenshots/Screenshot%202025-10-15%20162130.png)
*Live performance monitoring showing latency tracking, quote generation stats, and risk metrics*
![Simulation History](screenshots/Screenshot%202025-10-15%20154429.png)



## Technology Stack

### Backend
- **C++20**: Core trading engine
- **libwebsockets**: Real-time WebSocket connections
- **PostgreSQL + TimescaleDB**: Time-series data storage
- **libpqxx**: Database connectivity
- **OpenBLAS**: Optimized mathematical operations

### Frontend
- **FastAPI**: REST API and WebSocket server
- **Chart.js**: Real-time data visualization
- **HTML5/CSS3/JavaScript**: Modern web interface

### Infrastructure
- **Docker**: Database containerization
- **vcpkg**: C++ package management
- **CMake**: Build system



## Benchmark Results

### Latency Analysis (1,000 sample test)
```
Quote Generation: 10.97μs average

### Throughput Performance
- **Single-threaded**: 91,199 quotes/second theoretical max
- **Real-world**: ~17 ticks/second (network limited)
- **Database**: 90% reduction in write operations

### Production Deployment Notes
- **Network Latency**: ~16-20ms end-to-end in production (vs 51ms in development)
- **Database**: Direct connection eliminates Docker overhead
- **Monitoring**: Built-in latency tracking and performance metrics
- **Risk Management**: Automated inventory limits and P&L kill switches


## Acknowledgments

- **Avellaneda-Stoikov Algorithm**: Mathematical foundation for optimal market-making
- **Binance API**: Real-time market data feed
- **TimescaleDB**: Efficient time-series data storage
- **FastAPI**: Modern Python web framework

---

*Performance metrics achieved on Windows 11, Visual Studio 2022, with optimized compiler flags and real-time market data processing.*
