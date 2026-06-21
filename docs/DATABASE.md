# AeroMind Database

The AeroMind foundation uses PostgreSQL 16 and initializes from `sql/init.sql` and `sql/seed.sql`.

## Seed Volume

| Entity | Count |
| --- | ---: |
| Airlines | 5 |
| Aircraft | 25 |
| Terminals | 3 |
| Gates | 36 |
| Runways | 3 |
| Flights | 150 |
| Crew | 20 |
| Weather reports | 30 |
| Incidents | 30 |

## Tables

### users

Stores accounts for future authenticated access.

Key columns: `id`, `username`, `email`, `password_hash`, `role`, `created_at`, `updated_at`.

Constraints: unique username, unique email.

### airlines

Stores operating carriers.

Key columns: `id`, `name`, `iata_code`, `icao_code`, `country`, `created_at`.

Constraints: unique IATA and ICAO codes.

### aircraft

Stores aircraft assigned to airlines.

Key columns: `id`, `registration_number`, `aircraft_type`, `airline_id`, `status`, `created_at`.

Foreign key: `airline_id` -> `airlines.id`.

Constraints: unique registration number, checked aircraft status.

### terminals

Stores airport terminal metadata.

Key columns: `id`, `name`, `code`, `capacity`, `created_at`.

Constraints: unique name, unique code, positive capacity.

### gates

Stores gates within terminals.

Key columns: `id`, `gate_number`, `terminal_id`, `status`, `created_at`.

Foreign key: `terminal_id` -> `terminals.id`.

Constraints: unique `(gate_number, terminal_id)`, checked gate status.

### runways

Stores runway configuration and operational state.

Key columns: `id`, `runway_code`, `status`, `length_meters`, `surface`, `created_at`.

Constraints: unique runway code, positive length, checked runway status.

### flights

Central operational flight table.

Key columns: `id`, `flight_number`, `airline_id`, `aircraft_id`, `gate_id`, `runway_id`, `origin`, `destination`, `departure_time`, `arrival_time`, `status`, `created_at`.

Foreign keys:

- `airline_id` -> `airlines.id`
- `aircraft_id` -> `aircraft.id`
- `gate_id` -> `gates.id`
- `runway_id` -> `runways.id`

Constraints: airport-code format checks, arrival after departure, checked flight status.

### crew

Stores crew and operational staff.

Key columns: `id`, `full_name`, `role`, `employee_code`, `availability_status`, `created_at`.

Constraints: unique employee code, checked availability status.

### flight_crew

Join table assigning crew to flights.

Key columns: `flight_id`, `crew_id`, `assigned_role`, `created_at`.

Foreign keys:

- `flight_id` -> `flights.id`
- `crew_id` -> `crew.id`

Primary key: `(flight_id, crew_id)`.

### weather_reports

Stores airport weather observations.

Key columns: `id`, `condition`, `visibility_km`, `wind_speed_kmh`, `wind_direction`, `temperature_c`, `pressure_hpa`, `created_at`.

Constraints: non-negative visibility and wind speed.

### incidents

Stores operational incidents and alerts.

Key columns: `id`, `title`, `description`, `severity`, `location`, `status`, `created_at`, `resolved_at`.

Constraints: checked severity and incident status.

### conversations

Stores future AI chat sessions.

Key columns: `id`, `user_id`, `title`, `created_at`.

Foreign key: `user_id` -> `users.id`.

### messages

Stores future chat history messages.

Key columns: `id`, `conversation_id`, `role`, `content`, `created_at`.

Foreign key: `conversation_id` -> `conversations.id`.

Constraints: checked message role.

## Indexes

Operational indexes are defined for:

- User email lookup
- Aircraft by airline
- Gates by terminal and status
- Flights by airline, aircraft, gate, status, and departure time
- Crew assignments by crew member
- Latest weather reports
- Incident status and recency
- Conversation message history
