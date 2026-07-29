import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import StructuredAnswerRenderer from "./StructuredAnswerRenderer";

const flight = { id: 18, flight_number: "SB2101", airline_name: "SkyBridge Airways", status: "scheduled", origin: "SFO", destination: "AMI", departure_time: "2026-06-18T12:06:00Z", arrival_time: "2026-06-18T18:52:00Z", terminal: "A", gate_number: "A03", runway_code: "08L/26R" };

describe("StructuredAnswerRenderer", () => {
  it("renders flight status from the Phase 8 nested flight contract", () => {
    render(<StructuredAnswerRenderer presentation={{ type: "flight_status", data: { flight } }} />);
    expect(screen.getByRole("heading", { name: "SB2101" })).toBeVisible();
    expect(screen.getByText("SkyBridge Airways")).toBeVisible();
  });

  it("renders all list-based presentation contracts", () => {
    const { rerender } = render(<StructuredAnswerRenderer presentation={{ type: "flight_list", data: { flights: [flight] } }} />);
    expect(screen.getByText("1 results")).toBeVisible();
    rerender(<StructuredAnswerRenderer presentation={{ type: "incident_list", data: { incidents: [] } }} />);
    expect(screen.getByText("No incidents found.")).toBeVisible();
    rerender(<StructuredAnswerRenderer presentation={{ type: "runway_status", data: { runways: [{ id: 1, runway_code: "08L/26R", status: "closed" }], affected_flights: [] } }} />);
    expect(screen.getByText("08L/26R")).toBeVisible();
  });

  it("renders committed gate assignments", () => {
    render(<StructuredAnswerRenderer presentation={{ type: "gate_assignment", data: { flight, previous_gate: null, new_gate: { id: 3, gate_number: "A03", terminal_code: "A", status: "occupied" } } }} />);
    expect(screen.getByText("Assignment committed in Terminal A.")).toBeVisible();
  });

  it("renders operations overview partial values safely", () => {
    render(<StructuredAnswerRenderer presentation={{ type: "operations_overview", data: { delayed_flights: [flight], active_incidents: [], weather: null } }} />);
    expect(screen.getByText("Operations overview")).toBeVisible();
    expect(screen.getByText("Unavailable")).toBeVisible();
  });
});
