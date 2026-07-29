# Flight operation tools

All flight tools call `FlightService`. `get_flight_details` remains a legacy ID
lookup alias for compatibility.

- `get_flight_by_id`: integer `flight_id`
- `get_flight_by_number`: string `flight_number`
- `search_flights`: optional `origin`, `destination`, `status`, `airline`, and
  integer `terminal_id`
- `update_flight_status`: integer `flight_id` and supported `status`
- `assign_flight_to_gate`: either `flight_id` or `flight_number`, and either
  `gate_id` or `gate_number`

The last form supports “Move flight SB2101 to gate A03”; gate lookup also
matches the seeded `A3` representation. Writes run through authenticated
`/agent/query` and use the same service transaction as REST writes.

# Gate and terminal tools

All tools execute through the authenticated agent endpoint and call the same services as REST.

- `get_all_gates`: no arguments; returns every gate.
- `get_gate_by_id`: required integer `gate_id`.
- `get_gate_by_number`: required string `gate_number`; lookup is case-insensitive and preserves `A03`/`A3` compatibility.
- `get_available_gates`: no arguments; unchanged contract, returning only `status = AVAILABLE` gates.
- `get_terminal_status`: required integer `terminal_id`; returns terminal operational counts.
- `get_flights_by_terminal`: required integer `terminal_id`; returns flights assigned through terminal gates.

`get_all_terminals` is not registered because flight results already include `terminal_id`, directly supporting the required reasoning chain without a redundant discovery call.

# Incident tools

All incident tools call `IncidentService`; none access SQL directly. Severities use
the canonical values `LOW`, `MEDIUM`, `HIGH`, and `CRITICAL`.

- `get_all_incidents`: no arguments; returns active and resolved incidents.
- `get_active_incidents`: no arguments; returns `OPEN` and `INVESTIGATING` incidents.
- `get_incidents_by_severity`: required string `severity`; input is case-insensitive.
- `search_incidents`: required string `query`; searches title, description, and location using literal, case-insensitive partial matching.
- `create_incident`: required `title`, `description`, and `severity`; optional `location`.
- `resolve_incident`: required incident `id`; repeated or concurrent resolution returns a controlled conflict.

Write tools remain available only through the authenticated agent endpoint. The current
incident schema has no creator or resolver identity column, so identity is not persisted.
