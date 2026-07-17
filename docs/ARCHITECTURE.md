# AeroMind Architecture

AeroMind is organized as a layered C++ backend foundation for future AI-powered airport operations.

## Layers

### HTTP Controllers

Controllers in `backend/controllers/` own request validation, HTTP status codes, and JSON responses. They do not contain SQL.

Current controllers:

- `HealthController`
- `AuthController`
- `FlightController`
- `GateController`
- `RunwayController`
- `IncidentController`
- `WeatherController`
- `AgentController`

### Repository Layer

Repositories in `backend/repositories/` own SQL queries and translate PostgreSQL rows into JSON payloads for the foundation APIs.

Current repositories:

- `FlightRepository`
- `GateRepository`
- `RunwayRepository`
- `IncidentRepository`
- `WeatherRepository`

### Database Layer

`DatabaseManager` validates PostgreSQL connectivity at startup and opens scoped repository connections from the configured connection string. The Docker Compose stack injects database host, port, name, user, and password through environment variables.

### Domain Models

`backend/models/` contains lightweight C++ structs for the core airport operations domain:

- User, Airline, Aircraft, Terminal, Gate, Runway
- Flight, Crew, Incident, WeatherReport
- Conversation, Message

### Future Agent Layer

`backend/agent/` defines future AI interfaces:

- `AgentService`
- `LLMClient`
- `ToolRegistry`
- `PromptBuilder`

These are intentionally interfaces only in this foundation phase. Later phases should add concrete implementations that call Gemini, compose prompts, register airport operations tools, and run an agentic loop.

### Future Security Layer

`backend/security/` defines:

- `JwtService`
- `PasswordHasher`

These reserve the authentication design without implementing JWT signing or password hashing in the foundation phase.

## Request Flow

Typical data request:

1. Drogon routes request to a controller.
2. Controller validates request shape and calls a repository.
3. Repository obtains the PostgreSQL connection from `DatabaseManager`.
4. Repository executes parameterized SQL when input is involved.
5. Controller returns success JSON or an error JSON with the right HTTP status.

## Planned Agent Flow

Future AI request flow:

1. `AgentController` accepts a user query.
2. `AgentService` builds context using `PromptBuilder`.
3. `ToolRegistry` exposes airport operations tools.
4. `LLMClient` calls Gemini.
5. Agent loop selects tools, executes repository-backed operations, and persists messages.
6. Conversation history is stored in `conversations` and `messages`.

## Docker Flow

`docker-compose.yml` starts PostgreSQL first, runs schema and seed files from `sql/`, waits for database health, then starts the Drogon backend on port `8848`.
