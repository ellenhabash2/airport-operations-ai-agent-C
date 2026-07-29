const API_URL = import.meta.env.VITE_API_BASE_URL ?? import.meta.env.VITE_API_URL ?? "http://localhost:8848";
const TOKEN_KEY = "aeromind_token";

/** Raised for any non-2xx response or unreachable server. */
export class ApiError extends Error {
  status: number;
  retryable: boolean;

  constructor(message: string, status: number, retryable = false) {
    super(message);
    this.name = "ApiError";
    this.status = status;
    this.retryable = retryable;
  }
}

export function getToken(): string | null {
  return localStorage.getItem(TOKEN_KEY);
}

export function setToken(token: string | null): void {
  if (token === null) {
    localStorage.removeItem(TOKEN_KEY);
  } else {
    localStorage.setItem(TOKEN_KEY, token);
  }
}

interface RequestOptions {
  method?: string;
  body?: unknown;
  signal?: AbortSignal;
}

function normalizeEntity(value: unknown): unknown {
  if (Array.isArray(value)) return value.map(normalizeEntity);
  if (typeof value !== "object" || value === null) return value;
  const item = Object.fromEntries(Object.entries(value).map(([key, entry]) => [key, normalizeEntity(entry)])) as Record<string, unknown>;
  if (typeof item.status === "string") item.status = item.status.toLowerCase();
  if (typeof item.severity === "string") item.severity = item.severity.toLowerCase();
  if (item.gate_number === undefined && typeof item.gate === "string") item.gate_number = item.gate || null;
  if (item.runway_code === undefined && typeof item.runway === "string") item.runway_code = item.runway || null;
  if (item.terminal === undefined && typeof item.terminal_code === "string") item.terminal = item.terminal_code;
  if (item.length === undefined && typeof item.length_meters === "number") item.length = item.length_meters;
  if (item.visibility === undefined && typeof item.visibility_km === "number") item.visibility = item.visibility_km;
  if (item.wind_speed === undefined && typeof item.wind_speed_kmh === "number") item.wind_speed = item.wind_speed_kmh;
  if (item.temperature === undefined && typeof item.temperature_c === "number") item.temperature = item.temperature_c;
  return item;
}

async function request<T>(path: string, options: RequestOptions = {}): Promise<T> {
  const headers: Record<string, string> = {};

  if (options.body !== undefined) {
    headers["Content-Type"] = "application/json";
  }

  const token = getToken();

  if (token) {
    headers["Authorization"] = `Bearer ${token}`;
  }

  let response: Response;

  try {
    response = await fetch(`${API_URL}${path}`, {
      method: options.method ?? "GET",
      headers,
      body: options.body === undefined ? undefined : JSON.stringify(options.body),
      signal: options.signal,
    });
  } catch {
    throw new ApiError("Cannot reach the AeroMind API. Check that the server is running.", 0);
  }

  const payload = await response.json().catch(() => null);

  if (!response.ok) {
    // An expired or invalid token should log the user out everywhere.
    if (response.status === 401) {
      setToken(null);
      window.dispatchEvent(new Event("aeromind:unauthorized"));
    }

    const message =
      payload?.message ?? payload?.error ?? `Request failed with status ${response.status}.`;

    throw new ApiError(message, response.status, payload?.retryable === true);
  }

  return normalizeEntity(payload) as T;
}

export const api = {
  get: <T>(path: string, signal?: AbortSignal) => request<T>(path, { signal }),
  post: <T>(path: string, body?: unknown, signal?: AbortSignal) => request<T>(path, { method: "POST", body, signal }),
  patch: <T>(path: string, body?: unknown, signal?: AbortSignal) => request<T>(path, { method: "PATCH", body, signal }),
  delete: <T>(path: string, signal?: AbortSignal) => request<T>(path, { method: "DELETE", signal }),
};
