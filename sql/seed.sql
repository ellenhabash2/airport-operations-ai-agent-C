-- AeroMind Airport Operations Seed Data

-- Demo accounts. Passwords are documented in the README.
--   ops.admin@aeromind.local    / admin123
--   duty.manager@aeromind.local / manager123
-- Real bcrypt ($2b$, 10 rounds) hashes, verified against the Bcrypt.cpp build used by the backend.
INSERT INTO users (username, email, password_hash, role) VALUES
('ops_admin', 'ops.admin@aeromind.local', '$2b$10$hydVDVhjwlRi44TX1yzHYOFcnIIlCAvTqTf3PySiBJgsTrgrFpLk.', 'admin'),
('duty_manager', 'duty.manager@aeromind.local', '$2b$10$1Hak1Lx2UG/Yi4oodB/Buepv.14kx2CpLBCqL5HDB0.PbLhQcxagO', 'operator');

INSERT INTO airlines (name, iata_code, icao_code, country) VALUES
('British Airways', 'BA', 'BAW', 'United Kingdom'),
('Lufthansa', 'LH', 'DLH', 'Germany'),
('El Al Israel Airlines', 'LY', 'ELY', 'Israel'),
('Delta Air Lines', 'DL', 'DAL', 'United States'),
('Emirates', 'EK', 'UAE', 'United Arab Emirates');

INSERT INTO aircraft (registration_number, aircraft_type, airline_id, status) VALUES
('G-STBA', 'Boeing 777-300ER', 1, 'ACTIVE'),
('G-XWBA', 'Airbus A350-1000', 1, 'ACTIVE'),
('G-ZBJA', 'Boeing 787-8', 1, 'ACTIVE'),
('G-TTNA', 'Airbus A320neo', 1, 'ACTIVE'),
('G-EUPA', 'Airbus A319', 1, 'MAINTENANCE'),
('D-AIXA', 'Airbus A350-900', 2, 'ACTIVE'),
('D-ABYA', 'Boeing 747-8', 2, 'ACTIVE'),
('D-AINA', 'Airbus A320neo', 2, 'ACTIVE'),
('D-AIZQ', 'Airbus A320-200', 2, 'ACTIVE'),
('D-AIHF', 'Airbus A340-600', 2, 'MAINTENANCE'),
('4X-EDA', 'Boeing 787-9', 3, 'ACTIVE'),
('4X-EKA', 'Boeing 737-900ER', 3, 'ACTIVE'),
('4X-ELB', 'Boeing 777-200ER', 3, 'ACTIVE'),
('4X-EHD', 'Boeing 737-800', 3, 'ACTIVE'),
('4X-ECE', 'Boeing 787-8', 3, 'ACTIVE'),
('N501DN', 'Airbus A350-900', 4, 'ACTIVE'),
('N861NW', 'Airbus A330-200', 4, 'ACTIVE'),
('N375DA', 'Boeing 737-800', 4, 'ACTIVE'),
('N120DU', 'Airbus A220-100', 4, 'ACTIVE'),
('N704X', 'Boeing 757-200', 4, 'MAINTENANCE'),
('A6-EPA', 'Boeing 777-300ER', 5, 'ACTIVE'),
('A6-EUA', 'Airbus A380-800', 5, 'ACTIVE'),
('A6-EVG', 'Boeing 777F', 5, 'ACTIVE'),
('A6-EOA', 'Airbus A380-800', 5, 'ACTIVE'),
('A6-ENB', 'Boeing 777-200LR', 5, 'ACTIVE');

INSERT INTO terminals (name, code, capacity) VALUES
('Terminal 1 - International', 'T1', 42000),
('Terminal 2 - Domestic', 'T2', 28000),
('Terminal 3 - Cargo and Remote', 'T3', 12000);

INSERT INTO gates (gate_number, terminal_id, status)
SELECT gate_number, terminal_id,
       CASE
           WHEN ordinal % 17 = 0 THEN 'MAINTENANCE'
           WHEN ordinal % 5 = 0 THEN 'OCCUPIED'
           ELSE 'AVAILABLE'
       END
FROM (
    SELECT 'A' || gs AS gate_number, 1 AS terminal_id, gs AS ordinal FROM generate_series(1, 12) gs
    UNION ALL
    SELECT 'B' || gs AS gate_number, 1 AS terminal_id, 12 + gs AS ordinal FROM generate_series(1, 12) gs
    UNION ALL
    SELECT 'C' || gs AS gate_number, 2 AS terminal_id, 24 + gs AS ordinal FROM generate_series(1, 8) gs
    UNION ALL
    SELECT 'D' || gs AS gate_number, 3 AS terminal_id, 32 + gs AS ordinal FROM generate_series(1, 4) gs
) gates_seed;

