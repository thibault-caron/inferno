-- Create timescaledb extension (already loaded, but explicit for clarity)
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

-- Agents table (agent registration info)
CREATE TABLE IF NOT EXISTS agents (
    -- id SERIAL PRIMARY KEY,
    id TEXT PRIMARY KEY NOT NULL, -- right now = mac adress, might change
    hostname TEXT NOT NULL,
    os_type SMALLINT NOT NULL,           -- 0=Windows, 1=Linux, 2=macOS
    architecture SMALLINT NOT NULL,      -- 0=x86, 1=x64, 2=ARM
    os_version TEXT,
    current_username TEXT,
    ip_address INET,
    mac_address MACADDR,
    registered_at TIMESTAMPTZ DEFAULT NOW(),
    last_seen TIMESTAMPTZ DEFAULT NOW()
);

-- Base metrics (CPU + Memory only — singular per sample)
CREATE TABLE IF NOT EXISTS metrics (
    time TIMESTAMPTZ NOT NULL,
    agent_id TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    
    -- CPU
    cpu_total_percent FLOAT8,
    cpu_core_count SMALLINT,
    cpu_per_core FLOAT8[],
    
    -- Memory
    mem_phys_total BIGINT,
    mem_phys_used BIGINT,
    mem_phys_available BIGINT,
    mem_swap_total BIGINT,
    mem_swap_used BIGINT
);

SELECT create_hypertable('metrics', 'time', if_not_exists => TRUE);
CREATE INDEX IF NOT EXISTS idx_metrics_agent_time 
    ON metrics (agent_id, time DESC);

-- Disk metrics (one row per disk per sample)
CREATE TABLE IF NOT EXISTS metrics_disk (
    time TIMESTAMPTZ NOT NULL,
    agent_id TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    device TEXT NOT NULL,
    read_bytes_per_sec FLOAT8,
    write_bytes_per_sec FLOAT8
);

SELECT create_hypertable('metrics_disk', 'time', if_not_exists => TRUE);
CREATE INDEX IF NOT EXISTS idx_metrics_disk_agent_device_time 
    ON metrics_disk (agent_id, device, time DESC);

-- Network metrics (one row per interface per sample)
CREATE TABLE IF NOT EXISTS metrics_net (
    time TIMESTAMPTZ NOT NULL,
    agent_id TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    iface TEXT NOT NULL,
    rx_bytes_per_sec FLOAT8,
    tx_bytes_per_sec FLOAT8
);

SELECT create_hypertable('metrics_net', 'time', if_not_exists => TRUE);
CREATE INDEX IF NOT EXISTS idx_metrics_net_agent_iface_time 
    ON metrics_net (agent_id, iface, time DESC);

-- Command history (just tracking what was sent)
CREATE TABLE IF NOT EXISTS command_history (
    id SERIAL PRIMARY KEY,
    agent_id TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    command_type SMALLINT NOT NULL,      -- 0=OS_INFO, 1=PROCESSES, 2=SHELL, etc.
    command_data TEXT,
    sent_at TIMESTAMPTZ DEFAULT NOW()
);

-- Response storage (where status and actual results live)
CREATE TABLE IF NOT EXISTS responses (
    id SERIAL PRIMARY KEY,
    command_id INTEGER NOT NULL REFERENCES command_history(id) ON DELETE CASCADE,
    status SMALLINT NOT NULL,            -- 0=OK, 1=ERROR
    command_type SMALLINT NOT NULL,            -- 0=OK, 1=ERROR
    total_chunks SMALLINT NOT NULL,      -- how many chunks total
    chunk_index SMALLINT NOT NULL,       -- which chunk this is
    chunk_data BYTEA NOT NULL,           -- the actual data
    received_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index to reassemble chunks by command
CREATE INDEX IF NOT EXISTS idx_responses_command_chunk ON responses(command_id, chunk_index);
-- Indexes for performance
-- CREATE INDEX IF NOT EXISTS idx_agents_target ON agents(target_id);
-- CREATE INDEX IF NOT EXISTS idx_agents_ip ON agents(ip_address); -- on agents(mac_address) instead?
CREATE INDEX IF NOT EXISTS idx_command_history_agent ON command_history(agent_id, sent_at DESC);