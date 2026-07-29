import { render } from "@testing-library/react";
import type { ReactElement } from "react";
import { MemoryRouter } from "react-router-dom";

import { AuthProvider } from "../context/AuthContext";
import type { User } from "../types/api";
import { mockUser } from "./fixtures";

export function renderWithRouter(
  ui: ReactElement,
  { route = "/" }: { route?: string } = {},
) {
  return render(
    <MemoryRouter initialEntries={[route]}>
      {ui}
    </MemoryRouter>,
  );
}

export function renderWithProviders(
  ui: ReactElement,
  {
    route = "/",
    user = mockUser,
  }: { route?: string; user?: User | null } = {},
) {
  if (user) {
    localStorage.setItem("aeromind_user", JSON.stringify(user));
    localStorage.setItem("aeromind_token", "test-token");
  }

  return render(
    <MemoryRouter initialEntries={[route]}>
      <AuthProvider>{ui}</AuthProvider>
    </MemoryRouter>,
  );
}
