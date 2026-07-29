import { createContext, useCallback, useContext, useEffect, useState } from "react";
import type { ReactNode } from "react";

import { api, getToken, setToken } from "../api/client";
import type { LoginResponse, RegisterResponse, User } from "../types/api";

const USER_KEY = "aeromind_user";

interface AuthContextValue {
  user: User | null;
  loading: boolean;
  login: (identifier: string, password: string) => Promise<void>;
  register: (username: string, email: string, password: string) => Promise<void>;
  logout: () => void;
}

const AuthContext = createContext<AuthContextValue | null>(null);

function readStoredUser(): User | null {
  const raw = localStorage.getItem(USER_KEY);

  if (!raw) {
    return null;
  }

  try {
    return JSON.parse(raw) as User;
  } catch {
    localStorage.removeItem(USER_KEY);
    return null;
  }
}

export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<User | null>(readStoredUser);
  const [loading, setLoading] = useState(() => getToken() !== null);

  const logout = useCallback(() => {
    setToken(null);
    localStorage.removeItem(USER_KEY);
    setUser(null);
  }, []);

  // The API layer fires this when the server rejects our token.
  useEffect(() => {
    window.addEventListener("aeromind:unauthorized", logout);
    return () => window.removeEventListener("aeromind:unauthorized", logout);
  }, [logout]);

  useEffect(() => {
    if (!getToken()) return;
    api.get<{ status: "success"; authenticated: boolean }>("/auth/me")
      .then((result) => { if (!result.authenticated) logout(); })
      .catch(() => logout())
      .finally(() => setLoading(false));
  }, [logout]);

  const login = useCallback(async (identifier: string, password: string) => {
    const data = await api.post<LoginResponse>("/auth/login", {
      email: identifier,
      password,
    });

    setToken(data.token);
    localStorage.setItem(USER_KEY, JSON.stringify(data.user));
    setUser(data.user);
  }, []);

  const register = useCallback(
    async (username: string, email: string, password: string) => {
      await api.post<RegisterResponse>("/auth/register", {
        username,
        email,
        password,
      });

      await login(email, password);
    },
    [login],
  );

  return (
    <AuthContext.Provider value={{ user, loading, login, register, logout }}>
      {children}
    </AuthContext.Provider>
  );
}

// eslint-disable-next-line react-refresh/only-export-components
export function useAuth(): AuthContextValue {
  const context = useContext(AuthContext);

  if (context === null) {
    throw new Error("useAuth must be used inside an AuthProvider.");
  }

  return context;
}
