import { describe, expect, it } from "vitest";
import { isAgentPresentation, isToolExecution, parseAgentAnswer } from "./api";

describe("agent API validation", () => {
  it("accepts every documented presentation shape", () => {
    const values = [
      { type: "flight_list", data: { flights: [] } },
      { type: "flight_status", data: { flight: {} } },
      { type: "gate_assignment", data: { flight: {}, previous_gate: null, new_gate: {} } },
      { type: "runway_status", data: { runways: [], affected_flights: [] } },
      { type: "incident_list", data: { incidents: [] } },
      { type: "operations_overview", data: { delayed_flights: [], active_incidents: [], weather: null } },
    ];
    expect(values.every(isAgentPresentation)).toBe(true);
  });

  it("rejects unknown and structurally invalid presentations", () => {
    expect(isAgentPresentation({ type: "unknown", data: {} })).toBe(false);
    expect(isAgentPresentation({ type: "flight_list", data: { flights: {} } })).toBe(false);
    expect(isAgentPresentation({ type: "gate_assignment", data: { flight: {}, previous_gate: null } })).toBe(false);
  });

  it("falls back to text when optional metadata is malformed", () => {
    const result = parseAgentAnswer({ status: "success", answer: "Safe text", conversation_id: 7,
      tools_used: ["get_all_flights", 12], tool_executions: [{ tool: "bad", status: "success", arguments: [], duration_ms: 1 }],
      presentation: { type: "flight_list", data: { flights: "not-an-array" } } });
    expect(result.answer).toBe("Safe text");
    expect(result.tools_used).toEqual(["get_all_flights"]);
    expect(result.tool_executions).toEqual([]);
    expect(result.presentation).toBeNull();
  });

  it("rejects malformed required response fields and tool executions", () => {
    expect(() => parseAgentAnswer({ status: "error" })).toThrow("Invalid agent response");
    expect(isToolExecution({ tool: "x", status: "success", arguments: {}, duration_ms: 0 })).toBe(true);
    expect(isToolExecution({ tool: "x", status: "failed", arguments: {}, duration_ms: 0 })).toBe(false);
  });
});
