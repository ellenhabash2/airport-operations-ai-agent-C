# Operational tool registry

`ToolRegistry` is the single source of truth for Gemini schemas, access classification, and handler dispatch. All handlers call the same domain services as the REST API; tools contain no SQL and cannot load code dynamically.

| Tool | Domain | Access | Required arguments | Optional arguments | Service |
|---|---|---|---|---|---|
| `get_all_flights` | Flights | Read-only | — | — | `FlightService::getAll` |
| `find_delayed_flights` | Flights | Read-only | — | — | `FlightService::getDelayed` |
| `get_flight_by_id` | Flights | Read-only | `flight_id` (integer) | — | `FlightService::getById` |
| `get_flight_by_number` | Flights | Read-only | `flight_number` | — | `FlightService::getByNumber` |
| `search_flights` | Flights | Read-only | — | `origin`, `destination`, `status`, `airline`, `terminal_id` | `FlightService::searchFlights` |
| `update_flight_status` | Flights | Write | `flight_id`, `status` | — | `FlightService::updateFlightStatus` |
| `assign_flight_to_gate` | Flights | Write | one flight and one gate identifier | `flight_id`/`flight_number`, `gate_id`/`gate_number` | `FlightService::assignFlightToGate` |
| `get_all_gates` | Gates | Read-only | — | — | `GateService::getAllGates` |
| `get_available_gates` | Gates | Read-only | — | — | `GateService::getAvailableGates` |
| `get_gate_by_id` | Gates | Read-only | `gate_id` (integer) | — | `GateService::getGateById` |
| `get_gate_by_number` | Gates | Read-only | `gate_number` | — | `GateService::getGateByNumber` |
| `get_terminal_status` | Terminals | Read-only | `terminal_id` (integer) | — | `TerminalService::getTerminalStatus` |
| `get_flights_by_terminal` | Terminals | Read-only | `terminal_id` (integer) | — | `TerminalService::getFlightsByTerminal` |
| `get_runway_status` | Runways | Read-only | — | — | `RunwayService::getStatus` |
| `get_runway_by_id` | Runways | Read-only | `runway_id` (integer) | — | `RunwayService::getById` |
| `get_runway_by_code` | Runways | Read-only | `runway_code` | — | `RunwayService::getByCode` |
| `update_runway_status` | Runways | Write | `status` and one runway identifier | `runway_id`/`runway_code` | `RunwayService::updateStatus`/`updateStatusByCode` |
| `get_latest_weather` | Weather | Read-only | — | — | `WeatherService::getLatest` |
| `get_all_incidents` | Incidents | Read-only | — | — | `IncidentService::getAll` |
| `get_active_incidents` | Incidents | Read-only | — | — | `IncidentService::getActive` |
| `get_incidents_by_severity` | Incidents | Read-only | `severity` | — | `IncidentService::getBySeverity` |
| `search_incidents` | Incidents | Read-only | `query` | — | `IncidentService::search` |
| `create_incident` | Incidents | Write | `title`, `description`, `severity` | `location` | `IncidentService::create` |
| `resolve_incident` | Incidents | Write | `id` (integer) | — | `IncidentService::resolve` |

## Legacy compatibility

`get_flight_details` remains registered as a deprecated alias accepting string `id`. New model calls should use `get_flight_by_id`. Both paths delegate to `FlightService::getById`; no domain logic is duplicated.

## Validation and safe execution

The registry validates required fields, JSON types, positive integer identifiers, nonblank bounded strings, supported status/severity enums, and the paired identifiers required for assignments and runway updates. Domain existence, conflicts, and operational rules remain in services.

Unknown names, invalid arguments, unauthenticated writes, domain failures, and unexpected exceptions return controlled machine-readable errors. Internal exception details, SQL, authentication data, and stack traces are never returned.

All write tools require `ToolExecutionContext.authenticated` and a resolved `userId`. That identity comes from verified JWT context in `AgentService`; Gemini arguments cannot supply or override it. The current incident schema has no creator/resolver audit column, so identity is enforced but is not persisted or exposed.
