# AeroMind deployment guide

## Local Docker architecture

Docker Compose runs:

- `postgres`: PostgreSQL 16 Alpine, persistent `postgres_data` volume, schema/seed initialization, readiness health check.
- `backend`: multi-stage Ubuntu image containing the C++ backend and runtime libraries, dependent on healthy PostgreSQL, exposed on port `8848`.

The React frontend is run locally with Vite and is not currently packaged as a Compose service.

## Configuration

Copy `.env.example` to `.env`. Set a long random `JWT_SECRET`, database credentials, and a Gemini key for live AI. `AI_PROVIDER` must be `gemini`; the default model is `gemini-2.5-flash`; the default endpoint is Google's OpenAI-compatible chat-completions URL. Never commit `.env`.

## Build and start

```bash
cp .env.example .env
# Edit placeholders before continuing.
docker compose config
docker compose build
docker compose up -d
docker compose ps
```

PostgreSQL must become healthy before Compose starts the backend. The backend health check calls `GET /health`.

## Frontend

```bash
cd frontend
npm ci
npm run dev -- --host 0.0.0.0
```

Open `http://localhost:5173`. The Vite development proxy forwards `/api` to `http://localhost:8848`.

## Logs and rebuilds

```bash
docker compose logs -f backend
docker compose logs -f postgres
docker compose build backend
docker compose up -d backend
```

The `.dockerignore` excludes Git metadata, secrets, local builds, logs, `node_modules`, and frontend output from the backend build context.

## Stop and persistence

```bash
docker compose down
```

This preserves `postgres_data`. The destructive reset below deletes the local Compose database volume and should be used only when intentionally rebuilding seed data:

```bash
docker compose down -v
docker compose up -d
```

## Local backend build

```bash
cmake -S . -B build -DAEROMIND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake pins libpqxx 7.7.3 to avoid distribution packages with mismatched header/library ABIs. Initial configuration needs network access for missing FetchContent dependencies.

## Troubleshooting

### Port already in use

Check `8848`, `5173`, and `5432` with your platform's port tools. Stop the conflicting process/container or change the published host port and matching frontend proxy.

### Database is not ready

```bash
docker compose ps
docker compose logs --tail=200 postgres
```

Confirm database credentials agree and the volume is writable. Compose waits for `pg_isready`, but corrupted or incompatible persisted data requires diagnosis before any reset.

### Missing JWT secret

Compose configuration fails when `JWT_SECRET` is unset. Copy `.env.example`, replace the placeholder with a long random value, and recreate the backend. Local token generation also fails safely without the variable.

### Missing Gemini key

The server and non-agent endpoints can start, but `/agent/query` returns a controlled provider failure until `GEMINI_API_KEY` is configured. Recreate the backend after changing `.env`.

### Gemini status errors

- `401`: key missing, invalid, or revoked.
- `403`: project, API, region, or policy access denied.
- `404`: configured model unavailable or misspelled.
- `429`: project quota/rate limit exhausted; limited retries occur.
- Temporary `500`, `502`, `503`, and `504`: provider failure; limited retries occur.

Model access is never guaranteed and depends on the Google AI Studio project.

### Frontend cannot reach backend

Confirm `docker compose ps`, `curl http://localhost:8848/health`, Vite's `/api` proxy, and browser developer-network output. The backend development CORS policy allows `http://localhost:5173`; another frontend origin requires an intentional configuration change.

### CORS error

Use the documented Vite origin and methods. Preflight permits `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, and `OPTIONS` with `Content-Type` and `Authorization` headers.

### Registration failure

Username must be at least two characters, password at least six, and email syntactically plausible. Duplicate email or username returns `409`. Inspect sanitized backend logs for unexpected `500` responses.

### Docker build failure

Confirm network access to Ubuntu and GitHub dependency sources, adequate disk space, and Docker builder permissions. Use `docker compose --progress plain build` for full logs. Do not disable TLS verification.

## Production considerations

The provided configuration is for local academic demonstration. Production deployment would require TLS termination, non-default database credentials, managed secret storage, stricter network exposure, configurable CORS, role authorization, rate limiting, monitoring, backups, asynchronous provider work, and a formal migration process.
