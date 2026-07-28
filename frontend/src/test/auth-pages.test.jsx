import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { MemoryRouter, Route, Routes } from "react-router-dom";
import { beforeEach, describe, expect, it, vi } from "vitest";
import LoginPage from "../pages/LoginPage";
import RegisterPage from "../pages/RegisterPage";
import ProtectedRoute from "../components/ProtectedRoute";
import { login, register } from "../services/authService";

vi.mock("../services/authService", () => ({ login: vi.fn(), register: vi.fn() }));
vi.mock("../services/api", () => ({ default: { get: vi.fn() } }));

function renderAt(element, path = "/") {
  return render(
    <MemoryRouter initialEntries={[path]}>
      <Routes>
        <Route path={path} element={element} />
        <Route path="/" element={<div>Login destination</div>} />
        <Route path="/overview" element={<div>Overview destination</div>} />
      </Routes>
    </MemoryRouter>,
  );
}

beforeEach(() => vi.clearAllMocks());

describe("authentication pages", () => {
  it("logs in, stores the session, and navigates", async () => {
    login.mockResolvedValue({ token: "jwt", user: { id: "7", username: "ops" } });
    renderAt(<LoginPage />);
    await userEvent.type(screen.getByPlaceholderText("Enter your email"), "ops@example.com");
    await userEvent.type(screen.getByPlaceholderText("Enter your password"), "secret1");
    await userEvent.click(screen.getByRole("button", { name: "Login" }));
    await waitFor(() => expect(screen.getByText("Overview destination")).toBeInTheDocument());
    expect(login).toHaveBeenCalledWith("ops@example.com", "secret1");
    expect(localStorage.getItem("token")).toBe("jwt");
  });

  it("shows login API errors", async () => {
    login.mockRejectedValue({ response: { data: { error: "Invalid email or password" } } });
    renderAt(<LoginPage />);
    await userEvent.type(screen.getByPlaceholderText("Enter your email"), "ops@example.com");
    await userEvent.type(screen.getByPlaceholderText("Enter your password"), "wrong");
    await userEvent.click(screen.getByRole("button", { name: "Login" }));
    expect(await screen.findByText("Invalid email or password")).toBeInTheDocument();
  });

  it("registers and returns to login", async () => {
    register.mockResolvedValue({ status: "success" });
    renderAt(<RegisterPage />, "/register");
    await userEvent.type(screen.getByPlaceholderText("Enter your username"), "operator");
    await userEvent.type(screen.getByPlaceholderText("Enter your email"), "ops@example.com");
    await userEvent.type(screen.getByPlaceholderText("Create a password"), "secret1");
    await userEvent.click(screen.getByRole("button", { name: "Register" }));
    await waitFor(() => expect(screen.getByText("Login destination")).toBeInTheDocument());
    expect(register).toHaveBeenCalledWith("operator", "ops@example.com", "secret1");
  });

  it("shows registration API errors", async () => {
    register.mockRejectedValue({ response: { data: { error: "Email already exists" } } });
    renderAt(<RegisterPage />, "/register");
    await userEvent.type(screen.getByPlaceholderText("Enter your username"), "operator");
    await userEvent.type(screen.getByPlaceholderText("Enter your email"), "ops@example.com");
    await userEvent.type(screen.getByPlaceholderText("Create a password"), "secret1");
    await userEvent.click(screen.getByRole("button", { name: "Register" }));
    expect(await screen.findByText("Email already exists")).toBeInTheDocument();
  });
});

describe("ProtectedRoute", () => {
  it("redirects anonymous users", () => {
    renderAt(<ProtectedRoute><div>Private content</div></ProtectedRoute>, "/private");
    expect(screen.getByText("Login destination")).toBeInTheDocument();
    expect(screen.queryByText("Private content")).not.toBeInTheDocument();
  });

  it("renders children for authenticated users", () => {
    localStorage.setItem("token", "jwt");
    renderAt(<ProtectedRoute><div>Private content</div></ProtectedRoute>, "/private");
    expect(screen.getByText("Private content")).toBeInTheDocument();
  });
});
