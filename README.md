# AeroMind - Airport Operations AI Agent

AeroMind is a C++20 Drogon backend foundation for an airport operations AI agent. This phase establishes the production-style backend structure, PostgreSQL schema, Docker runtime, REST endpoints, repository layer, and documented extension points for later AI, authentication, and tool-calling phases.

This foundation intentionally does not implement Gemini integration, function tools, the agentic loop, JWT authentication, password hashing, or a React frontend.

## Architecture

The backend follows a layered architecture:

- Controllers handle HTTP requests and JSON responses.
- Repositories own SQL access and keep SQL out of controllers.
- DatabaseManager centralizes PostgreSQL connectivity.
- Models define lightweight domain contracts for airport operations data.
- Agent and security interfaces reserve clean extension points for future phases.

## Tech Stack

- C++20
- Drogon
- PostgreSQL 16
- libpqxx
- jsoncpp
- CMake
- Docker and Docker Compose

## Setup

Build locally when Drogon, jsoncpp, PostgreSQL headers, and libpqxx are installed:

```bash
cmake -S . -B build
cmake --build build
./build/backend/aeromind_backend
```

Run the complete stack with Docker:

```bash
docker compose up --build
```

The backend listens on `http://localhost:8848`, and PostgreSQL listens on `localhost:5432`.

Reset seeded data:

```bash
docker compose down -v
docker compose up --build
```

## Environment

Copy `.env.example` when running locally and adjust values as needed.

Required variables:

- `DB_HOST`
- `DB_PORT`
- `DB_NAME`
- `DB_USER`
- `DB_PASSWORD`
- `PORT`

## Endpoints

| Method | Endpoint | Purpose |
| --- | --- | --- |
| GET | `/health` | Service health check |
| POST | `/auth/register` | Authentication phase placeholder |
| POST | `/auth/login` | Authentication phase placeholder |
| GET | `/flights` | List seeded operational flights |
| GET | `/flights/{id}` | Get one flight by ID |
| GET | `/gates` | List gates and terminal status |
| GET | `/runways` | List runway status |
| GET | `/incidents` | List recent incidents |
| POST | `/incidents` | Create an incident |
| GET | `/weather` | List latest weather reports |
| POST | `/weather` | Create a weather report |
| POST | `/agent/query` | AI agent phase placeholder |
| GET | `/agent/history` | Conversation memory phase placeholder |

Health response:

```json
{
  "status": "ok",
  "service": "AeroMind"
}
```

## Database Overview

The PostgreSQL foundation includes:

- 13 tables: users, airlines, aircraft, terminals, gates, runways, flights, crew, flight_crew, weather_reports, incidents, conversations, messages
- Primary keys, foreign keys, checks, uniqueness constraints, and operational indexes
- Seed data with 5 airlines, 25 aircraft, 3 terminals, 36 gates, 3 runways, 150 flights, 20 crew, 30 weather reports, and 30 incidents

See [docs/DATABASE.md](docs/DATABASE.md) for the table-by-table reference.

## Future AI Phases

Future phases should implement:

- Gemini API client and prompt construction
- Function tools for flight, gate, runway, weather, and incident operations
- Agentic planning and tool execution loop
- JWT authentication and authorization middleware
- Secure password hashing
- Persistent chat history behavior
- Optional React operations console
