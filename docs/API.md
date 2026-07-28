# AeroMind API

## Flight operations

Read routes preserve the existing public behavior; operational writes require a
JWT bearer token. `GET /flights`, `GET /flights/delayed`, `GET /flights/{id}`,
and `GET /flights/number/{flightNumber}` return flight data. Number matching is
exact and case-insensitive. `GET /flights/search` accepts optional `origin`,
`destination`, `status`, `airline`, and `terminal_id`; text filters are partial
and case-insensitive, all filters combine with AND, and no matches return an
empty list.

`PATCH /flights/{id}/status` accepts `{"status":"delayed"}`.
`PATCH /flights/{id}/gate` accepts `{"gate_id":3}` and returns the updated
flight, nullable previous gate, and new gate. Missing resources return 404;
unavailable, maintenance, or closed gates return 409.

Supported canonical database statuses are `SCHEDULED`, `BOARDING`, `IN_FLIGHT`,
`DELAYED`, `CANCELLED`, and `LANDED`. Inputs are case-insensitive; `departed` and
`arrived` are accepted aliases for `IN_FLIGHT` and `LANDED`.

## Conventions

The local base URL is `http://localhost:8848`. JSON successes generally contain `"status": "success"`; errors contain a safe top-level `error` string. Protected routes require:

```http
Authorization: Bearer <token>
```

Missing, malformed, invalid, or expired tokens return `401`. Unexpected database or application failures return `500` without SQL details. IDs are positive integer strings in route parameters and are often serialized as strings in responses.

## Health

### Service health

- **Method/route:** `GET /health`
- **Authentication:** None
- **Purpose:** Report backend and database availability.
- **Request:** No body or parameters.
- **Success:** `200` when PostgreSQL is connected.

```json
{"status":"ok","service":"AeroMind","database":"connected"}
```

- **Failure:** `503` when the backend is running but cannot reach PostgreSQL.

## Authentication

### Register

- **Method/route:** `POST /auth/register`
- **Authentication:** None
- **Body:** `username`, `email`, and `password`, all strings.
- **Validation:** Trimmed username has at least 2 characters; email has a non-empty local part, `@`, a later `.`, and no whitespace; password has at least 6 characters. Email is normalized to lowercase.
- **Side effect:** Creates a user with a bcrypt password hash.
- **Success:** `201` with the created user data and no password hash.

```json
{
  "status": "success",
  "message": "User registered",
  "data": {"id":"3","username":"operator","email":"operator@example.com","role":"operator"}
}
```

- **Failures:** `400` missing/invalid input; `409` duplicate username or email; `500` unexpected failure.

### Login

- **Method/route:** `POST /auth/login`
- **Authentication:** None
- **Body:** string `email` and `password`.
- **Validation:** Both fields are required; email is trimmed and lowercased.
- **Success:** `200`; returns a 24-hour HS256 JWT and public user fields.

```json
{
  "status":"success",
  "message":"Login successful",
  "token":"<token>",
  "user":{"id":"3","username":"operator","email":"operator@example.com","role":"operator"}
}
```

- **Failures:** `400` missing fields; `401` invalid email/password; `500` unexpected failure. The same credential error is used whether the account or password is wrong.

### Verify current session

- **Method/route:** `GET /auth/me`
- **Authentication:** JWT
- **Purpose:** Verify that the current token is accepted.
- **Success:** `200` with `{"status":"success","authenticated":true}`.
- **Failure:** `401` invalid authentication.

## Flights

### List flights

- **Method/route:** `GET /flights`
- **Authentication:** None
- **Purpose:** Return flights joined with airline, aircraft, gate, and runway details.
- **Success:** `200` with `status` and a `data` array.
- **Failure:** `500` unexpected database failure.

### List delayed flights

- **Method/route:** `GET /flights/delayed`
- **Authentication:** None
- **Purpose:** Return flights whose status is `DELAYED`.
- **Success:** `200` with `status`, numeric `count`, and `data`.
- **Failure:** `500` unexpected database failure.

### Get flight

- **Method/route:** `GET /flights/{id}`
- **Authentication:** None
- **Validation:** `id` must contain decimal digits.
- **Success:** `200` with `{"status":"success","data":{...}}`.
- **Failures:** `400` invalid ID; `404` no matching flight; `500` unexpected database failure.

## Gates

### List gates

- **Method/route:** `GET /gates`
- **Authentication:** None
- **Purpose:** Return every gate with its terminal and status.
- **Success:** `200` with `status` and `data` array.
- **Failure:** `500` unexpected database failure.

### Available and individual gates

- **Routes:** `GET /gates/available`, `GET /gates/{id}`, and `GET /gates/number/{gateNumber}`
- **Authentication:** None
- **Success:** `200` with the existing `status`/`data` envelope. Empty lists are valid.
- **Validation:** IDs must be positive integers; gate numbers are trimmed, limited to 10 characters, and matched case-insensitively. Established `A03`/`A3` normalization remains supported.
- **Failures:** `400` invalid input; `404` missing gate; `500` unexpected database failure.

