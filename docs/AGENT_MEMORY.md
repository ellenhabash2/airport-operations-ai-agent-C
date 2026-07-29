# Rich conversation memory

AeroMind persists every provider-visible message instead of reducing a turn to
user text and final prose. A server-generated `turn_id` groups the user message,
assistant tool calls, matching tool results, and final assistant message.
`provider_payload` stores the replayable OpenAI-compatible message used by the
configured Gemini endpoint. Normalized tool fields, future-ready presentation
data, and safe metadata are stored separately.

Replay verifies ownership, selects the newest complete turn groups, and restores
all messages chronologically. `AGENT_HISTORY_MAX_TURNS` defaults to 30 and is
clamped to 1–100. Tool-call IDs, arguments, results, and ordering are preserved.
Legacy text-only rows become ordinary user or assistant messages. Invalid
provider payloads fall back to validated normalized fields, while incomplete
structured turns are excluded.

Writes are incremental and marked `in_progress`, `completed`, or `failed`. This
retains safe diagnostic structure after provider/tool failure without replaying
an incomplete call sequence. Concurrent requests share no mutable conversation
buffer; database timestamp plus ID provides deterministic row ordering.

API keys, JWTs, authorization/raw HTTP headers, database credentials, SQL errors,
and stack traces are never stored. Public history omits `provider_payload`.

Existing databases apply the idempotent, data-preserving upgrade with:

```bash
psql "$DATABASE_URL" -f sql/upgrades/phase7_rich_conversation_memory.sql
```

