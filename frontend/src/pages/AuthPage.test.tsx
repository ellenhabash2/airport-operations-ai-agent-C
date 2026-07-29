import { screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { api, ApiError } from "../api/client";
import { mockUser } from "../test/fixtures";
import { renderWithProviders } from "../test/render";
import AuthPage from "./AuthPage";

const postMock = vi.mocked(api.post);

describe("AuthPage", () => {
  beforeEach(() => {
    postMock.mockReset();
  });

  it("submits the expected login payload through the existing auth flow", async () => {
    postMock.mockResolvedValue({ status: "success", token: "private-test-token", user: mockUser } as never);
    const user = userEvent.setup();
    renderWithProviders(<AuthPage mode="login" />, { route: "/login", user: null });

    await user.type(screen.getByLabelText("Username or email"), "operator");
    await user.type(screen.getByLabelText("Password"), "password123");
    await user.click(screen.getByRole("button", { name: "Sign in to console" }));

    expect(postMock).toHaveBeenCalledWith("/auth/login", {
      email: "operator",
      password: "password123",
    });
    expect(screen.queryByText("private-test-token")).not.toBeInTheDocument();
  });

  it("does not submit an obviously empty login form", async () => {
    const user = userEvent.setup();
    renderWithProviders(<AuthPage mode="login" />, { route: "/login", user: null });

    await user.click(screen.getByRole("button", { name: "Sign in to console" }));

    expect(postMock).not.toHaveBeenCalled();
  });

  it("renders authentication errors clearly", async () => {
    postMock.mockRejectedValue(new ApiError("Invalid credentials", 401));
    const user = userEvent.setup();
    renderWithProviders(<AuthPage mode="login" />, { route: "/login", user: null });

    await user.type(screen.getByLabelText("Username or email"), "operator");
    await user.type(screen.getByLabelText("Password"), "wrong-password");
    await user.click(screen.getByRole("button", { name: "Sign in to console" }));

    expect(await screen.findByText("Invalid credentials")).toBeVisible();
    expect(screen.queryByText("[object Object]")).not.toBeInTheDocument();
  });
});
