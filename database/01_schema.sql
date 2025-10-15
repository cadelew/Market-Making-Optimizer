/*
    {
        "u": 400900217,           // order book updateId
        "s": "BTCUSDT",          // symbol
        "b": "115600.50",        // best bid price
        "B": "1.23456789",       // best bid quantity
        "a": "115601.00",        // best ask price
        "A": "2.34567890"        // best ask quantity
    }

*/

CREATE TABLE  market_ticks (
    time TIMESTAMPTZ NOT NULL,
    symbol TEXT NOT NULL,
    bid NUMERIC(18, 8) NOT NULL,
    bid_size NUMERIC(18, 8) NOT NULL,
    ask NUMERIC(18, 8) NOT NULL,
    ask_size NUMERIC(18, 8) NOT NULL,
    spread NUMERIC(18, 8),
    mid_price NUMERIC(18, 8)
);

SELECT create_hypertable('market_ticks', 'time');

CREATE INDEX idx_market_ticks_symbol_time ON market_ticks (symbol, time DESC);

CREATE INDEX idx_market_ticks_time ON market_ticks (time DESC);

ALTER TABLE market_ticks SET (
    timescaledb.compress,
    timescaledb.compress_segmentby = 'symbol'
);

SELECT add_compression_policy('market_ticks', INTERVAL '7 day');