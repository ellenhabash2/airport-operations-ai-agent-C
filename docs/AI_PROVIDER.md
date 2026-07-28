# Gemini AI provider

AeroMind uses Google Gemini through Google's OpenAI-compatible Chat
Completions endpoint. This preserves the application's OpenAI-format messages,
function definitions, assistant tool calls, and tool-result messages without a
Gemini SDK dependency.

## Configuration

Create an API key in Google AI Studio, copy `.env.example` to `.env`, and set:

```dotenv
AI_PROVIDER=gemini
GEMINI_API_KEY=replace_with_your_google_ai_studio_key
GEMINI_MODEL=gemini-2.5-flash
GEMINI_BASE_URL=https://generativelanguage.googleapis.com/v1beta/openai/chat/completions
```

Never commit `.env` or an actual API key. Model availability, supported
regions, and quotas depend on the Google project associated with the AI Studio
key and are not guaranteed by AeroMind.

## Request and function-calling flow

`LLMClient` sends a non-streaming OpenAI-compatible request containing the
configured model, conversation messages, ToolRegistry definitions, and
`"tool_choice":"auto"`. TLS certificate validation is enabled and each request
has a 30-second timeout.

For function calls, AeroMind appends Gemini's complete assistant message to the
current turn, executes every requested tool, and appends one `role: tool`
message per result. Each result keeps the exact `tool_call_id` supplied by
Gemini. The loop supports multiple calls in one response and up to five tool
iterations before requesting a final summary without tools.

## Error behavior

- `400`: request or tool schema rejected
- `401`: invalid Gemini API key
- `403`: Google project or regional access denied
- `404`: configured model unavailable
- `429`: quota or rate limit reached; retried with limited backoff
- `500`, `502`, `503`, `504`: temporary provider error; retried

Timeouts and network failures are also retried within the configured limit.
Provider bodies and credentials are never returned to frontend users.

## Testing

Automated tests inject a fake HTTP transport and never require an API key or
contact Google. Run them using the commands in [TESTING.md](TESTING.md).

An explicit live check is available only when enabled:

```bash
AEROMIND_TEST_LIVE_AI=1 \
GEMINI_API_KEY=your_key \
AEROMIND_LIVE_TOKEN=your_aeromind_jwt \
scripts/live_ai_smoke.sh
```

The script reports only a sanitized status and does not print credentials or
the provider response.

## Migration note

Earlier AeroMind revisions used Groq's OpenAI-compatible endpoint and a Llama
model. Runtime support now uses Gemini configuration exclusively; the message
and function-calling contract remains OpenAI-compatible.
