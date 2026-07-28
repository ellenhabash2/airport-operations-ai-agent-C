# Automated testing

AeroMind has isolated backend unit tests and frontend component tests. Unit and
component tests do not contact PostgreSQL, the backend HTTP server, or Groq.

## Backend

```sh
cmake -S . -B build -DAEROMIND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`AEROMIND_BUILD_TESTS` defaults to `OFF`, so the production executable and its
normal build remain unchanged. GoogleTest is discovered from the system first;
CMake fetches the pinned version only when it is unavailable.

## Frontend

```sh
cd frontend
npm ci
npm run test:run
npm run build
```

Use `npm test` for Vitest watch mode. Tests use jsdom and mock the shared Axios
client, so they never call a live AeroMind backend.

## Database integration tests

Database integration tests must never run against development data. A future
integration target should require a dedicated connection string (for example
`AEROMIND_TEST_DATABASE_URL`) and isolate or roll back every test case.

## Current coverage

- bcrypt hashing, verification, rejection, and salting
- JWT generation, verification, claims, tampering, signatures, and expiration
- tool declaration uniqueness and schemas, unknown tools, and input validation
- agent-loop behavior with a fake provider, including tool chains, bad
  arguments, provider errors, and iteration limits
- login, registration, route protection, chat send/loading/error states,
  conversation selection, and new-chat state

Not yet automated: dedicated PostgreSQL integration scenarios, live Drogon HTTP
endpoint tests, logout UI (there is no logout control in the current frontend),
and end-to-end browser tests.
