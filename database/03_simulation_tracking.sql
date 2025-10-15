-- Simulation tracking system
-- Each simulation gets a unique ID and we track metadata

CREATE TABLE simulation_sessions (
    simulation_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    start_time TIMESTAMPTZ NOT NULL,
    end_time TIMESTAMPTZ,
    duration_seconds INTEGER,
    symbol TEXT NOT NULL,
    algorithm_params JSONB, -- Store gamma, sigma, T, kappa
    final_stats JSONB, -- Store final P&L, fill count, etc.
    status TEXT DEFAULT 'running' CHECK (status IN ('running', 'completed', 'stopped', 'error'))
);

-- Add simulation_id to existing tables
ALTER TABLE market_ticks ADD COLUMN simulation_id UUID;
ALTER TABLE as_quotes ADD COLUMN simulation_id UUID;
ALTER TABLE trading_stats ADD COLUMN simulation_id UUID;

-- Create indexes for simulation-based queries
CREATE INDEX idx_market_ticks_simulation ON market_ticks (simulation_id, time DESC);
CREATE INDEX idx_as_quotes_simulation ON as_quotes (simulation_id, time DESC);
CREATE INDEX idx_trading_stats_simulation ON trading_stats (simulation_id, time DESC);

-- Create indexes for simulation sessions
CREATE INDEX idx_simulation_sessions_start_time ON simulation_sessions (start_time DESC);
CREATE INDEX idx_simulation_sessions_status ON simulation_sessions (status);

-- Add foreign key constraints (optional, can be added later)
-- ALTER TABLE market_ticks ADD CONSTRAINT fk_market_ticks_simulation 
--     FOREIGN KEY (simulation_id) REFERENCES simulation_sessions(simulation_id);
-- ALTER TABLE as_quotes ADD CONSTRAINT fk_as_quotes_simulation 
--     FOREIGN KEY (simulation_id) REFERENCES simulation_sessions(simulation_id);
-- ALTER TABLE trading_stats ADD CONSTRAINT fk_trading_stats_simulation 
--     FOREIGN KEY (simulation_id) REFERENCES simulation_sessions(simulation_id);

