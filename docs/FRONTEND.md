# Frontend

The AeroMind frontend is a strict TypeScript React application that communicates only with the C++ Drogon REST API.

## Development

```bash
cd frontend
cp .env.example .env.local
npm ci
npm run dev
```

`VITE_API_BASE_URL` defaults to `http://localhost:8848`. Only the public API origin belongs in frontend environment files. Gemini, JWT, and database secrets must never use a `VITE_` variable.

Available checks are `npm run typecheck`, `npm run lint`, `npm run test:run`, and `npm run build`.

## Architecture

- `src/api/client.ts`: fetch, JWT attachment, 401 handling, safe errors, and C++ field normalization.
- `src/types/api.ts`: domain types, six Phase 8 presentation variants, tool execution types, and runtime guards.
- `src/context/AuthContext.tsx`: token/user persistence and `/auth/me` verification.
- `src/pages`: authentication, dashboard, operations map, and chat.
- `src/components/structured-answers`: cards rendered only from backend `presentation`.
- `src/components/tool-execution`: safe timeline rendered only from backend `tool_executions`.

Assistant `answer` Markdown always remains visible. Raw HTML is disabled, and presentations are never reconstructed from prose. Invalid structured data falls back to the textual answer.
