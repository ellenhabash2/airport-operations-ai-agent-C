import { act, screen, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { api } from "../api/client";
import {
  mockFlight,
  mockGate,
  mockRunway,
  mockTerminal,
} from "../test/fixtures";
import { renderWithRouter } from "../test/render";
import OperationsMapPage from "./OperationsMapPage";

const getMock = vi.mocked(api.get);

function mockMapData({ failFlights = false } = {}) {
  getMock.mockImplementation((path) => {
    if (path === "/flights" && failFlights) {
      return Promise.reject(new Error("Flight feed unavailable"));
    }

    const responses: Record<string, unknown> = {
      "/flights": { data: [mockFlight] },
      "/gates": { data: [mockGate] },
      "/runways": { data: [mockRunway] },
      "/terminals": { data: [mockTerminal] },
    };

    return Promise.resolve(responses[path]) as never;
  });
}

describe("OperationsMapPage", () => {
  beforeEach(() => {
    getMock.mockReset();
  });

  it("renders terminals, their gates, statuses, and runways", async () => {
    mockMapData();
    renderWithRouter(<OperationsMapPage />, { route: "/operations-map" });

    expect(await screen.findByRole("heading", { name: "Terminal A" })).toBeVisible();
    const gate = screen.getByRole("button", { name: /Gate A04, Occupied/ });
    expect(within(gate).getByText("PW2018")).toBeVisible();
    expect(screen.getByText("08L/26R")).toBeVisible();
    expect(screen.getByText("Open")).toBeVisible();
  });

  it("opens the correct gate details view", async () => {
    mockMapData();
    const user = userEvent.setup();
    renderWithRouter(<OperationsMapPage />, { route: "/operations-map" });

    await user.click(await screen.findByRole("button", { name: /Gate A04, Occupied/ }));

    expect(screen.getByRole("dialog", { name: "Gate A04" })).toBeVisible();
    expect(screen.getByText("Assigned flight")).toBeVisible();
  });

  it("shows loading and then supports empty terminal and runway states", async () => {
    let resolveRequests!: (value: { data: [] }) => void;
    getMock.mockReturnValue(
      new Promise((resolve) => {
        resolveRequests = resolve;
      }) as never,
    );
    renderWithRouter(<OperationsMapPage />, { route: "/operations-map" });

    expect(screen.getByLabelText("Loading airport map")).toBeVisible();
    await act(async () => resolveRequests({ data: [] }));

    expect(await screen.findByText("No terminals to display")).toBeVisible();
    expect(screen.getByText("No runways to display")).toBeVisible();
  });

  it("keeps partial map data visible when one resource fails", async () => {
    mockMapData({ failFlights: true });
    renderWithRouter(<OperationsMapPage />, { route: "/operations-map" });

    expect(await screen.findByText("Flights unavailable.")).toBeVisible();
    expect(screen.getByRole("heading", { name: "Terminal A" })).toBeVisible();
    expect(screen.getByRole("button", { name: /Gate A04, Occupied/ })).toBeVisible();
    expect(screen.getByText("08L/26R")).toBeVisible();
  });
});
