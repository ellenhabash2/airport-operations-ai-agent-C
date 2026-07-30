import { screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { api, ApiError } from "../api/client";
import { renderWithProviders } from "../test/render";
import ChatPage from "./ChatPage";

const getMock = vi.mocked(api.get);
const postMock = vi.mocked(api.post);

describe("ChatPage Drogon integration", () => {
  beforeEach(() => {
    getMock.mockImplementation((path) => {
      if (path === "/agent/history") return Promise.resolve({ status: "success", conversations: [] }) as never;
      return Promise.reject(new Error(`Unexpected GET ${path}`));
    });
  });

  it("uses the C++ query field and renders answer, presentation, and execution", async () => {
    postMock.mockResolvedValue({ status: "success", answer: "**One delayed flight.**", conversation_id: 12,
      tools_used: ["find_delayed_flights"], tool_executions: [{ tool: "find_delayed_flights", status: "success", arguments: {}, duration_ms: 7, sequence: 1 }],
      presentation: { type: "flight_list", data: { flights: [] } } } as never);
    renderWithProviders(<ChatPage />, { route: "/chat" });
    const user = userEvent.setup();
    await user.type(screen.getByLabelText("Message AeroMind"), "Which flights are delayed?");
    await user.click(screen.getByLabelText("Send message"));
    await screen.findByText("One delayed flight.");
    expect(postMock).toHaveBeenCalledWith("/agent/query", { query: "Which flights are delayed?", conversation_id: null });
    expect(screen.getByText("Find delayed flights")).toBeVisible();
    expect(screen.getByText("0 results")).toBeVisible();
  });

  it("loads C++ conversation history and stored metadata", async () => {
    getMock.mockImplementation((path) => {
      if (path === "/agent/history") return Promise.resolve({ status: "success", conversations: [{ id: 12, title: "Flight status", message_count: 2, created_at: null, last_message_at: null }] }) as never;
      if (path === "/agent/conversations/12/messages") return Promise.resolve({ status: "success", conversation_id: 12, messages: [{ id: 1, role: "user", content: "Status?", created_at: null }, { id: 2, role: "assistant", content: "Scheduled.", created_at: null, presentation: null, tool_executions: [] }] }) as never;
      return Promise.reject(new Error(`Unexpected GET ${path}`));
    });
    renderWithProviders(<ChatPage />, { route: "/chat" });
    await userEvent.click(await screen.findByText("Flight status"));
    expect(await screen.findByText("Scheduled.")).toBeVisible();
    expect(getMock).toHaveBeenCalledWith("/agent/conversations/12/messages");
  });

  it("shows a safe error when the backend rejects a request", async () => {
    postMock.mockRejectedValue(new Error("database details"));
    renderWithProviders(<ChatPage />, { route: "/chat" });
    await userEvent.type(screen.getByLabelText("Message AeroMind"), "status");
    await userEvent.click(screen.getByLabelText("Send message"));
    await waitFor(() => expect(screen.getByRole("alert")).toHaveTextContent("could not be completed"));
    expect(screen.queryByText("database details")).not.toBeInTheDocument();
  });

  it("shows the AI provider message for provider gateway errors", async () => {
    postMock.mockRejectedValue(new ApiError("AI provider is currently unavailable", 502));
    renderWithProviders(<ChatPage />, { route: "/chat" });
    await userEvent.type(screen.getByLabelText("Message AeroMind"), "status");
    await userEvent.click(screen.getByLabelText("Send message"));
    await waitFor(() => expect(screen.getByRole("alert")).toHaveTextContent("AI provider is currently unavailable"));
  });
});
