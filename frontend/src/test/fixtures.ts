import type {
  ConversationMessagesResponse,
  Flight,
  Gate,
  Incident,
  Runway,
  Terminal,
  ToolExecution,
  User,
  WeatherReport,
} from "../types/api";

export const mockUser: User = {
  id: 7,
  username: "operator",
  email: "operator@airport.test",
  created_at: "2026-07-22T08:00:00+00:00",
};

export const mockToolCalls: ToolExecution[] = [
  {
    tool: "get_flight_by_number",
    arguments: { flight_number: "PW2018" },
    status: "success",
    duration_ms: 12,
  },
  {
    tool: "get_available_gates",
    arguments: { terminal: "A" },
    status: "error",
    duration_ms: 8,
    error_code: "service_unavailable",
  },
];

export const mockFlight: Flight = {
  id: 18,
  flight_number: "PW2018",
  airline_name: "Palestinian Wings",
  aircraft_registration: "E4-PW18",
  aircraft_type: "Airbus A320",
  gate_number: "A04",
  terminal: "Terminal A",
  runway_code: "08L/26R",
  origin: "AMM",
  destination: "CDG",
  departure_time: "2026-07-22T10:30:00+00:00",
  arrival_time: "2026-07-22T14:35:00+00:00",
  status: "delayed",
};

export const mockGate: Gate = {
  id: 4,
  gate_number: "A04",
  terminal: "Terminal A",
  status: "occupied",
};

export const mockRunway: Runway = {
  id: 1,
  runway_code: "08L/26R",
  status: "available",
  length: 4100,
};

export const mockTerminal: Terminal = {
  id: 1,
  name: "Terminal A",
  capacity: 18000,
  total_gates: 1,
  available_gates: 0,
  available_gate_numbers: [],
};

export const mockWeather: WeatherReport = {
  id: 1,
  condition: "partly cloudy",
  visibility: 8.5,
  wind_speed: 12,
  temperature: 24,
  created_at: "2026-07-22T09:00:00+00:00",
};

export const mockIncident: Incident = {
  id: 3,
  title: "PW2018 baggage inspection",
  description: "Inspection affecting flight PW2018.",
  severity: "medium",
  location: "Gate A04",
  status: "open",
  created_at: "2026-07-22T09:15:00+00:00",
};

export const mockConversation: ConversationMessagesResponse = {
  status: "success",
  conversation_id: 12,
  messages: [
    {
      id: 1,
      role: "user",
      content: "Check flight PW2018",
      created_at: "2026-07-22T08:00:00+00:00",
    },
    {
      id: 2,
      role: "assistant",
      content: "PW2018 is delayed.\nGate staff have been notified.",
      tool_executions: mockToolCalls,
      created_at: "2026-07-22T08:01:00+00:00",
    },
    {
      id: 3,
      role: "assistant",
      content: "This is an older message.",
      created_at: "2026-07-22T08:02:00+00:00",
    },
  ],
};
