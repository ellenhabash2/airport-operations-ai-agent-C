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
