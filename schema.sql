-- schemas table: stores schema definitions (json)
CREATE TABLE IF NOT EXISTS schemas
(
    id
    SERIAL
    PRIMARY
    KEY,
    name
    TEXT
    NOT
    NULL,
    version
    INTEGER
    NOT
    NULL
    DEFAULT
    1,
    definition
    JSONB
    NOT
    NULL, -- the user-defined schema
    created_at
    TIMESTAMP
    WITH
    TIME
    ZONE
    DEFAULT
    now
(
),
    UNIQUE
(
    name,
    version
)
    );

-- Entities/events table: flexible storage
CREATE TABLE IF NOT EXISTS events
(
    id
    BIGSERIAL
    PRIMARY
    KEY,
    schema_name
    TEXT
    NOT
    NULL,
    schema_version
    INTEGER
    NOT
    NULL,
    entity_id
    TEXT, -- optional logical id (user/item)
    payload
    JSONB
    NOT
    NULL, -- the event or entity data
    event_type
    TEXT, -- e.g. "click","view","rating"
    created_at
    TIMESTAMP
    WITH
    TIME
    ZONE
    DEFAULT
    now
(
)
    );

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_events_schema ON events (schema_name, schema_version);
CREATE INDEX IF NOT EXISTS idx_events_entity ON events (entity_id);
CREATE INDEX IF NOT EXISTS idx_events_type_ts ON events (event_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_events_payload_gin ON events USING GIN (payload jsonb_path_ops);