INSERT INTO runways (runway_code, status, length_meters, surface) VALUES
('09L/27R', 'OPERATIONAL', 4105, 'ASPHALT'),
('09R/27L', 'OPERATIONAL', 3810, 'CONCRETE'),
('18/36', 'MAINTENANCE', 3350, 'ASPHALT');

INSERT INTO flights (
    flight_number,
    airline_id,
    aircraft_id,
    gate_id,
    runway_id,
    origin,
    destination,
    departure_time,
    arrival_time,
    status
)
-- Flights are anchored to the current time so the agent always sees a live
-- operational picture: roughly 12 hours of history and 33 hours ahead.
-- Status is derived from the schedule instead of a fixed cycle, so a LANDED
-- flight can never sit in the future.
SELECT
    flight_number,
    airline_id,
    aircraft_id,
    gate_id,
    runway_id,
    origin,
    destination,
    departure_time,
    arrival_time,
    CASE
        WHEN gs % 13 = 0 AND departure_time > LOCALTIMESTAMP                      THEN 'CANCELLED'
        WHEN gs % 7  = 0 AND departure_time > LOCALTIMESTAMP - INTERVAL '3 hours' THEN 'DELAYED'
        WHEN arrival_time   < LOCALTIMESTAMP                                      THEN 'LANDED'
        WHEN departure_time < LOCALTIMESTAMP                                      THEN 'IN_FLIGHT'
        WHEN departure_time < LOCALTIMESTAMP + INTERVAL '45 minutes'              THEN 'BOARDING'
        ELSE 'SCHEDULED'
    END AS status
FROM (
    SELECT
        gs,
        (ARRAY['BA', 'LH', 'LY', 'DL', 'EK'])[(gs - 1) % 5 + 1] ||
            (200 + gs)::text AS flight_number,
        ((gs - 1) % 5) + 1 AS airline_id,
        ((((gs - 1) % 5) * 5) + (((gs - 1) / 5) % 5) + 1) AS aircraft_id,
        ((gs - 1) % 36) + 1 AS gate_id,
        ((gs - 1) % 3) + 1 AS runway_id,
        (ARRAY['JFK', 'LHR', 'FRA', 'TLV', 'DXB', 'AMS', 'CDG', 'MAD', 'ATH', 'ZRH'])[(gs - 1) % 10 + 1] AS origin,
        (ARRAY['LAX', 'BOS', 'ORD', 'MIA', 'SFO', 'YYZ', 'IST', 'FCO', 'VIE', 'CPH'])[(gs - 1) % 10 + 1] AS destination,
        date_trunc('hour', LOCALTIMESTAMP) - INTERVAL '12 hours'
            + (gs * INTERVAL '18 minutes') AS departure_time,
        date_trunc('hour', LOCALTIMESTAMP) - INTERVAL '12 hours'
            + (gs * INTERVAL '18 minutes')
            + (((gs % 7) + 2) * INTERVAL '45 minutes') AS arrival_time
    FROM generate_series(1, 150) gs
) f;

INSERT INTO crew (full_name, role, employee_code, availability_status) VALUES
('Captain Amelia Brooks', 'Captain', 'CRW-1001', 'ON_DUTY'),
('Captain Daniel Reyes', 'Captain', 'CRW-1002', 'AVAILABLE'),
('Captain Hannah Stein', 'Captain', 'CRW-1003', 'RESTING'),
('First Officer Luca Weber', 'First Officer', 'CRW-1004', 'ON_DUTY'),
('First Officer Naomi Cohen', 'First Officer', 'CRW-1005', 'AVAILABLE'),
('First Officer Grace Miller', 'First Officer', 'CRW-1006', 'AVAILABLE'),
('Chief Purser Oliver Grant', 'Cabin Lead', 'CRW-1007', 'ON_DUTY'),
('Chief Purser Mira Haddad', 'Cabin Lead', 'CRW-1008', 'AVAILABLE'),
('Cabin Crew Sofia Evans', 'Flight Attendant', 'CRW-1009', 'ON_DUTY'),
('Cabin Crew Ethan Brooks', 'Flight Attendant', 'CRW-1010', 'AVAILABLE'),
('Cabin Crew Leila Mansour', 'Flight Attendant', 'CRW-1011', 'AVAILABLE'),
('Cabin Crew Jonas Keller', 'Flight Attendant', 'CRW-1012', 'RESTING'),
('Ground Lead Priya Shah', 'Ground Operations', 'CRW-1013', 'ON_DUTY'),
('Ramp Coordinator Ben Carter', 'Ramp Operations', 'CRW-1014', 'AVAILABLE'),
('Dispatcher Ava Lewis', 'Dispatcher', 'CRW-1015', 'ON_DUTY'),
('Dispatcher Omar Nasser', 'Dispatcher', 'CRW-1016', 'AVAILABLE'),
('Maintenance Lead Mark Fischer', 'Maintenance', 'CRW-1017', 'ON_DUTY'),
('Fuel Coordinator Chen Wang', 'Fuel Operations', 'CRW-1018', 'AVAILABLE'),
('Load Planner Iris Novak', 'Load Planning', 'CRW-1019', 'AVAILABLE'),
('Security Liaison Maya Patel', 'Security Operations', 'CRW-1020', 'ON_DUTY');

