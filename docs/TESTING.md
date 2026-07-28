# AeroMind testing

Flight service tests use injected repository functions and tool-registry tests
use fake tool handlers, so neither reaches Gemini or the development database.
They cover lookup, status canonicalization, search handoff, update behavior,
gate-conflict mapping, and registration of the complete flight tool set.

## Testing strategy

The repository has deterministic backend unit tests, frontend component tests, Docker health verification, and an explicit live-provider smoke test. Normal tests never contact Gemini and do not require `GEMINI_API_KEY`.

The current suite does not contain a separate controller/API integration executable or an isolated PostgreSQL test database. Those remain known gaps rather than claimed coverage.

## Backend tests

Enable tests at configure time:

```bash
cmake -S . -B build -DAEROMIND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Phase 3 adds gate ID/number validation, availability rules, terminal lookup/status/empty-flight behavior, gate and terminal tool schemas, and a deterministic three-tool flight-terminal scenario. Fake services/tools are used; no Gemini request occurs.

The exact final CTest count is recorded by the final verification run rather than maintained as a historical total here.

| Group | Cases | Behavior |
| --- | ---: | --- |
| `PasswordHasherTest` | 4 | Non-plaintext bcrypt output, correct/incorrect passwords, unique salts |
| `JwtServiceTest` | 6 | Generation, user ID, tampering, wrong signature, expiration, required secret |
| `ToolRegistryTest` | current suite | Unique tools, flight/gate/terminal schemas, unknown dispatch, fake execution |
| `AgentLoopTest` | current suite | No-tool response, multi-tool chains including flight/terminal reasoning, invalid arguments, provider failure, bounds, and tool-call ID/order preservation |
| `LLMConfigTest` | 3 | Gemini defaults, environment overrides, HTTPS requirement |
| `LLMClientTest` and parameterized status cases | 12 | Request serialization, text/tool parsing, auth header, retries, transport failures, malformed responses, offline configuration, 400/401/403/404 mapping |

CTest displays each GoogleTest case separately through `gtest_discover_tests`.

## Fake provider and tools

`LLMClientTest` injects `FakeTransport`, which records URL, headers, body, timeout, and scripted responses. `AgentLoopTest` scripts provider messages in memory. `backend/tests/fakes/FakeTools.cpp` supplies the tool symbols without connecting to PostgreSQL. This design protects credentials and quota while making retries and multi-tool sequences repeatable.

## Database and integration coverage

The automated backend target does not currently start PostgreSQL, reset schema state, or exercise controller routes over HTTP. Database behavior is validated through schema constraints, manual/Compose smoke checks, and direct application use. Normal development data is therefore not modified by CTest.

High-value future tests are:

- registration/login HTTP integration
- flight and incident controller validation/status codes
- incident create/resolve transactions
- conversation ownership across two users
- repository/schema compatibility in a disposable database

## Frontend tests

The frontend uses Vitest, jsdom, React Testing Library, and user-event.

```bash
cd frontend
npm ci
npm run lint
npm run test:run
npm run build
```

The verified suite contains 11 tests in two files:

- `auth-pages.test.jsx` (6): login success/failure and registration validation/success paths.
- `chat-page.test.jsx` (5): message response, pending UI, provider-friendly error, history selection, and new-chat reset.

`npm run lint` checks the source with ESLint. `npm run build` verifies Vite's production bundle. No coverage percentage is reported because no coverage command was run.

## Docker verification

```bash
JWT_SECRET=replace_with_a_long_random_secret_at_least_32_chars \
GEMINI_API_KEY=placeholder \
docker compose config
docker compose --progress plain build
docker compose up -d
docker compose ps
docker compose logs --tail=200 backend
```

Use placeholders only for configuration rendering/building. A real key is required only for a live agent request.

## Smoke testing

The repository currently provides `scripts/live_ai_smoke.sh`, not a general offline `smoke_test.sh`. Without explicit enablement it exits successfully after reporting that the live test was skipped:

```bash
./scripts/live_ai_smoke.sh
```

To run a real request after login:

```bash
AEROMIND_TEST_LIVE_AI=1 \
GEMINI_API_KEY='<configured-key>' \
AEROMIND_LIVE_TOKEN='<AeroMind JWT>' \
./scripts/live_ai_smoke.sh
```

The script checks required variables without printing them, sends one authenticated `/agent/query`, validates HTTP 200 and an `answer` field, and reports only sanitized success/failure. Do not enable it in normal automation.

## Observed cleanup baseline

Before final cleanup, the host executable failed to link because the installed libpqxx headers expected symbols absent from the packaged binary. One JWT tamper test was intermittently ineffective because changing the final base64url character can preserve decoded bytes. Frontend lint reported 14 errors and one warning. These findings drove the pinned libpqxx build, deterministic signature mutation, and frontend cleanup.

## Known gaps

- No automated HTTP controller/database integration suite
- No dedicated disposable PostgreSQL test database
- No measured coverage percentage
- No browser end-to-end runner
- No automated accessibility or load testing
- Live Gemini behavior is intentionally opt-in and quota-dependent

## Dependency audit note

`npm audit --omit=dev` reports a high-severity React Router advisory affecting React Server Components action handling. AeroMind is a client-only Vite application: it does not enable RSC mode or server actions, so the vulnerable path is not used. The audit's proposed change is outside the installed version range and should be evaluated as a dedicated dependency upgrade rather than forced during final cleanup. Development-tool advisories reported by the full audit likewise require a separate compatibility review.
