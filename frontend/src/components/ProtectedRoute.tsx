import type { ReactNode } from "react";
import { Navigate } from "react-router-dom";

import { useAuth } from "../context/AuthContext";

interface ProtectedRouteProps {
  children: ReactNode;
}

export default function ProtectedRoute({
  children,
}: ProtectedRouteProps) {
  const { user, loading } = useAuth();

  if (loading) {
    return (
      <div className="relative flex min-h-screen items-center justify-center overflow-hidden px-4">
        <div className="absolute left-1/3 top-1/3 h-72 w-72 rounded-full bg-accent-strong/20 blur-[120px]" />

        <div className="absolute bottom-1/4 right-1/3 h-72 w-72 rounded-full bg-violet/15 blur-[120px]" />

        <div className="glass-panel relative flex flex-col items-center rounded-3xl px-10 py-9">
          <div className="relative flex h-16 w-16 items-center justify-center">
            <div className="absolute inset-0 animate-spin rounded-full border-2 border-white/10 border-t-cyan" />

            <div className="flex h-11 w-11 items-center justify-center rounded-2xl bg-gradient-to-br from-accent-strong to-accent shadow-[0_0_30px_rgba(29,214,245,0.2)]">
              <span className="font-bold text-white">
                A
              </span>
            </div>
          </div>

          <p className="mt-5 text-sm font-medium text-white">
            Loading AeroMind
          </p>

          <p className="mt-1 text-xs text-muted">
            Connecting to operations console…
          </p>
        </div>
      </div>
    );
  }

  if (!user) {
    return <Navigate to="/login" replace />;
  }

  return <>{children}</>;
}