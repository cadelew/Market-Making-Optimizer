#!/usr/bin/env python3
"""
Trading Dashboard - FastAPI Web Interface
Displays live trading data from TimescaleDB
"""

from contextlib import asynccontextmanager
from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
import asyncpg
import asyncio
from datetime import datetime, timedelta, timezone
import json
from typing import List, Dict, Optional
import subprocess
import os
import psutil
import signal

# Database connection string
DATABASE_URL = "postgresql://postgres:password@localhost:5432/postgres"

# Global database connection pool
db_pool = None

# Engine process tracking
engine_process = None
ENGINE_PATH = "build/Debug/simple_as_engine.exe"
simulation_duration = 120  # Default duration in seconds
simulation_start_time = None

# WebSocket connection manager
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []
    
    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
    
    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
    
    async def broadcast(self, message: str):
        for connection in self.active_connections:
            try:
                await connection.send_text(message)
            except:
                # Remove dead connections
                if connection in self.active_connections:
                    self.active_connections.remove(connection)

manager = ConnectionManager()

@asynccontextmanager
async def lifespan(app: FastAPI):
    """Manage database connection lifecycle"""
    global db_pool
    # Startup
    db_pool = await asyncpg.create_pool(DATABASE_URL, min_size=1, max_size=10)
    print("[OK] Database connection pool created")
    yield
    # Shutdown
    if db_pool:
        await db_pool.close()
    print("[OK] Database connections closed")

app = FastAPI(title="Trading Dashboard", version="1.0.0", lifespan=lifespan)

# Templates for HTML pages
templates = Jinja2Templates(directory="templates")

async def get_db_connection():
    """Get database connection from pool"""
    return await db_pool.acquire()

async def release_db_connection(conn):
    """Release database connection back to pool"""
    await db_pool.release(conn)

# ============================================
# API ENDPOINTS
# ============================================

