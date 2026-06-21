-- AeroMind Airport Operations Database Schema

CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(80) UNIQUE NOT NULL,
    email VARCHAR(160) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(40) NOT NULL DEFAULT 'operator',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS airlines (
    id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    iata_code VARCHAR(3) UNIQUE NOT NULL,
    icao_code VARCHAR(4) UNIQUE NOT NULL,
    country VARCHAR(100) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS aircraft (
    id SERIAL PRIMARY KEY,
    registration_number VARCHAR(20) UNIQUE NOT NULL,
    aircraft_type VARCHAR(60) NOT NULL,
    airline_id INTEGER NOT NULL REFERENCES airlines(id) ON DELETE RESTRICT,
    status VARCHAR(40) NOT NULL DEFAULT 'ACTIVE',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT aircraft_status_check CHECK (status IN ('ACTIVE', 'MAINTENANCE', 'RETIRED'))
);

CREATE TABLE IF NOT EXISTS terminals (
    id SERIAL PRIMARY KEY,
    name VARCHAR(80) UNIQUE NOT NULL,
    code VARCHAR(8) UNIQUE NOT NULL,
    capacity INTEGER NOT NULL CHECK (capacity > 0),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS gates (
    id SERIAL PRIMARY KEY,
    gate_number VARCHAR(10) NOT NULL,
    terminal_id INTEGER NOT NULL REFERENCES terminals(id) ON DELETE RESTRICT,
    status VARCHAR(40) NOT NULL DEFAULT 'AVAILABLE',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(gate_number, terminal_id),
    CONSTRAINT gates_status_check CHECK (status IN ('AVAILABLE', 'OCCUPIED', 'MAINTENANCE', 'CLOSED'))
);

CREATE TABLE IF NOT EXISTS runways (
    id SERIAL PRIMARY KEY,
    runway_code VARCHAR(10) UNIQUE NOT NULL,
    status VARCHAR(40) NOT NULL DEFAULT 'OPERATIONAL',
    length_meters INTEGER NOT NULL CHECK (length_meters > 0),
    surface VARCHAR(40) NOT NULL DEFAULT 'ASPHALT',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT runways_status_check CHECK (status IN ('OPERATIONAL', 'MAINTENANCE', 'CLOSED'))
);

CREATE TABLE IF NOT EXISTS flights (
    id SERIAL PRIMARY KEY,
    flight_number VARCHAR(20) NOT NULL,
    airline_id INTEGER NOT NULL REFERENCES airlines(id) ON DELETE RESTRICT,
    aircraft_id INTEGER NOT NULL REFERENCES aircraft(id) ON DELETE RESTRICT,
    gate_id INTEGER REFERENCES gates(id) ON DELETE SET NULL,
    runway_id INTEGER REFERENCES runways(id) ON DELETE SET NULL,
    origin VARCHAR(3) NOT NULL,
    destination VARCHAR(3) NOT NULL,
    departure_time TIMESTAMP NOT NULL,
    arrival_time TIMESTAMP NOT NULL,
    status VARCHAR(40) NOT NULL DEFAULT 'SCHEDULED',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT flights_airport_code_check CHECK (origin ~ '^[A-Z]{3}$' AND destination ~ '^[A-Z]{3}$'),
    CONSTRAINT flights_schedule_check CHECK (arrival_time > departure_time),
    CONSTRAINT flights_status_check CHECK (status IN ('SCHEDULED', 'BOARDING', 'IN_FLIGHT', 'DELAYED', 'CANCELLED', 'LANDED'))
);

CREATE TABLE IF NOT EXISTS crew (
    id SERIAL PRIMARY KEY,
    full_name VARCHAR(150) NOT NULL,
    role VARCHAR(60) NOT NULL,
    employee_code VARCHAR(24) UNIQUE NOT NULL,
    availability_status VARCHAR(40) NOT NULL DEFAULT 'AVAILABLE',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT crew_availability_check CHECK (availability_status IN ('AVAILABLE', 'ON_DUTY', 'RESTING', 'UNAVAILABLE'))
);

CREATE TABLE IF NOT EXISTS flight_crew (
    flight_id INTEGER NOT NULL REFERENCES flights(id) ON DELETE CASCADE,
    crew_id INTEGER NOT NULL REFERENCES crew(id) ON DELETE RESTRICT,
    assigned_role VARCHAR(60) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (flight_id, crew_id)
);

CREATE TABLE IF NOT EXISTS weather_reports (
    id SERIAL PRIMARY KEY,
    condition VARCHAR(100) NOT NULL,
    visibility_km NUMERIC(5,2) NOT NULL CHECK (visibility_km >= 0),
    wind_speed_kmh NUMERIC(6,2) NOT NULL CHECK (wind_speed_kmh >= 0),
    wind_direction VARCHAR(3) NOT NULL DEFAULT 'VRB',
    temperature_c NUMERIC(5,2) NOT NULL,
    pressure_hpa NUMERIC(7,2) NOT NULL DEFAULT 1013.25,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS incidents (
    id SERIAL PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    severity VARCHAR(40) NOT NULL,
    location VARCHAR(150),
    status VARCHAR(40) NOT NULL DEFAULT 'OPEN',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    resolved_at TIMESTAMP,
    CONSTRAINT incidents_severity_check CHECK (severity IN ('LOW', 'MEDIUM', 'HIGH', 'CRITICAL')),
    CONSTRAINT incidents_status_check CHECK (status IN ('OPEN', 'INVESTIGATING', 'RESOLVED'))
);

CREATE TABLE IF NOT EXISTS conversations (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(200) NOT NULL DEFAULT 'Airport Operations Conversation',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS messages (
    id SERIAL PRIMARY KEY,
    conversation_id INTEGER NOT NULL REFERENCES conversations(id) ON DELETE CASCADE,
    role VARCHAR(40) NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT messages_role_check CHECK (role IN ('user', 'assistant', 'system', 'tool'))
);

CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_aircraft_airline ON aircraft(airline_id);
CREATE INDEX IF NOT EXISTS idx_gates_terminal_status ON gates(terminal_id, status);
CREATE INDEX IF NOT EXISTS idx_flights_airline ON flights(airline_id);
CREATE INDEX IF NOT EXISTS idx_flights_aircraft ON flights(aircraft_id);
CREATE INDEX IF NOT EXISTS idx_flights_gate ON flights(gate_id);
CREATE INDEX IF NOT EXISTS idx_flights_status_departure ON flights(status, departure_time);
CREATE INDEX IF NOT EXISTS idx_flight_crew_crew ON flight_crew(crew_id);
CREATE INDEX IF NOT EXISTS idx_weather_created ON weather_reports(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_incidents_status_created ON incidents(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_messages_conversation_created ON messages(conversation_id, created_at);
