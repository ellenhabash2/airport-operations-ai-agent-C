export interface User {
  id: number;
  username: string;
  email: string;
  role?: string;
  created_at?: string | null;
}

export interface LoginResponse {
  status: "success";
  token: string;
  user: User;
}

export interface RegisterResponse {
  status: "success";
  message: string;
  data: User;
}

export interface Flight {
  id: number;
  flight_number: string;
  airline?: string | null;
  airline_name: string | null;
  aircraft?: string | null;
  aircraft_registration?: string | null;
  aircraft_type?: string | null;
  gate?: string | null;
  gate_id?: number | null;
  gate_number?: string | null;
  terminal?: string | null;
  terminal_id?: number | null;
  runway?: string | null;
  runway_id?: number | null;
  runway_code?: string | null;
  origin: string;
  destination: string;
  departure_time: string | null;
  arrival_time: string | null;
  scheduled_departure?: string | null;
  scheduled_arrival?: string | null;
  estimated_departure_time?: string | null;
  actual_departure_time?: string | null;
  estimated_arrival_time?: string | null;
  actual_arrival_time?: string | null;
  delay_minutes?: number | null;
  delay_duration_minutes?: number | null;
  delay_reason?: string | null;
  status: string;
  created_at?: string | null;
  updated_at?: string | null;
}

export interface Gate {
  id: number;
  gate_number: string;
  number?: string;
  terminal?: string | null;
  terminal_code?: string | null;
  terminal_id?: number | null;
  status: string;
  available?: boolean;
  assigned_flight?: Flight | null;
}

export interface Terminal {
  id: number;
  name: string;
  code?: string;
  capacity: number;
  total_gates?: number;
  available_gates?: number;
  occupied_gates?: number;
  active_flights?: number;
  available_gate_numbers?: string[];
}

export interface Runway {
  id: number;
  runway_code: string;
  code?: string;
  status: string;
  length?: number;
  length_meters?: number;
  surface?: string | null;
  current_usage?: string | null;
  closure_reason?: string | null;
  affected_flights?: Flight[];
  updated_at?: string | null;
}

export interface Incident {
  id: number;
  title: string;
  description: string;
  severity: string;
  location: string;
  status?: string | null;
  active?: boolean;
  resolved?: boolean;
  created_at: string | null;
  resolved_at?: string | null;
}

export interface WeatherReport {
  id: number;
  condition: string;
  visibility?: number;
  visibility_km?: number;
  wind_speed?: number;
  wind_speed_kmh?: number;
  wind_direction?: string | null;
  temperature?: number;
  temperature_c?: number;
  pressure_hpa?: number;
  created_at: string | null;
  recorded_at?: string | null;
  observed_at?: string | null;
}

export interface ListResponse<T> { status?: string; data: T[]; count?: number; }
export interface ItemResponse<T> { status?: string; data: T; }

export interface ToolExecution {
  tool: string;
  status: "success" | "error";
  arguments: Record<string, unknown>;
  duration_ms: number;
  call_id?: string;
  sequence?: number;
  error_code?: string;
  access?: "read" | "write";
}

export interface FlightListPresentation { type: "flight_list"; data: { flights: Flight[] }; }
export interface FlightStatusPresentation { type: "flight_status"; data: { flight: Flight }; }
export interface GateAssignmentPresentation { type: "gate_assignment"; data: { flight: Flight; previous_gate: Gate | null; new_gate: Gate }; }
export interface RunwayStatusPresentation { type: "runway_status"; data: { runways: Runway[]; affected_flights: Flight[] }; }
export interface IncidentListPresentation { type: "incident_list"; data: { incidents: Incident[] }; }
export interface OperationsOverviewPresentation { type: "operations_overview"; data: { delayed_flights: Flight[]; active_incidents: Incident[]; weather: WeatherReport | null }; }
export type AgentPresentation = FlightListPresentation | FlightStatusPresentation | GateAssignmentPresentation | RunwayStatusPresentation | IncidentListPresentation | OperationsOverviewPresentation;

export interface AgentAnswer {
  status: "success";
  answer: string;
  conversation_id: number;
  tools_used: string[];
  tool_executions: ToolExecution[];
  presentation: AgentPresentation | null;
}

export interface ConversationSummary {
  id: number;
  title: string;
  message_count: number;
  created_at: string | null;
  last_message_at?: string | null;
  updated_at?: string | null;
}

export interface StoredMessage {
  id: number;
  conversation_id?: number;
  role: "user" | "assistant";
  content: string | null;
  created_at: string | null;
  presentation?: AgentPresentation | null;
  tools_used?: string[];
  tool_executions?: ToolExecution[];
  turn_status?: string;
}

export interface ConversationListResponse { status: "success"; conversations: ConversationSummary[]; }
export interface ConversationMessagesResponse { status: "success"; conversation_id: string | number; messages: StoredMessage[]; }

const object = (value: unknown): value is Record<string, unknown> => typeof value === "object" && value !== null && !Array.isArray(value);
const arraysForType: Record<AgentPresentation["type"], string[]> = {
  flight_list: ["flights"], flight_status: [], gate_assignment: [], runway_status: ["runways", "affected_flights"],
  incident_list: ["incidents"], operations_overview: ["delayed_flights", "active_incidents"],
};

export function isAgentPresentation(value: unknown): value is AgentPresentation {
  if (!object(value) || typeof value.type !== "string" || !object(value.data) || !(value.type in arraysForType)) return false;
  const type = value.type as AgentPresentation["type"];
  const data = value.data;
  if (!arraysForType[type].every((key) => Array.isArray(data[key]))) return false;
  if (type === "flight_status") return object(data.flight);
  if (type === "gate_assignment") return object(data.flight) && object(data.new_gate) && (data.previous_gate === null || object(data.previous_gate));
  if (type === "operations_overview") return data.weather === null || object(data.weather);
  return true;
}

export function isToolExecution(value: unknown): value is ToolExecution {
  return object(value) && typeof value.tool === "string" && (value.status === "success" || value.status === "error") && object(value.arguments) && typeof value.duration_ms === "number";
}

export function parseAgentAnswer(value: unknown): AgentAnswer {
  if (!object(value) || value.status !== "success" || typeof value.answer !== "string" || typeof value.conversation_id !== "number") throw new Error("Invalid agent response");
  const executions = Array.isArray(value.tool_executions) ? value.tool_executions.filter(isToolExecution) : [];
  return { status: "success", answer: value.answer, conversation_id: value.conversation_id,
    tools_used: Array.isArray(value.tools_used) ? value.tools_used.filter((tool): tool is string => typeof tool === "string") : [],
    tool_executions: executions, presentation: isAgentPresentation(value.presentation) ? value.presentation : null };
}
