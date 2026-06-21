# AeroMind API

Base URL: `http://localhost:8848`

All responses are JSON. Errors use:

```json
{
  "error": "Message"
}
```

## Health

### GET `/health`

Response `501`:

```json
{
  "status": "ok",
  "service": "AeroMind"
}
```

## Authentication Placeholders

### POST `/auth/register`

Request:

```json
{
  "username": "operator",
  "email": "operator@example.com",
  "password": "secret"
}
```

Response `501`:

```json
{
  "status": "planned",
  "message": "User registration is reserved for the authentication phase",
  "phase": "Foundation Phase"
}
```

### POST `/auth/login`

Request:

```json
{
  "username": "operator",
  "password": "secret"
}
```

Response `200`:

```json
{
  "status": "planned",
  "message": "User login is reserved for the authentication phase",
  "phase": "Foundation Phase"
}
```

## Flights

### GET `/flights`

Response `200`:

```json
{
  "status": "success",
  "data": [
    {
      "id": "1",
      "flight_number": "BA201",
      "airline": "BA",
      "aircraft": "G-STBA",
      "gate": "A1",
      "runway": "09L/27R",
      "origin": "JFK",
      "destination": "LAX",
      "status": "SCHEDULED",
      "departure_time": "2026-06-20 05:18:00",
      "arrival_time": "2026-06-20 07:33:00"
    }
  ]
}
```

Response `500`:

```json
{
  "error": "Database connection is not available."
}
```

### GET `/flights/{id}`

Response `200`: one flight object.

Response `404`:

```json
{
  "error": "Flight not found"
}
```

Response `400`:

```json
{
  "error": "Flight ID must be a positive integer"
}
```

## Gates

### GET `/gates`

Response `200`:

```json
{
  "status": "success",
  "data": [
    {
      "id": "1",
      "gate_number": "A1",
      "terminal_id": "1",
      "terminal_code": "T1",
      "status": "AVAILABLE"
    }
  ]
}
```

## Runways

### GET `/runways`

Response `200`:

```json
{
  "status": "success",
  "data": [
    {
      "id": "1",
      "runway_code": "09L/27R",
      "status": "OPERATIONAL",
      "length_meters": 4105,
      "surface": "ASPHALT"
    }
  ]
}
```

## Incidents

### GET `/incidents`

Response `200`:

```json
{
  "status": "success",
  "data": [
    {
      "id": "30",
      "title": "Weather Flow Restriction",
      "description": "Operational event 30 recorded by AeroMind seed data for airport control center workflows.",
      "severity": "MEDIUM",
      "location": "Control Tower",
      "status": "RESOLVED",
      "created_at": "2026-06-20 10:00:00"
    }
  ]
}
```

### POST `/incidents`

Request:

```json
{
  "title": "Gate Delay",
  "description": "Gate A4 unavailable due to jet bridge maintenance",
  "severity": "HIGH",
  "location": "Terminal 1"
}
```

Response `201`: created incident object.

Response `400`:

```json
{
  "error": "Missing required fields: title, description, severity"
}
```

## Weather

### GET `/weather`

Response `200`:

```json
{
  "status": "success",
  "data": [
    {
      "id": "30",
      "condition": "Partly Cloudy",
      "visibility_km": 10.7,
      "wind_speed_kmh": 6,
      "wind_direction": "SW",
      "temperature_c": 21.3,
      "pressure_hpa": 1011.8,
      "created_at": "2026-06-20 10:00:00"
    }
  ]
}
```

### POST `/weather`

Request:

```json
{
  "condition": "Clear",
  "visibility_km": 10.0,
  "wind_speed_kmh": 8.5,
  "temperature_c": 22.3
}
```

Response `201`: created weather report object.

Response `400`:

```json
{
  "error": "Missing required fields: condition, visibility_km, wind_speed_kmh, temperature_c"
}
```

## Agent Placeholders

### POST `/agent/query`

Request:

```json
{
  "message": "Which delayed flights need gate reassignment?"
}
```

Response `501`:

```json
{
  "status": "planned",
  "message": "AI Agent querying is reserved for the Gemini and agentic loop phase",
  "phase": "Foundation Phase",
  "todo": [
    "Implement Gemini API integration",
    "Define AI function tools",
    "Build Agentic Loop"
  ]
}
```

### GET `/agent/history`

Response `501`:

```json
{
  "status": "planned",
  "message": "Chat history retrieval is reserved for the conversation memory phase",
  "phase": "Foundation Phase"
}
```

## Agent Placeholders

### POST `/agent/query`

Request:

```json
{
  "query": "Which delayed flights need gate reassignment?"
}
```

Response `200`: planned future agent response.

### GET `/agent/history`

Response `200`: planned future chat history response.
