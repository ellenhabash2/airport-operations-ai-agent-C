import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { MemoryRouter } from "react-router-dom";
import { beforeEach, describe, expect, it, vi } from "vitest";
import ChatPage from "../pages/ChatPage";
import api from "../services/api";

vi.mock("../services/api", () => ({ default: { get: vi.fn(), post: vi.fn() } }));

function renderChat() {
  return render(<MemoryRouter><ChatPage /></MemoryRouter>);
}

beforeEach(() => {
  vi.clearAllMocks();
  api.get.mockResolvedValue({ data: { conversations: [] } });
});

describe("ChatPage", () => {
  it("sends a message and displays the assistant response", async () => {
    api.post.mockResolvedValue({ data: { conversation_id: "12", answer: "Flight AM101 is delayed." } });
    renderChat();
    await userEvent.type(screen.getByPlaceholderText(/Ask anything/), "Any delays?");
    await userEvent.click(screen.getByRole("button", { name: "Send message" }));
    expect(await screen.findByText("Flight AM101 is delayed.")).toBeInTheDocument();
    expect(api.post).toHaveBeenCalledWith("/agent/query", { query: "Any delays?" });
  });

  it("shows loading while a chat request is pending", async () => {
    let resolveRequest;
    api.post.mockImplementation(() => new Promise((resolve) => { resolveRequest = resolve; }));
    renderChat();
    await userEvent.type(screen.getByPlaceholderText(/Ask anything/), "Weather?");
    await userEvent.keyboard("{Enter}");
    expect(screen.getByText(/AeroMind is thinking/i)).toBeInTheDocument();
    resolveRequest({ data: { answer: "Clear" } });
    await screen.findByText("Clear");
  });

  it("renders a friendly provider error", async () => {
    api.post.mockRejectedValue({ response: { status: 502 } });
    renderChat();
    await userEvent.type(screen.getByPlaceholderText(/Ask anything/), "Status?");
    await userEvent.keyboard("{Enter}");
    expect(await screen.findByText(/AI service is temporarily unavailable/)).toBeInTheDocument();
  });

  it("loads and selects conversation history", async () => {
    api.get.mockImplementation((url) => url === "/agent/history"
      ? Promise.resolve({ data: { conversations: [{ id: "8", title: "Morning ops", created_at: "2026-07-01" }] } })
      : Promise.resolve({ data: { messages: [{ id: "1", role: "user", content: "Saved question" }] } }));
    renderChat();
    await userEvent.click(await screen.findByRole("button", { name: /Morning ops/ }));
    expect(await screen.findByText("Saved question")).toBeInTheDocument();
    expect(api.get).toHaveBeenCalledWith("/agent/conversations/8/messages");
    expect(sessionStorage.getItem("selectedConversationId")).toBe("8");
  });

  it("starts a new chat and clears the saved selection", async () => {
    sessionStorage.setItem("selectedConversationId", "8");
    api.get.mockResolvedValueOnce({ data: { conversations: [] } }).mockResolvedValueOnce({ data: { messages: [] } });
    renderChat();
    await waitFor(() => expect(api.get).toHaveBeenCalled());
    await userEvent.click(screen.getByRole("button", { name: /New Chat/ }));
    expect(sessionStorage.getItem("selectedConversationId")).toBeNull();
    expect(screen.getByText(/Hello! I'm AeroMind/)).toBeInTheDocument();
  });
});
