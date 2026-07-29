# AeroMind architecture

## Flight-operation consistency

Flight REST handlers and Gemini tools both use `FlightService`; only
`FlightRepository` contains flight SQL. Gate assignment locks the flight,
target gate, and different previous gate in one PostgreSQL transaction,
rechecks target availability under lock, and updates all three states before a
single commit. Failure rolls back the whole operation; same-gate assignment is
idempotent.

## Architectural goals

AeroMind favors visible separation of responsibilities, small testable agent components, provider isolation, safe registered tools, persistent user-owned memory, and deterministic simulated data. It deliberately avoids a large framework or live airport dependency for an academic demonstration.

## Backend layers

### Application entry point

`backend/main.cpp` reads database/server configuration, initializes the connection pool, configures the Drogon worker threads and restricted development CORS origin, then starts the HTTP server. Blocking provider calls are isolated from a single-thread event loop by using multiple Drogon workers.

### Controllers

`backend/controllers` declares all 17 routes. Controllers parse transport inputs, call domain services, map domain errors to HTTP status codes, and preserve public JSON contracts. `JwtAuthFilter` is attached declaratively to protected routes. Authentication is the sole current controller exception: it uses `UserRepository` directly because there is no corresponding AI tool or duplicated user-domain workflow in this phase.

### Domain services

`backend/services` owns current business policy and supplies the shared boundary used by REST controllers and AI tools:

- `FlightService` owns flight identifier validation and not-found behavior.
- `GateService` owns gate ID/number validation, all/available lookups, and the shared availability and operational-status rules.
- `TerminalService` owns terminal existence validation, terminal status, and flights-by-terminal workflows.
- `RunwayService` exposes current runway status.
- `IncidentService` owns incident input, severity, search, ID validation, and resolution outcomes, including the already-resolved conflict. REST endpoints and all six incident tools share this service.
- `WeatherService` owns current weather input validation and lookup/create operations.
- `ConversationService` owns creation, ownership checks, ordered history access, and visible-message persistence.
- `AgentService` coordinates conversation context, `AgentLoop`, and visible turn persistence.

Dependencies are explicit function objects, allowing service tests to use deterministic fakes without PostgreSQL. Terminal aggregates and flight joins remain in `TerminalRepository`.

### Agent layer

- `LLMConfig` reads provider-neutral runtime configuration for the active Gemini provider.
- `LLMClient` serializes OpenAI-compatible requests and handles TLS HTTP, bounded responses, retries, and sanitized errors.
- `AgentLoop` coordinates provider iterations, assistant tool calls, matching tool result IDs, multiple calls, and bounded iterations.
- `ToolRegistry` is the only name-to-function dispatch boundary. One deterministic definition map owns every tool's name, description, JSON Schema, read/write access, and handler, and generates provider declarations from that metadata.

`AgentService` coordinates conversation context and persistence around `AgentLoop`; it does not absorb provider/tool iteration behavior.

### Tool implementations

`backend/tools` adapts agent-facing arguments and delegates to the same domain services as REST. `ToolRegistry` performs shared shape validation, requires authenticated context for writes, rejects duplicate names, and converts unknown or unexpected failures to controlled errors. Neither layer contains SQL.

### Repositories

`backend/repositories` owns parameterized SQL, atomic database primitives, and libpqxx row-to-JSON mapping. The incident repository atomically reads and updates resolution state; `IncidentService` decides that an already-resolved result is a conflict. Conversation message reads join through `conversations.user_id`, providing defense-in-depth ownership enforcement.

### Database manager

`DatabaseManager` is a process-wide singleton connection pool. Initialization is synchronized, callers borrow shared connections, and repository transactions manage commits and rollback through RAII.

### Security

- `PasswordHasher` creates unique bcrypt salts and verifies password hashes.
- `JwtService` creates 24-hour HS256 tokens from an environment-only secret and verifies issuer, signature, and expiration.
- `JwtAuthFilter` rejects requests without a valid `Bearer` token.

### Models

The terminal domain uses typed `Terminal` and `TerminalStatus` models with shared JSON serializers. Existing flight and gate response contracts remain JsonCpp-based for compatibility.

### Flight-terminal reasoning flow

```mermaid
flowchart TD
    User --> Gemini
    Gemini --> GF[get_flight_by_number]
    GF --> FlightService
    FlightService --> FlightRepository
    FlightRepository --> DB[(PostgreSQL)]
    Gemini --> TS[get_terminal_status]
    TS --> TerminalService
    TerminalService --> TerminalRepository
    TerminalRepository --> DB
    Gemini --> TF[get_flights_by_terminal]
    TF --> TerminalService
```

## Frontend architecture

