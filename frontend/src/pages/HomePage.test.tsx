import { screen, waitFor, within } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { api, ApiError } from "../api/client";
import {
  mockFlight,
  mockGate,
  mockIncident,
  mockWeather,
} from "../test/fixtures";
import { renderWithProviders } from "../test/render";
import HomePage from "./HomePage";

const getMock = vi.mocked(api.get);

function mockDashboardData() {
  getMock.mockImplementation((path) => {
    const responses: Record<string, unknown> = {
      "/flights": { data: [mockFlight] },
      "/gates": { data: [mockGate, { ...mockGate, id: 5, gate_number: "A05", status: "available" }] },
      "/incidents": { data: [mockIncident] },
      "/weather": { data: [mockWeather] },
    };

    return Promise.resolve(responses[path]) as never;
  });
}

describe("HomePage dashboard states", () => {
  beforeEach(() => {
    getMock.mockReset();
  });

  it("shows loading indicators while API requests are pending", async () => {
    getMock.mockReturnValue(new Promise(() => undefined) as never);
    renderWithProviders(<HomePage />);

    expect(screen.getByRole("button", { name: /Refresh/ })).toBeDisabled();
    expect(screen.getAllByText("Loading…").length).toBeGreaterThan(0);
    expect(screen.getByText("Loading flights…")).toBeVisible();
  });

  it("renders KPI data and operational content", async () => {
    mockDashboardData();
    renderWithProviders(<HomePage />);

    const availableCard = (await screen.findByText("Available gates")).closest("article");
    expect(availableCard).not.toBeNull();
    await waitFor(() =>
      expect(within(availableCard as HTMLElement).getByText("1")).toBeVisible(),
    );
    expect(screen.getByText("PW2018")).toBeVisible();
    expect(screen.getByText("Partly cloudy")).toBeVisible();
    expect(screen.getByText(mockIncident.title)).toBeVisible();
  });

  it("renders readable empty states", async () => {
    getMock.mockResolvedValue({ data: [] } as never);
    renderWithProviders(<HomePage />);

    expect(
      await screen.findByText(/No flights in the database yet/),
    ).toBeVisible();
    expect(screen.getByText("No weather reports recorded yet.")).toBeVisible();
    expect(screen.getByText("No incidents logged.")).toBeVisible();
  });

  it("shows a readable API failure without exposing exception objects", async () => {
    getMock.mockRejectedValue(new ApiError("Operations feed unavailable", 503));
    renderWithProviders(<HomePage />);

    expect(await screen.findByText("Operations feed unavailable")).toBeVisible();
    expect(screen.queryByText("[object Object]")).not.toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Try again" })).toBeEnabled();
  });
});
