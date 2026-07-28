#!/usr/bin/env bash
set -euo pipefail

if [[ "${AEROMIND_TEST_LIVE_AI:-0}" != "1" ]]; then
    echo "Live AI test skipped. Set AEROMIND_TEST_LIVE_AI=1 to enable it."
    exit 0
fi
if [[ -z "${GEMINI_API_KEY:-}" ]]; then
    echo "Live AI test cannot run: GEMINI_API_KEY is not configured." >&2
    exit 1
fi
if [[ -z "${AEROMIND_LIVE_TOKEN:-}" ]]; then
    echo "Live AI test cannot run: AEROMIND_LIVE_TOKEN is not configured." >&2
    exit 1
fi

backend_url="${AEROMIND_BACKEND_URL:-http://localhost:8848}"
response_file="$(mktemp)"
trap 'rm -f "$response_file"' EXIT

status="$(curl --silent --show-error --output "$response_file" --write-out '%{http_code}' \
    --header "Authorization: Bearer ${AEROMIND_LIVE_TOKEN}" \
    --header "Content-Type: application/json" \
    --data '{"query":"Reply with a short airport operations readiness status."}' \
    "${backend_url}/agent/query")"

if [[ "$status" != "200" ]] || ! grep -q '"answer"' "$response_file"; then
    echo "Live Gemini smoke test failed with sanitized HTTP status ${status}." >&2
    exit 1
fi
echo "Live Gemini smoke test passed."
