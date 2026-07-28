# AeroMind quick start

## Prerequisites

- Docker Engine and Docker Compose
- Node.js and npm for the frontend
- A Google AI Studio key only for live agent queries

## Start the backend and database

```bash
cp .env.example .env
# Replace JWT_SECRET and other placeholder values in .env.
docker compose build
docker compose up -d
docker compose ps
curl http://localhost:8848/health
```

Expected health is HTTP 200 with `status: ok` and `database: connected`.

## Start the frontend

```bash
cd frontend
npm ci
npm run dev -- --host 0.0.0.0
```

Open `http://localhost:5173` and register a new user or sign in with an account in the initialized database.

## Useful checks

```bash
curl http://localhost:8848/flights
curl http://localhost:8848/flights/delayed
curl http://localhost:8848/gates
curl http://localhost:8848/runways
curl http://localhost:8848/incidents
curl http://localhost:8848/weather
docker compose logs --tail=200 backend
```

Protected requests require `Authorization: Bearer <token>`. Full examples are in [docs/API.md](docs/API.md).

## Stop

```bash
docker compose down
```

This preserves the database volume. `docker compose down -v` is destructive and should be used only for an intentional seed reset.

## Tests

```bash
cmake -S . -B build -DAEROMIND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure

cd frontend
npm run lint
npm run test:run
npm run build
```

Automated tests do not contact Gemini. See [docs/TESTING.md](docs/TESTING.md) for the optional live smoke test.