## Terminals

- `GET /terminals` lists terminals; an empty array is valid.
- `GET /terminals/{id}` returns one terminal.
- `GET /terminals/{id}/status` returns `total_gates`, `available_gates`, `occupied_gates`, `non_operational_gates`, and `active_flights` with the terminal object.
- `GET /terminals/{id}/flights` returns every flight assigned to a gate in the terminal; an empty array is valid.

All terminal routes are unauthenticated, matching the operational read policy. Success uses the `status`/`data` envelope. Invalid IDs return `400`, missing terminals return `404`, and unexpected failures return `500`.

## Runways

### List runways

- **Method/route:** `GET /runways`
- **Authentication:** None
- **Purpose:** Return all runway codes, statuses, lengths, surfaces, and timestamps.
- **Success:** `200` with `status` and `data` array.
- **Failure:** `500` unexpected database failure.

## Incidents

### List incidents

- **Method/route:** `GET /incidents`
- **Authentication:** None
- **Purpose:** Return all incidents, newest first.
- **Success:** `200` with `status` and `data` array.
- **Failure:** `500` unexpected database failure.

### Create incident

- **Method/route:** `POST /incidents`
- **Authentication:** JWT
- **Body:** Required string `title`, `description`, and `severity`; optional string `location`.
- **Validation:** Title and description must be non-empty. Severity is one of `LOW`, `MEDIUM`, `HIGH`, or `CRITICAL`.
- **Side effect:** Inserts an incident with default status `OPEN`.
- **Success:** `201` with the created incident under `data`.
- **Failures:** `400` missing/invalid fields; `401` invalid authentication; `500` unexpected failure.

```json
{"title":"Gate turnaround delay","description":"Inbound equipment arrived late","severity":"MEDIUM","location":"Gate A4"}
```

### Resolve incident

- **Method/route:** `PATCH /incidents/{id}/resolve`
- **Authentication:** JWT
- **Validation:** `id` must contain decimal digits.
- **Side effect:** Changes status to `RESOLVED` and records `resolved_at`.
- **Success:** `200` with message `Incident resolved` and the updated incident.
- **Failures:** `400` invalid ID; `401` invalid authentication; `404` incident absent; `409` incident already resolved; `500` unexpected failure.

## Weather

### List recent weather

- **Method/route:** `GET /weather`
- **Authentication:** None
- **Purpose:** Return up to the 10 latest reports, newest first.
- **Success:** `200` with `status` and `data` array.
- **Failure:** `500` unexpected database failure.

### Create weather report

- **Method/route:** `POST /weather`
- **Authentication:** JWT
- **Body:** `condition`, `visibility_km`, `wind_speed_kmh`, and `temperature_c`.
- **Validation:** Condition must be non-empty; visibility and wind speed must not be negative.
- **Defaults:** Repository inserts `wind_direction=VRB` and `pressure_hpa=1013.25` through schema defaults.
- **Success:** `201` with the created report under `data`.
- **Failures:** `400` missing/invalid input; `401` invalid authentication; `500` unexpected failure.

```json
{"condition":"Clear","visibility_km":12.5,"wind_speed_kmh":8.2,"temperature_c":21.3}
```

## Agent and conversation history

### Query agent

- **Method/route:** `POST /agent/query`
- **Authentication:** JWT
- **Purpose:** Start a conversation or continue an owned conversation through Gemini and registered tools.
- **Body:** Required non-blank string `query`; optional integer or decimal-string `conversation_id`.
- **Business rules:** A supplied conversation must belong to the authenticated user. The loop has a bounded number of iterations. User and final assistant messages are persisted.
- **Success:** `200`.

```json
{
  "status":"success",
  "conversation_id":"12",
  "query":"What flights are delayed?",
  "answer":"...",
  "tools_used":["find_delayed_flights"]
}
```

- **Failures:** `400` missing/blank query or invalid conversation ID; `401` invalid authentication; `403` conversation not owned (including inaccessible IDs); `500` persistence/internal failure; `502` Gemini request failed. Raw provider details are never returned.

### List conversation history

- **Method/route:** `GET /agent/history`
- **Authentication:** JWT
- **Purpose:** List only the authenticated user's conversations, newest activity first.
- **Success:** `200` with `status` and `conversations`. Entries include `id`, `user_id`, `title`, `created_at`, `message_count`, and `last_message_at`.
- **Failures:** `401` invalid authentication; `500` unexpected database failure.

### Get conversation messages

- **Method/route:** `GET /agent/conversations/{id}/messages`
- **Authentication:** JWT
- **Purpose:** Return chronological messages for one owned conversation.
- **Success:** `200` with `status`, `conversation_id`, and `messages`.
- **Failures:** `401` invalid authentication; `403` conversation absent or not owned; `500` unexpected database failure.

Each message includes `id`, `conversation_id`, `role`, `content`, and `created_at`. Long-term history stores visible user and assistant turns. Temporary provider tool-call objects stay within the active agent iteration so invalid standalone tool messages are not replayed later.