INSERT INTO flight_crew (flight_id, crew_id, assigned_role)
SELECT flight_id, crew_id,
       CASE crew_slot
           WHEN 0 THEN 'Captain'
           WHEN 1 THEN 'First Officer'
           ELSE 'Cabin Lead'
       END
FROM (
    SELECT f.id AS flight_id,
           ((f.id + slot - 1) % 20) + 1 AS crew_id,
           slot AS crew_slot
    FROM flights f
    CROSS JOIN generate_series(0, 2) slot
    WHERE f.id <= 40
) assignments;

INSERT INTO weather_reports (
    condition,
    visibility_km,
    wind_speed_kmh,
    wind_direction,
    temperature_c,
    pressure_hpa,
    created_at
)
SELECT
    (ARRAY['Clear', 'Partly Cloudy', 'Overcast', 'Light Rain', 'Rain', 'Mist', 'Crosswind Advisory'])[(gs - 1) % 7 + 1],
    ROUND((12.5 - (gs % 8) * 0.9)::numeric, 2),
    ROUND((6 + (gs % 10) * 2.7)::numeric, 2),
    (ARRAY['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'])[(gs - 1) % 8 + 1],
    ROUND((18 + (gs % 9) * 1.1)::numeric, 2),
    ROUND((1007 + (gs % 12) * 0.8)::numeric, 2),
    NOW() - ((30 - gs) * INTERVAL '30 minutes')
FROM generate_series(1, 30) gs;

INSERT INTO incidents (title, description, severity, location, status, created_at, resolved_at)
SELECT
    (ARRAY[
        'Gate Turnaround Delay',
        'Baggage Belt Slowdown',
        'Runway Inspection Hold',
        'Catering Truck Late Arrival',
        'Crew Reassignment',
        'Fueling Sequence Conflict',
        'Passenger Assistance Request',
        'Aircraft Technical Review',
        'Apron Vehicle Congestion',
        'Weather Flow Restriction'
    ])[(gs - 1) % 10 + 1] AS title,
    'Operational event ' || gs || ' recorded by AeroMind seed data for airport control center workflows.' AS description,
    (ARRAY['LOW', 'MEDIUM', 'HIGH', 'CRITICAL'])[(gs - 1) % 4 + 1] AS severity,
    (ARRAY['Terminal 1', 'Terminal 2', 'Terminal 3', 'Apron East', 'Apron West', 'Control Tower', 'Runway 09L', 'Baggage Hall'])[(gs - 1) % 8 + 1] AS location,
    CASE WHEN gs % 5 = 0 THEN 'RESOLVED' WHEN gs % 3 = 0 THEN 'INVESTIGATING' ELSE 'OPEN' END AS status,
    NOW() - ((30 - gs) * INTERVAL '1 hour') AS created_at,
    CASE WHEN gs % 5 = 0 THEN NOW() - ((30 - gs) * INTERVAL '1 hour') + INTERVAL '35 minutes' ELSE NULL END AS resolved_at
FROM generate_series(1, 30) gs;

INSERT INTO conversations (user_id, title) VALUES
(1, 'Morning airport operations briefing'),
(2, 'Weather and gate recovery review');

INSERT INTO messages (conversation_id, role, content) VALUES
(1, 'user', 'Summarize current delayed flights and high severity incidents.'),
(1, 'assistant', 'I can review delayed flights and active incidents using the current simulated operations data.'),
(2, 'user', 'Check crosswind impact on runway assignments.'),
(2, 'assistant', 'I can compare the latest weather report with current runway and gate status.');