@app.get("/", response_class=HTMLResponse)
async def dashboard(request: Request):
    """Main dashboard page"""
    return templates.TemplateResponse("dashboard.html", {"request": request})

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time data streaming"""
    await manager.connect(websocket)
    try:
        while True:
            # Keep connection alive and wait for messages
            data = await websocket.receive_text()
            # Echo back (or handle client messages if needed)
            await websocket.send_text(f"Echo: {data}")
    except WebSocketDisconnect:
        manager.disconnect(websocket)

@app.post("/api/broadcast")
async def broadcast_data(data: dict):
    """Receive data from C++ client and broadcast to WebSocket clients"""
    try:
        # Broadcast to all connected WebSocket clients
        await manager.broadcast(json.dumps(data))
        return {"status": "success", "message": "Data broadcasted"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

@app.get("/api/live-stats")
async def get_live_stats():
    """Get current trading statistics"""
    conn = await get_db_connection()
    try:
        # Get latest market data
        latest_tick = await conn.fetchrow("""
            SELECT time, symbol, bid, ask, spread, mid_price
            FROM market_ticks
            ORDER BY time DESC
            LIMIT 1
        """)
        
        # Get tick count for today
        today = datetime.now().date()
        tick_count = await conn.fetchval("""
            SELECT COUNT(*) 
            FROM market_ticks 
            WHERE DATE(time) = $1
        """, today)
        
        # Get recent tick rate (last 5 minutes)
        five_min_ago = datetime.now() - timedelta(minutes=5)
        recent_ticks = await conn.fetchval("""
            SELECT COUNT(*) 
            FROM market_ticks 
            WHERE time > $1
        """, five_min_ago)
        
        tick_rate = recent_ticks / 5.0 if recent_ticks else 0  # ticks per minute
        
        return {
            "latest_tick": dict(latest_tick) if latest_tick else None,
            "tick_count_today": tick_count,
            "tick_rate_per_minute": round(tick_rate, 2),
            "last_updated": datetime.now().isoformat()
        }
    finally:
        await release_db_connection(conn)

@app.get("/api/recent-ticks")
async def get_recent_ticks(limit: int = 20):
    """Get recent market ticks"""
    conn = await get_db_connection()
    try:
        ticks = await conn.fetch(f"""
            SELECT time, symbol, bid, ask, spread, mid_price
            FROM market_ticks
            ORDER BY time DESC
            LIMIT {limit}
        """)
        
        return [dict(tick) for tick in ticks]
    finally:
        await release_db_connection(conn)

@app.get("/api/market-summary")
async def get_market_summary():
    """Get market summary statistics"""
    conn = await get_db_connection()
    try:
        # Get price range for today
        today = datetime.now().date()
        price_stats = await conn.fetchrow("""
            SELECT 
                MIN(mid_price) as low,
                MAX(mid_price) as high,
                AVG(mid_price) as avg_price,
                STDDEV(mid_price) as volatility
            FROM market_ticks 
            WHERE DATE(time) = $1
        """, today)
        
        # Get spread statistics
        spread_stats = await conn.fetchrow("""
            SELECT 
                AVG(spread) as avg_spread,
                MIN(spread) as min_spread,
                MAX(spread) as max_spread
            FROM market_ticks 
            WHERE DATE(time) = $1
        """, today)
        
        return {
            "price_stats": dict(price_stats) if price_stats else {},
            "spread_stats": dict(spread_stats) if spread_stats else {},
            "date": today.isoformat()
        }
    finally:
        await release_db_connection(conn)

@app.get("/api/performance-data")
async def get_performance_data(simulation_id: str = None):
    """Get performance data for charts - now using tick-by-tick data"""
    conn = await get_db_connection()
    try:
        if simulation_id:
            # Get data for specific simulation
            tick_data = await conn.fetch("""
                SELECT 
                    time,
                    mid_price,
                    bid,
                    ask,
                    spread
                FROM market_ticks
                WHERE simulation_id = $1
                ORDER BY time ASC
            """, simulation_id)
        else:
            # Get data from the latest simulation only - get full simulation duration
            tick_data = await conn.fetch("""
                SELECT 
                    mt.time,
                    mt.mid_price,
                    mt.bid,
                    mt.ask,
                    mt.spread
                FROM market_ticks mt
                JOIN (
                    SELECT simulation_id 
                    FROM simulation_sessions 
                    ORDER BY start_time DESC 
                    LIMIT 1
                ) latest_sim ON mt.simulation_id = latest_sim.simulation_id
                ORDER BY mt.time ASC
                LIMIT 10000
            """)
        
        # Return in chronological order for chart
        return [dict(row) for row in tick_data]
    finally:
        await release_db_connection(conn)

@app.get("/api/orderbook")
async def get_orderbook():
    """Get current order book levels from real market data"""
    conn = await get_db_connection()
    try:
        # Get latest bid/ask with sizes from real market data
        latest_tick = await conn.fetchrow("""
            SELECT bid, ask, bid_size, ask_size, mid_price
            FROM market_ticks
            ORDER BY time DESC
            LIMIT 1
        """)
        
        if not latest_tick:
            return {"bids": [], "asks": []}
        
        # Use real market data to create order book levels
        bid = float(latest_tick['bid'])
        ask = float(latest_tick['ask'])
        bid_size = float(latest_tick['bid_size'])
        ask_size = float(latest_tick['ask_size'])
        
        # Generate realistic order book levels around current market
        bids = []
        asks = []
        
        for i in range(5):
            # Create realistic order book levels with decreasing sizes
            bid_level = bid - (i * 0.5)  # Each level 50 cents lower
            ask_level = ask + (i * 0.5)  # Each level 50 cents higher
            
            # Size decreases with distance from market
            bid_level_size = max(0.001, bid_size * (1.0 - i * 0.2))
            ask_level_size = max(0.001, ask_size * (1.0 - i * 0.2))
            
            bids.append({
                "price": round(bid_level, 2),
                "size": round(bid_level_size, 3)
            })
            asks.append({
                "price": round(ask_level, 2), 
                "size": round(ask_level_size, 3)
            })
        
        return {"bids": bids, "asks": asks}
    finally:
        await release_db_connection(conn)

@app.get("/api/recent-trades")
async def get_recent_trades():
    """Get recent trade executions from market data"""
    conn = await get_db_connection()
    try:
        # Get recent market ticks as simulated trades
        trades = await conn.fetch("""
            SELECT time, bid, ask, mid_price, bid_size, ask_size
            FROM market_ticks
            ORDER BY time DESC
            LIMIT 15
        """)
        
        trade_list = []
        for i, trade in enumerate(trades):
            # Simulate realistic trade sizes based on market depth
            # Alternate between buy/sell to simulate market activity
            side = "buy" if i % 3 == 0 else "sell"
            
            if side == "buy":
                price = float(trade['ask'])  # Buy at ask price
                size = float(trade['ask_size']) * 0.1  # Use 10% of ask size
            else:
                price = float(trade['bid'])  # Sell at bid price  
                size = float(trade['bid_size']) * 0.1  # Use 10% of bid size
            
            # Format time nicely
            trade_time = trade['time'].strftime("%H:%M:%S")
            
            trade_list.append({
                "time": trade_time,
                "side": side,
                "price": round(price, 2),
                "size": round(size, 4)
            })
        
        return trade_list
    finally:
        await release_db_connection(conn)

@app.get("/api/trading-status")
async def get_trading_status_real():
    """Get current trading state from C++ engine data"""
    conn = await get_db_connection()
    try:
        # Get latest market tick for market quotes
        latest_tick = await conn.fetchrow("""
            SELECT bid, ask, mid_price
            FROM market_ticks
            ORDER BY time DESC
            LIMIT 1
        """)
        
        # Get latest A-S quotes
        latest_quote = await conn.fetchrow("""
            SELECT our_bid, our_ask, our_spread, position, avg_entry_price
            FROM as_quotes
            ORDER BY time DESC
            LIMIT 1
        """)
        
        # Get latest trading stats
        latest_stats = await conn.fetchrow("""
            SELECT realized_pnl, unrealized_pnl, total_pnl, 
                   fill_count, quote_count, fill_rate
            FROM trading_stats
            ORDER BY time DESC
            LIMIT 1
        """)
        
        if latest_tick and latest_quote and latest_stats:
            # Real C++ engine data
            position = float(latest_quote['position'])
            realized_pnl = float(latest_stats['realized_pnl'])
            unrealized_pnl = float(latest_stats['unrealized_pnl'])
            total_pnl = float(latest_stats['total_pnl'])
            our_bid = float(latest_quote['our_bid'])
            our_ask = float(latest_quote['our_ask'])
            our_spread = float(latest_quote['our_spread'])
            
            market_bid = float(latest_tick['bid'])
            market_ask = float(latest_tick['ask'])
            
            # Calculate balances based on position
            btc_balance = 0.05 + position
            avg_entry = float(latest_quote['avg_entry_price'] or 0)
            cost_basis = position * avg_entry if position != 0 else 0
            usdt_balance = 5000 - cost_basis
            
            return {
                "position": round(position, 4),
                "realized_pnl": round(realized_pnl, 2),
                "unrealized_pnl": round(unrealized_pnl, 2),
                "total_pnl": round(total_pnl, 2),
                "current_quotes": {"bid": round(our_bid, 2), "ask": round(our_ask, 2)},
                "market_quotes": {"bid": round(market_bid, 2), "ask": round(market_ask, 2)},
                "balances": {"BTC": round(btc_balance, 4), "USDT": round(usdt_balance, 2)},
                "our_spread": round(our_spread, 2),
                "fill_count": int(latest_stats['fill_count']),
                "quote_count": int(latest_stats['quote_count']),
                "fill_rate": round(float(latest_stats['fill_rate']), 2)
            }
        else:
            # No data yet - return empty state
            return {
                "position": 0.0,
                "realized_pnl": 0.0,
                "unrealized_pnl": 0.0,
                "total_pnl": 0.0,
                "current_quotes": {"bid": 0.0, "ask": 0.0},
                "market_quotes": {"bid": 0.0, "ask": 0.0},
                "balances": {"BTC": 0.05, "USDT": 5000.0},
                "our_spread": 0.0,
                "fill_count": 0,
                "quote_count": 0,
                "fill_rate": 0.0
            }
    finally:
        await release_db_connection(conn)

@app.get("/api/system-health")
async def get_system_health():
    """Get system health metrics"""
    conn = await get_db_connection()
    try:
        # Get latest data timestamp to check for freshness
        latest_tick_time = await conn.fetchval("""
            SELECT time FROM market_ticks ORDER BY time DESC LIMIT 1
        """)
        
        # Calculate data lag
        if latest_tick_time:
            now = datetime.now(timezone.utc)
            # Ensure latest_tick_time is timezone-aware
            if latest_tick_time.tzinfo is None:
                latest_tick_time = latest_tick_time.replace(tzinfo=timezone.utc)
            lag_seconds = (now - latest_tick_time).total_seconds()
            data_fresh = lag_seconds < 10  # Data is fresh if less than 10 seconds old
        else:
            lag_seconds = 999
            data_fresh = False
        
        return {
            "websocket_connected": data_fresh,
            "database_healthy": True,
            "data_lag_ms": int(lag_seconds * 1000),
            "last_update": latest_tick_time.isoformat() if latest_tick_time else None
        }
    finally:
        await release_db_connection(conn)

@app.get("/api/database-status")
async def get_database_status():
    """Get database status and health"""
    conn = await get_db_connection()
    try:
        # Get table size
        table_size = await conn.fetchval("""
            SELECT pg_size_pretty(pg_total_relation_size('market_ticks'))
        """)
        
        # Get total record count
        total_ticks = await conn.fetchval("SELECT COUNT(*) FROM market_ticks")
        
        # Get oldest and newest records
        time_range = await conn.fetchrow("""
            SELECT MIN(time) as oldest, MAX(time) as newest
            FROM market_ticks
        """)
        
        return {
            "table_size": table_size,
            "total_ticks": total_ticks,
            "time_range": dict(time_range) if time_range else {},
            "database_status": "healthy"
        }
    finally:
        await release_db_connection(conn)

# ============================================
# ENGINE CONTROL ENDPOINTS
# ============================================

def is_engine_running():
    """Check if the engine process is running"""
    global engine_process, simulation_start_time
    if engine_process is None:
        return False
    
    # Check if process is still alive
    try:
        is_alive = engine_process.poll() is None
        if not is_alive and simulation_start_time is not None:
            # Process ended, reset simulation start time
            simulation_start_time = None
        return is_alive
    except:
        return False

@app.get("/api/engine/status")
async def get_engine_status():
    """Get current engine status"""
    global simulation_start_time, simulation_duration
    
    running = is_engine_running()
    pid = engine_process.pid if running and engine_process else None
    
    # Calculate time remaining and elapsed
    time_remaining = 0
    elapsed = 0
    progress_percent = 0
    
    if simulation_start_time:
        elapsed = (datetime.now() - simulation_start_time).total_seconds()
        if simulation_duration > 0:
            progress_percent = min(100, (elapsed / simulation_duration) * 100)
            time_remaining = max(0, simulation_duration - elapsed)
    
    return {
        "running": running,
        "pid": pid,
        "engine_path": ENGINE_PATH,
        "duration": simulation_duration,
        "elapsed": round(elapsed * 1000, 0),  # Convert to milliseconds
        "time_remaining": round(time_remaining, 1),
        "progress_percent": round(progress_percent, 1)
    }

@app.post("/api/engine/start")
async def start_engine(request: Request):
    """Start the C++ trading engine with specified duration"""
    global engine_process, simulation_start_time, simulation_duration
    
    # Parse JSON body
    try:
        body = await request.json()
        duration = body.get('duration', 120)
    except:
        duration = 120
    
    if is_engine_running():
        return JSONResponse(
            status_code=400,
            content={"error": "Engine is already running", "pid": engine_process.pid}
        )
    
    # Check if engine executable exists
    if not os.path.exists(ENGINE_PATH):
        return JSONResponse(
            status_code=404,
            content={"error": f"Engine executable not found at {ENGINE_PATH}"}
        )
    
    try:
        # Set simulation parameters
        simulation_duration = duration
        simulation_start_time = datetime.now()
        
        # Start the engine process with duration parameter
        engine_process = subprocess.Popen(
            [ENGINE_PATH, str(duration)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == 'nt' else 0
        )
        
        # Give it a moment to start
        await asyncio.sleep(0.5)
        
        # Check if it started successfully
        if engine_process.poll() is not None:
            # Process already exited
            stdout, stderr = engine_process.communicate()
            return JSONResponse(
                status_code=500,
                content={
                    "error": "Engine failed to start",
                    "stdout": stdout.decode('utf-8', errors='ignore')[:500],
                    "stderr": stderr.decode('utf-8', errors='ignore')[:500]
                }
            )
        
        return {
            "status": "started",
            "pid": engine_process.pid,
            "message": "Engine started successfully"
        }
    
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={"error": f"Failed to start engine: {str(e)}"}
        )

@app.post("/api/engine/stop")
async def stop_engine():
    """Stop the C++ trading engine"""
    global engine_process
    
    if not is_engine_running():
        return JSONResponse(
            status_code=400,
            content={"error": "Engine is not running"}
        )
    
    try:
        pid = engine_process.pid
        
        # Try graceful termination first
        if os.name == 'nt':
            # Windows: send CTRL_BREAK_EVENT
            engine_process.send_signal(signal.CTRL_BREAK_EVENT)
        else:
            # Unix: send SIGTERM
            engine_process.terminate()
        
        # Wait up to 5 seconds for graceful shutdown
        try:
            engine_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            # Force kill if it didn't stop
            engine_process.kill()
            engine_process.wait()
        
        engine_process = None
        simulation_start_time = None  # Reset start time when stopping
        
        return {
            "status": "stopped",
            "pid": pid,
            "message": "Engine stopped successfully"
        }
    
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={"error": f"Failed to stop engine: {str(e)}"}
        )

@app.get("/api/simulations")
async def get_simulation_history():
    """Get simulation history"""
    conn = await get_db_connection()
    try:
        simulations = await conn.fetch("""
            SELECT 
                simulation_id,
                start_time,
                end_time,
                duration_seconds,
                symbol,
                algorithm_params,
                final_stats,
                status,
                CASE 
                    WHEN end_time IS NOT NULL THEN 
                        EXTRACT(EPOCH FROM (end_time - start_time))
                    ELSE NULL 
                END as actual_duration
            FROM simulation_sessions
            ORDER BY start_time DESC
            LIMIT 50
        """)
        
        result = []
        for sim in simulations:
            # Parse final_stats string into a dictionary
            final_stats_dict = {}
            if sim["final_stats"]:
                try:
                    # Parse the string format: "total_pnl=1.68,realized_pnl=1.64,unrealized_pnl=0.04,fill_count=8,quote_count=70,final_position=-0.02"
                    for pair in sim["final_stats"].split(','):
                        if '=' in pair:
                            key, value = pair.split('=', 1)
                            final_stats_dict[key] = value
                except Exception as e:
                    print(f"Error parsing final_stats: {e}")
                    final_stats_dict = {}
            
            result.append({
                "simulation_id": sim["simulation_id"],
                "start_time": sim["start_time"].isoformat() if sim["start_time"] else None,
                "end_time": sim["end_time"].isoformat() if sim["end_time"] else None,
                "duration_seconds": sim["duration_seconds"],
                "actual_duration": float(sim["actual_duration"]) if sim["actual_duration"] else None,
                "symbol": sim["symbol"],
                "algorithm_params": sim["algorithm_params"],
                "final_stats": final_stats_dict,
                "status": sim["status"]
            })
        
        return result
    finally:
        await release_db_connection(conn)

# ============================================
# MAIN
# ============================================

if __name__ == "__main__":
    import uvicorn
    print("=== Starting Trading Dashboard ===")
    print("Dashboard: http://localhost:8000")
    print("API Docs: http://localhost:8000/docs")
    uvicorn.run("trading_dashboard:app", host="0.0.0.0", port=8000, reload=True)
