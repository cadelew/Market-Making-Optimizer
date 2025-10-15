-- Table for storing Avellaneda-Stoikov algorithm quotes
CREATE TABLE as_quotes (
    time TIMESTAMPTZ NOT NULL,
    symbol TEXT NOT NULL,
    our_bid NUMERIC(18, 8) NOT NULL,
    our_ask NUMERIC(18, 8) NOT NULL,
    our_spread NUMERIC(18, 8) NOT NULL,
    spread_bps NUMERIC(18, 4) NOT NULL,
    market_mid NUMERIC(18, 8) NOT NULL,
    position NUMERIC(18, 8) NOT NULL,
    avg_entry_price NUMERIC(18, 8),
    volatility NUMERIC(18, 8)
);

SELECT create_hypertable('as_quotes', 'time');

CREATE INDEX idx_as_quotes_symbol_time ON as_quotes (symbol, time DESC);
CREATE INDEX idx_as_quotes_time ON as_quotes (time DESC);

ALTER TABLE as_quotes SET (
    timescaledb.compress,
    timescaledb.compress_segmentby = 'symbol'
);

SELECT add_compression_policy('as_quotes', INTERVAL '7 day');

-- Table for storing P&L and trading statistics
CREATE TABLE trading_stats (
    time TIMESTAMPTZ NOT NULL,
    symbol TEXT NOT NULL,
    position NUMERIC(18, 8) NOT NULL,
    avg_entry_price NUMERIC(18, 8),
    realized_pnl NUMERIC(18, 8) NOT NULL,
    unrealized_pnl NUMERIC(18, 8) NOT NULL,
    total_pnl NUMERIC(18, 8) NOT NULL,
    fill_count INTEGER NOT NULL,
    quote_count INTEGER NOT NULL,
    fill_rate NUMERIC(8, 4)
);

SELECT create_hypertable('trading_stats', 'time');

CREATE INDEX idx_trading_stats_symbol_time ON trading_stats (symbol, time DESC);
CREATE INDEX idx_trading_stats_time ON trading_stats (time DESC);

ALTER TABLE trading_stats SET (
    timescaledb.compress,
    timescaledb.compress_segmentby = 'symbol'
);

SELECT add_compression_policy('trading_stats', INTERVAL '7 day');