The React/JavaScript application uses React Router for public and protected pages. `ProtectedRoute` checks local token presence; the login page verifies existing sessions against `/auth/me`. `services/api.js` owns the `/api` Axios base URL and injects the bearer token. `authService.js` wraps login and registration.

`OverviewPage` loads flight, gate, runway, weather, and incident resources independently so one failure does not erase all dashboard state. `ChatPage` manages query submission, selected conversation persistence in session storage, conversation history loading, provider-friendly errors, and reusable sidebar/message/input/suggestion components.

## Request flows

### Standard REST request

1. Drogon matches a controller route.
2. The controller validates path/body data.
3. A domain service applies current business policy.
4. A repository executes parameterized SQL through the pool.
5. The controller returns compatible JSON and an appropriate HTTP status.

### Authenticated request

1. `JwtAuthFilter` parses `Authorization: Bearer <token>`.
2. `JwtService` verifies signature, issuer, and expiration.
3. The controller extracts `user_id` where an ownership decision is required.
4. The operation proceeds or returns `401`/`403`.

### Agent and tool sequence

```mermaid
sequenceDiagram
    actor User
    participant Frontend
    participant AgentController
    participant AgentLoop
    participant Gemini
    participant ToolRegistry
    participant Service
    participant Repository
    participant PostgreSQL

    User->>Frontend: Submit query
    Frontend->>AgentController: POST /agent/query + JWT
    AgentController->>Service: Query through AgentService
    Service->>Repository: Verify/create conversation and load history
    Repository->>PostgreSQL: Parameterized queries
    PostgreSQL-->>Repository: Owned conversation data
    AgentController->>AgentLoop: Messages and nine tool schemas
    AgentLoop->>Gemini: Chat completion
    Gemini-->>AgentLoop: Assistant tool_calls
    loop Up to maximum iterations
        AgentLoop->>ToolRegistry: Execute registered name and arguments
        ToolRegistry->>Service: Operational query/action
        Service->>Repository: Repository request
        Repository->>PostgreSQL: Parameterized SQL
        PostgreSQL-->>Repository: Rows/result
        Repository-->>ToolRegistry: JSON result
        ToolRegistry-->>AgentLoop: Tool result + matching call ID
        AgentLoop->>Gemini: Assistant call and tool result messages
        Gemini-->>AgentLoop: More calls or final answer
    end
    AgentLoop-->>AgentController: Answer and tools used
    Service->>Repository: Persist visible turns
    AgentController-->>Frontend: answer, conversation_id, tools_used
    Frontend-->>User: Render answer
```

### Conversation continuation

The optional `conversation_id` can be numeric JSON or a decimal string. `ConversationService` rejects non-owned conversations before loading context. Only stored user and assistant messages are replayed; tool messages require matching assistant call objects from the same provider turn and therefore remain ephemeral.

## Error propagation

- Transport validation errors stop in controllers with `400`; domain validation errors originate in services and are mapped to `400`.
- Authentication and ownership failures return `401` and `403`.
- Missing records return `404`; business conflicts return `409`.
- Repository exceptions are logged server-side and become sanitized `500` responses.
- Provider transport/status/JSON failures become controlled categories in `LLMClient`, then a generic `502` at `/agent/query`.
- Tool validation problems become JSON tool results so Gemini can explain or recover without terminating the entire request.

Logs intentionally omit credentials, authorization headers, full prompt history, passwords, and provider bodies.

## Dependency boundaries

The frontend depends only on AeroMind HTTP contracts. Gemini is behind `LLMClient`; controllers and tools share services; services depend explicitly on repository operations; repositories depend on PostgreSQL. Automated service and agent tests replace repository operations, HTTP transport, and tool execution with fakes, so they remain deterministic and offline.

## Design decisions

- **Drogon:** native C++ HTTP routing, JSON responses, filters, and asynchronous server infrastructure.
- **PostgreSQL/libpqxx:** relational constraints and parameterized queries for operational data and memory.
- **Registered tools:** a narrow allowlist prevents arbitrary code or SQL execution.
- **Simulated data:** predictable demonstrations without paid, unstable, or unavailable airport feeds.
- **Bounded loop:** prevents unlimited provider/tool execution.
- **Mocked Gemini tests:** protects secrets, quota, speed, and repeatability.
- **OpenAI-compatible Gemini endpoint:** preserves the existing message/tool format without a new SDK.

## Trade-offs

Provider requests and repository calls are synchronous, so throughput is limited by the worker pool. JSON row mappings reduce type boilerplate but provide weaker compile-time domain guarantees. Public read endpoints are intentionally open for demonstration, while write and conversation endpoints require a JWT; roles are stored but role-specific authorization is not yet enforced.
