import { useState } from "react";
import type { FormEvent } from "react";
import { Link, Navigate } from "react-router-dom";

import { ApiError } from "../api/client";
import { useAuth } from "../context/AuthContext";

interface AuthPageProps {
  mode: "login" | "register";
}

const inputClass =
  "w-full rounded-xl border border-white/10 bg-white/[0.045] px-4 py-3 text-sm text-ink outline-none transition-all duration-300 placeholder:text-muted/55 hover:border-white/20 focus:border-cyan/70 focus:bg-white/[0.065] focus:shadow-[0_0_0_4px_rgba(29,214,245,0.08)]";

export default function AuthPage({ mode }: AuthPageProps) {
  const { user, login, register } = useAuth();

  const [identifier, setIdentifier] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  const isRegister = mode === "register";

  if (user) {
    return <Navigate to="/" replace />;
  }

  async function handleSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setError(null);

    if (isRegister && password.length < 8) {
      setError("Password must be at least 8 characters.");
      return;
    }

    setSubmitting(true);

    try {
      if (isRegister) {
        await register(identifier, email, password);
      } else {
        await login(identifier, password);
      }
    } catch (caught) {
      setError(
        caught instanceof ApiError
          ? caught.message
          : "Something went wrong. Try again.",
      );
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <main className="relative min-h-screen overflow-hidden px-4 py-5 sm:px-6 lg:p-6">
      <div className="pointer-events-none absolute -left-20 top-10 h-80 w-80 rounded-full bg-accent-strong/20 blur-[120px]" />

      <div className="pointer-events-none absolute bottom-[-100px] right-[-60px] h-96 w-96 rounded-full bg-violet/20 blur-[130px]" />

      <div className="pointer-events-none absolute left-[48%] top-[20%] h-72 w-72 rounded-full bg-cyan/10 blur-[120px]" />

      <div className="glass-panel relative mx-auto grid min-h-[calc(100vh-2.5rem)] max-w-7xl overflow-hidden rounded-[30px] lg:min-h-[calc(100vh-3rem)] lg:grid-cols-[1.08fr_0.92fr]">
        <section className="soft-grid relative hidden overflow-hidden border-r border-white/10 p-10 lg:flex lg:flex-col lg:justify-between xl:p-14">
          <div className="absolute inset-0 bg-gradient-to-br from-accent-strong/16 via-transparent to-violet/12" />

          <div className="absolute -right-20 -top-20 h-80 w-80 rounded-full bg-accent/15 blur-[110px]" />

          <div className="relative z-10 flex items-center gap-3">
            <div className="flex h-11 w-11 items-center justify-center rounded-2xl border border-cyan/35 bg-gradient-to-br from-accent-strong/40 to-cyan/15 shadow-[0_0_35px_rgba(29,214,245,0.16)]">
              <span className="text-xl font-bold text-white">A</span>
            </div>

            <div>
              <p className="text-lg font-semibold tracking-tight text-white">
                AeroMind
              </p>

              <p className="text-xs text-muted">
                Airport Operations Intelligence
              </p>
            </div>
          </div>

          <div className="relative z-10 max-w-2xl">
            <div className="mb-5 inline-flex items-center gap-2 rounded-full border border-clear/25 bg-clear/10 px-3.5 py-1.5 text-xs font-medium text-clear">
              <span className="h-2 w-2 animate-pulse rounded-full bg-clear shadow-[0_0_12px_rgba(36,212,138,0.7)]" />
              Operations systems online
            </div>

            <h1 className="max-w-xl text-5xl font-bold leading-[1.05] tracking-[-0.045em] text-white xl:text-6xl">
              Airport operations,
              <span className="text-gradient mt-1 block">
                intelligently connected.
              </span>
            </h1>

            <p className="mt-6 max-w-lg text-base leading-7 text-muted-light/80">
              Monitor flights, gates, runways, weather and incidents through
              one intelligent airport operations platform.
            </p>

            <div className="mt-10 grid grid-cols-3 gap-3">
              <article className="rounded-2xl border border-white/10 bg-black/15 p-4 backdrop-blur-xl">
                <div className="mb-5 flex h-9 w-9 items-center justify-center rounded-xl bg-accent/12 text-lg text-cyan">
                  ✈
                </div>

                <p className="font-mono text-2xl font-semibold text-white">
                  248
                </p>

                <p className="mt-1 text-xs text-muted">
                  Active flights
                </p>

                <p className="mt-3 text-[11px] text-clear">
                  +12 in the last hour
                </p>
              </article>

              <article className="rounded-2xl border border-white/10 bg-black/15 p-4 backdrop-blur-xl">
                <div className="mb-5 flex h-9 w-9 items-center justify-center rounded-xl bg-clear/10 text-lg text-clear">
                  ◫
                </div>

                <p className="font-mono text-2xl font-semibold text-white">
                  14
                </p>

                <p className="mt-1 text-xs text-muted">
                  Available gates
                </p>

                <p className="mt-3 text-[11px] text-clear">
                  4 currently occupied
                </p>
              </article>

              <article className="rounded-2xl border border-white/10 bg-black/15 p-4 backdrop-blur-xl">
                <div className="mb-5 flex h-9 w-9 items-center justify-center rounded-xl bg-alert/10 text-lg text-alert">
                  ◈
                </div>

                <p className="font-mono text-2xl font-semibold text-white">
                  03
                </p>

                <p className="mt-1 text-xs text-muted">
                  Active alerts
                </p>

                <p className="mt-3 text-[11px] text-alert">
                  No critical incidents
                </p>
              </article>
            </div>
          </div>

          <div className="relative z-10 flex items-center gap-2 text-xs text-muted">
            <span className="text-cyan">◉</span>
            Secure, encrypted and protected
          </div>
        </section>

        <section className="relative flex items-center justify-center px-5 py-10 sm:px-10 lg:px-12 xl:px-16">
          <div className="pointer-events-none absolute right-[-80px] top-[-80px] h-72 w-72 rounded-full bg-accent-strong/15 blur-[100px]" />

          <div className="relative w-full max-w-md">
            <div className="mb-9 lg:hidden">
              <div className="mb-5 flex h-12 w-12 items-center justify-center rounded-2xl border border-cyan/30 bg-gradient-to-br from-accent-strong/40 to-cyan/15 shadow-[0_0_30px_rgba(29,214,245,0.15)]">
                <span className="text-xl font-bold text-white">A</span>
              </div>

              <h1 className="text-2xl font-bold tracking-tight text-white">
                AeroMind
              </h1>

              <p className="mt-1 text-sm text-muted">
                Airport Operations Intelligence
              </p>
            </div>

            <div className="mb-8">
              <p className="mb-2 text-xs font-semibold uppercase tracking-[0.18em] text-cyan">
                Secure access
              </p>

              <h2 className="text-3xl font-bold tracking-[-0.03em] text-white sm:text-4xl">
                {isRegister ? "Create your account" : "Welcome back"}
              </h2>

              <p className="mt-3 text-sm leading-6 text-muted">
                {isRegister
                  ? "Create an account to access the AeroMind operations console."
                  : "Sign in to access the airport operations console."}
              </p>
            </div>

            <form
              onSubmit={handleSubmit}
              className="rounded-[26px] border border-white/10 bg-black/15 p-6 shadow-[0_25px_80px_rgba(0,0,0,0.32)] backdrop-blur-2xl sm:p-8"
            >
              <label htmlFor="auth-identifier" className="label mb-2 block">
                {isRegister ? "Username" : "Username or email"}
              </label>

              <input
                id="auth-identifier"
                value={identifier}
                onChange={(event) =>
                  setIdentifier(event.target.value)
                }
                required
                autoComplete="username"
                placeholder={
                  isRegister
                    ? "Choose a username"
                    : "Enter username or email"
                }
                className={`${inputClass} mb-5`}
              />

              {isRegister && (
                <>
                  <label htmlFor="auth-email" className="label mb-2 block">
                    Email address
                  </label>

                  <input
                    id="auth-email"
                    type="email"
                    value={email}
                    onChange={(event) =>
                      setEmail(event.target.value)
                    }
                    required
                    autoComplete="email"
                    placeholder="name@airport.com"
                    className={`${inputClass} mb-5`}
                  />
                </>
              )}

              <label htmlFor="auth-password" className="label mb-2 block">
                Password
              </label>

              <input
                id="auth-password"
                type="password"
                value={password}
                onChange={(event) =>
                  setPassword(event.target.value)
                }
                required
                autoComplete={
                  isRegister
                    ? "new-password"
                    : "current-password"
                }
                placeholder="Enter your password"
                className={inputClass}
              />

              {isRegister && (
                <p className="mt-2 text-xs text-muted">
                  Use at least 8 characters.
                </p>
              )}

              {error && (
                <p className="mt-4 rounded-xl border border-alert/25 bg-alert/10 px-3.5 py-3 text-xs leading-5 text-alert">
                  {error}
                </p>
              )}

              <button
                type="submit"
                disabled={submitting}
                className="mt-6 w-full rounded-xl bg-gradient-to-r from-accent-strong via-accent to-cyan px-4 py-3 text-sm font-semibold text-white shadow-[0_14px_35px_rgba(47,128,255,0.28)] transition-all duration-300 hover:-translate-y-0.5 hover:shadow-[0_18px_45px_rgba(29,214,245,0.28)] disabled:translate-y-0 disabled:opacity-50"
              >
                {submitting
                  ? "Working…"
                  : isRegister
                    ? "Create account"
                    : "Sign in to console"}
              </button>
            </form>

            <p className="mt-6 text-center text-sm text-muted">
              {isRegister
                ? "Already have an account? "
                : "New to AeroMind? "}

              <Link
                to={isRegister ? "/login" : "/register"}
                className="font-semibold text-cyan transition-colors hover:text-white"
              >
                {isRegister
                  ? "Sign in"
                  : "Create an account"}
              </Link>
            </p>

            <div className="mt-8 flex flex-wrap items-center justify-center gap-x-3 gap-y-2 text-xs text-muted/80">
              <span className="inline-flex items-center gap-2">
                <span className="h-1.5 w-1.5 rounded-full bg-clear shadow-[0_0_8px_rgba(36,212,138,0.7)]" />
                Encrypted connection
              </span>

              <span className="text-line">•</span>

              <span>Protected access</span>
            </div>
          </div>
        </section>
      </div>
    </main>
  );
}
