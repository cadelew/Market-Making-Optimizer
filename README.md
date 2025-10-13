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

### Core Engine
- **C++17** - High-performance computational engine
- **CMake** - Build system
- **xtensor** - Linear algebra and numerical computing
- **Clangd** - Language server for development

### API & Backend
- **FastAPI** - Python web framework
- **PostgreSQL** - Primary database
- **SQLAlchemy** - ORM for database operations
- **libwebsockets** - Real-time data streaming

### Frontend
- **React** - User interface framework
- **Chart.js/D3.js** - Data visualization
- **Material-UI** - UI components

### Data Sources
- **Binance API** - Live cryptocurrency data
- **Historical Data** - OHLCV time series
- **WebSocket Streams** - Real-time market feeds



MIT License - See LICENSE file for details.
