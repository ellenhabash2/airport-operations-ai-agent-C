import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import ToolExecutionTimeline from "./ToolExecutionTimeline";

describe("ToolExecutionTimeline", () => {
  it("renders Phase 8 fields in sequence and hides secrets", () => {
    render(<ToolExecutionTimeline executions={[{ tool: "assign_flight_to_gate", status: "success", arguments: { gate_id: 3, api_key: "hidden" }, duration_ms: 12, sequence: 2 }, { tool: "get_flight_by_number", status: "error", arguments: {}, duration_ms: 5, sequence: 1, error_code: "not_found" }]} />);
    const statuses = screen.getAllByRole("status");
    expect(statuses[0]).toHaveTextContent("Get flight by number");
    expect(statuses[1]).toHaveTextContent("Assign flight to gate");
    expect(screen.getByText("write")).toBeVisible();
    expect(screen.getByText("12 ms")).toBeVisible();
    expect(screen.queryByText("hidden")).not.toBeInTheDocument();
  });

  it("renders nothing without executions", () => {
    const { container } = render(<ToolExecutionTimeline />);
    expect(container).toBeEmptyDOMElement();
  });
});
