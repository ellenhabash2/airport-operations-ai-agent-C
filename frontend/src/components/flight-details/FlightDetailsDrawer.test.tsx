import { screen, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { api } from "../../api/client";
import {
  mockFlight,
  mockGate,
  mockIncident,
  mockRunway,
  mockWeather,
} from "../../test/fixtures";
import { renderWithRouter } from "../../test/render";
import FlightDetailsDrawer from "./FlightDetailsDrawer";

const getMock = vi.mocked(api.get);

function mockDrawerData() {
  getMock.mockImplementation((path) => {
    const responses: Record<string, unknown> = {
      [`/flights/${mockFlight.id}`]: { data: mockFlight },
      "/gates": { data: [mockGate] },
      "/runways": { data: [mockRunway] },
      "/weather": { data: [mockWeather] },
      "/incidents": { data: [mockIncident] },
    };

    return Promise.resolve(responses[path]) as never;
  });
}

describe("FlightDetailsDrawer", () => {
  beforeEach(() => {
    getMock.mockReset();
    mockDrawerData();
  });

  it("renders current flight details and missing schedule fallbacks", async () => {
    renderWithRouter(
      <FlightDetailsDrawer
        flightId={mockFlight.id}
        initialFlight={mockFlight}
        onClose={vi.fn()}
      />,
    );

    const dialog = screen.getByRole("dialog", { name: "PW2018" });
    expect(within(dialog).getAllByText("Palestinian Wings").length).toBeGreaterThan(0);
    expect(within(dialog).getByText("Airbus A320")).toBeVisible();
    expect(within(dialog).getByText("E4-PW18")).toBeVisible();
    expect(within(dialog).getAllByText("Not Available").length).toBeGreaterThan(0);
    expect(await within(dialog).findByText(/partly cloudy/i)).toBeVisible();
    expect(within(dialog).getByText(mockIncident.title)).toBeVisible();
  });

  it("closes from both the close button and Escape", async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    renderWithRouter(
      <FlightDetailsDrawer
        flightId={mockFlight.id}
        initialFlight={mockFlight}
        onClose={onClose}
      />,
    );
    const dialog = screen.getByRole("dialog");

    await user.click(within(dialog).getByRole("button", { name: "Close flight details" }));
    expect(onClose).toHaveBeenCalledTimes(1);

    await user.keyboard("{Escape}");
    expect(onClose).toHaveBeenCalledTimes(2);
  });

  it("copies the flight number and links to a prefilled unsent chat prompt", async () => {
    const user = userEvent.setup();
    const writeText = vi
      .spyOn(navigator.clipboard, "writeText")
      .mockResolvedValue(undefined);
    renderWithRouter(
      <FlightDetailsDrawer
        flightId={mockFlight.id}
        initialFlight={mockFlight}
        onClose={vi.fn()}
      />,
    );

    await user.click(screen.getByRole("button", { name: "Copy flight number" }));
    expect(writeText).toHaveBeenCalledWith("PW2018");
    const askLink = screen.getByRole("link", { name: "Ask AeroMind" });
    expect(askLink).toHaveAttribute(
      "href",
      "/chat?prompt=Explain%20the%20current%20operational%20status%20of%20flight%20PW2018.",
    );
  });

  it("shows a loading skeleton when only a flight identifier is provided", async () => {
    getMock.mockReturnValue(new Promise(() => undefined) as never);
    renderWithRouter(
      <FlightDetailsDrawer flightId={mockFlight.id} onClose={vi.fn()} />,
    );

    expect(screen.getByLabelText("Loading flight details")).toBeVisible();
    expect(screen.queryByText("Palestinian Wings")).not.toBeInTheDocument();
  });
});
