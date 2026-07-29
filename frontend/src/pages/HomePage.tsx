import { useCallback, useEffect, useState } from "react";
import { Link } from "react-router-dom";
import {
  AlertTriangle,
  Bot,
  Clock,
  DoorOpen,
  Eye,
  LayoutDashboard,
  Map,
  Plane,
  RefreshCw,
  Wind,
} from "lucide-react";

import { api, ApiError } from "../api/client";
import FlightDetailsDrawer from "../components/flight-details/FlightDetailsDrawer";
import DateTimeDisplay from "../components/common/DateTimeDisplay";
import StatusBadge from "../components/common/StatusBadge";
import { useAuth } from "../context/AuthContext";
import type {
  Flight,
  Gate,
  Incident,
  ListResponse,
  WeatherReport,
} from "../types/api";

const IN_PROGRESS_STATUSES = ["scheduled", "boarding", "departed"];
const URGENT_SEVERITIES = ["high", "critical"];

function capitalise(value: string): string {
  return value.charAt(0).toUpperCase() + value.slice(1);
}

export default function HomePage() {
  const { user, logout } = useAuth();

  const [flights, setFlights] = useState<Flight[]>([]);
  const [gates, setGates] = useState<Gate[]>([]);
  const [incidents, setIncidents] = useState<Incident[]>([]);
  const [weather, setWeather] = useState<WeatherReport | null>(null);
  const [selectedFlight, setSelectedFlight] = useState<Flight | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const displayName = user?.username?.trim() || "Operator";

  const loadOperations = useCallback(async () => {
    setLoading(true);
    setError(null);

    try {
      const [flightList, gateList, incidentList, weatherList] = await Promise.all([
        api.get<ListResponse<Flight>>("/flights"),
        api.get<ListResponse<Gate>>("/gates"),
        api.get<ListResponse<Incident>>("/incidents"),
        api.get<ListResponse<WeatherReport>>("/weather"),
      ]);

      setFlights(flightList.data);
      setGates(gateList.data);
      setIncidents(incidentList.data);
      setWeather(weatherList.data[0] ?? null);
    } catch (caught) {
      setError(
        caught instanceof ApiError
          ? caught.message
          : "Could not load operations data.",
      );
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    const request = window.setTimeout(() => void loadOperations(), 0);
    return () => window.clearTimeout(request);
  }, [loadOperations]);

  const activeFlights = flights.filter((flight) =>
    IN_PROGRESS_STATUSES.includes(flight.status),
  );
  const delayedFlights = flights.filter((flight) => flight.status === "delayed");
  const availableGates = gates.filter((gate) => gate.status === "available");
  const occupiedGates = gates.filter((gate) => gate.status === "occupied");
  const urgentIncidents = incidents.filter((incident) =>
    URGENT_SEVERITIES.includes(incident.severity),
  );

  const stats = [
    {
      title: "Active flights",
      value: activeFlights.length,
      detail: `${flights.length} total in the system`,
      icon: Plane,
      iconClass: "bg-cyan/10 text-cyan",
      detailClass: "text-muted",
    },
    {
      title: "Available gates",
      value: availableGates.length,
      detail: `${occupiedGates.length} currently occupied`,
      icon: DoorOpen,
      iconClass: "bg-clear/10 text-clear",
      detailClass: "text-clear",
    },
    {
      title: "Delayed flights",
      value: delayedFlights.length,
      detail:
        delayedFlights.length > 0
          ? "Review gate assignments"
          : "Everything on schedule",
      icon: Clock,
      iconClass: "bg-warning/10 text-warning",
      detailClass: delayedFlights.length > 0 ? "text-warning" : "text-clear",
    },
    {
      title: "Logged incidents",
      value: incidents.length,
      detail:
        urgentIncidents.length > 0
          ? `${urgentIncidents.length} high or critical`
          : "No critical incidents",
      icon: AlertTriangle,
      iconClass: "bg-alert/10 text-alert",
      detailClass: urgentIncidents.length > 0 ? "text-alert" : "text-muted",
    },
  ];

  return (
    <div className="min-h-screen px-3 py-3 sm:px-5 sm:py-5">
      <div className="glass-panel mx-auto flex min-h-[calc(100vh-1.5rem)] max-w-[1540px] overflow-hidden rounded-[28px] sm:min-h-[calc(100vh-2.5rem)]">
        <aside className="hidden w-[235px] shrink-0 flex-col border-r border-white/10 bg-black/15 lg:flex">
          <div className="flex items-center gap-3 border-b border-white/10 px-5 py-5">
            <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-gradient-to-br from-accent-strong to-accent shadow-[0_8px_25px_rgba(47,128,255,0.28)]">
              <span className="font-bold text-white">A</span>
            </div>

            <div>
              <p className="font-semibold tracking-tight text-white">AeroMind</p>
              <p className="text-xs text-muted">Operations Intelligence</p>
            </div>
          </div>

          <nav className="flex-1 space-y-1 px-3 py-5">
            <span className="flex w-full items-center gap-3 rounded-xl border border-accent/15 bg-gradient-to-r from-accent-strong/25 to-violet/10 px-3.5 py-2.5 text-sm text-white shadow-[0_8px_22px_rgba(49,87,246,0.12)]">
              <LayoutDashboard className="h-4 w-4 text-cyan" />
              Overview
            </span>

            <Link
              to="/operations-map"
              className="flex w-full items-center gap-3 rounded-xl border border-transparent px-3.5 py-2.5 text-sm text-muted transition-all hover:border-white/5 hover:bg-white/[0.045] hover:text-white"
            >
              <Map className="h-4 w-4" />
              Operations Map
            </Link>

            <Link
              to="/chat"
              className="flex w-full items-center gap-3 rounded-xl border border-transparent px-3.5 py-2.5 text-sm text-muted transition-all hover:border-white/5 hover:bg-white/[0.045] hover:text-white"
            >
              <Bot className="h-4 w-4" />
              AI Assistant
            </Link>
          </nav>

          <div className="border-t border-white/10 p-4">
            <div className="flex items-center gap-3 rounded-2xl bg-white/[0.035] p-3">
              <div className="flex h-10 w-10 shrink-0 items-center justify-center rounded-full bg-gradient-to-br from-accent-strong/70 to-violet/70 text-xs font-semibold text-white">
                {displayName.slice(0, 2).toUpperCase()}
              </div>

              <div className="min-w-0 flex-1">
                <p className="truncate text-sm font-medium text-white">
                  {displayName}
                </p>
                <p className="truncate text-xs text-muted">{user?.email}</p>
              </div>
            </div>

            <button
              type="button"
              onClick={logout}
              className="mt-3 w-full rounded-xl border border-white/10 bg-white/[0.035] py-2 text-xs font-medium text-muted transition-all hover:border-alert/30 hover:bg-alert/10 hover:text-alert"
            >
              Sign out
            </button>
          </div>
        </aside>

        <div className="min-w-0 flex-1">
          <header className="sticky top-0 z-30 border-b border-white/10 bg-paper/75 backdrop-blur-2xl">
            <div className="flex items-center justify-between gap-4 px-4 py-4 sm:px-6 lg:px-7">
              <div className="flex items-center gap-3 lg:hidden">
                <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-gradient-to-br from-accent-strong to-accent">
                  <span className="font-bold text-white">A</span>
                </div>
                <span className="font-semibold text-white">AeroMind</span>
              </div>

              <div className="hidden lg:block">
                <p className="text-xs text-muted">Airport Operations Console</p>
              </div>

              <div className="ml-auto flex items-center gap-2 sm:gap-3">
                <button
                  type="button"
                  onClick={() => void loadOperations()}
                  disabled={loading}
                  className="flex items-center gap-2 rounded-xl border border-white/10 bg-white/[0.035] px-3.5 py-2 text-xs font-medium text-muted transition-all hover:border-cyan/25 hover:bg-cyan/10 hover:text-cyan disabled:opacity-50"
                >
                  <RefreshCw
                    className={`h-3.5 w-3.5 ${loading ? "animate-spin" : ""}`}
                  />
                  Refresh
                </button>

                <Link
                  to="/chat"
                  className="flex items-center gap-3 rounded-xl border border-accent/20 bg-gradient-to-r from-accent-strong/20 to-violet/15 px-3.5 py-2"
                >
                  <span className="flex h-8 w-8 items-center justify-center rounded-full bg-gradient-to-br from-cyan to-accent text-white shadow-[0_0_20px_rgba(29,214,245,0.35)]">
                    <Bot className="h-4 w-4" />
                  </span>
                  <span className="hidden text-left sm:block">
                    <span className="block text-xs font-medium text-white">
                      AI Assistant
                    </span>
                    <span className="flex items-center gap-1 text-xs text-clear">
                      <span className="h-1.5 w-1.5 rounded-full bg-clear" />
                      Online
                    </span>
                  </span>
                </Link>

                <button
                  type="button"
                  onClick={logout}
                  className="rounded-xl border border-white/10 bg-white/[0.035] px-3 py-2 text-xs font-medium text-muted transition-all hover:border-alert/30 hover:bg-alert/10 hover:text-alert lg:hidden"
                >
                  Sign out
                </button>
              </div>
            </div>
          </header>

          <main className="px-4 py-6 sm:px-6 lg:px-7 lg:py-7">
            <section className="mb-6 flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
              <div>
                <h1 className="text-2xl font-bold tracking-[-0.03em] text-white sm:text-3xl">
                  Welcome back, {displayName}
                </h1>
                <p className="mt-1.5 text-sm text-muted">
                  Current demo operations across flights, gates, incidents and weather.
                </p>
              </div>
              <Link
                to="/operations-map"
                className="flex w-fit items-center gap-2 rounded-xl border border-cyan/20 bg-cyan/[0.07] px-4 py-2.5 text-xs font-medium text-cyan transition-all hover:-translate-y-0.5 hover:bg-cyan/10"
              >
                <Map className="h-4 w-4" />
                Open operations map
              </Link>
            </section>

            {error && (
              <div className="mb-6 flex items-center justify-between gap-4 rounded-2xl border border-alert/25 bg-alert/10 px-5 py-4">
                <p className="text-sm text-alert">{error}</p>
                <button
                  type="button"
                  onClick={() => void loadOperations()}
                  className="shrink-0 rounded-lg border border-alert/30 px-3 py-1.5 text-xs font-medium text-alert hover:bg-alert/10"
                >
                  Try again
                </button>
              </div>
            )}

            <section className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
              {stats.map((stat) => (
                <article
                  key={stat.title}
                  className="relative overflow-hidden rounded-2xl border border-white/10 bg-white/[0.04] p-5 shadow-[0_15px_50px_rgba(0,0,0,0.14)] backdrop-blur-xl transition-all duration-300 hover:-translate-y-1 hover:border-cyan/20 hover:bg-white/[0.055]"
                >
                  <div className="absolute right-[-30px] top-[-35px] h-28 w-28 rounded-full bg-accent/5 blur-3xl" />

                  <div className="relative flex gap-3">
                    <div
                      className={`flex h-10 w-10 shrink-0 items-center justify-center rounded-xl ${stat.iconClass}`}
                    >
                      <stat.icon className="h-5 w-5" />
                    </div>

                    <div>
                      <p className="text-xs text-muted">{stat.title}</p>
                      <p className="mt-1 font-mono text-3xl font-semibold tracking-tight text-white">
                        {loading ? "—" : stat.value}
                      </p>
                      <p className={`mt-1 text-xs ${stat.detailClass}`}>
                        {loading ? "Loading…" : stat.detail}
                      </p>
                    </div>
                  </div>
                </article>
              ))}
            </section>

            <section className="mt-5 grid gap-5 xl:grid-cols-[1.6fr_1fr]">
              <article className="overflow-hidden rounded-2xl border border-white/10 bg-white/[0.04] backdrop-blur-xl">
                <div className="flex items-center justify-between border-b border-white/10 px-5 py-4">
                  <div>
                    <p className="text-sm font-semibold text-white">
                      Flight status
                    </p>
                    <p className="mt-0.5 text-xs text-muted">
                      {loading
                        ? "Loading flights…"
                        : `Showing ${Math.min(flights.length, 8)} of ${flights.length}`}
                    </p>
                  </div>
                </div>

                {!loading && flights.length === 0 ? (
                  <p className="px-5 py-10 text-center text-sm text-muted">
                    No flights in the database yet. Run the seed script to load
                    sample operations data.
                  </p>
                ) : (
                  <div className="overflow-x-auto">
                    <table className="w-full min-w-[680px] text-left">
                      <thead>
                        <tr className="border-b border-white/10 text-xs uppercase tracking-[0.08em] text-muted">
                          <th className="px-5 py-3 font-medium">Flight</th>
                          <th className="px-3 py-3 font-medium">From</th>
                          <th className="px-3 py-3 font-medium">To</th>
                          <th className="px-3 py-3 font-medium">Status</th>
                          <th className="px-3 py-3 font-medium">Departure</th>
                          <th className="px-5 py-3 font-medium">Gate</th>
                        </tr>
                      </thead>

                      <tbody>
                        {flights.slice(0, 8).map((flight) => (
                          <tr
                            key={flight.id}
                            role="button"
                            tabIndex={0}
                            aria-label={`View details for flight ${flight.flight_number}`}
                            onClick={() => setSelectedFlight(flight)}
                            onKeyDown={(event) => {
                              if (event.key === "Enter" || event.key === " ") {
                                event.preventDefault();
                                setSelectedFlight(flight);
                              }
                            }}
                            className="cursor-pointer border-b border-white/[0.055] text-sm transition-colors last:border-0 hover:bg-white/[0.04] focus-visible:outline-offset-[-3px]"
                          >
                            <td className="px-5 py-4 font-mono font-semibold text-white">
                              {flight.flight_number}
                            </td>
                            <td className="px-3 py-4 text-muted-light">
                              {flight.origin}
                            </td>
                            <td className="px-3 py-4 text-muted-light">
                              {flight.destination}
                            </td>
                            <td className="px-3 py-4">
                              <StatusBadge status={flight.status} />
                            </td>
                            <td className="px-3 py-4 font-mono text-muted-light">
                              <DateTimeDisplay value={flight.departure_time} />
                            </td>
                            <td className="px-5 py-4 font-mono text-muted-light">
                              {flight.gate_number ?? "Unassigned"}
                            </td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </article>

              <div className="space-y-5">
                <article className="rounded-2xl border border-white/10 bg-white/[0.04] p-5 backdrop-blur-xl">
                  <p className="text-sm font-semibold text-white">
                    Current conditions
                  </p>
                  <p className="mt-0.5 text-xs text-muted">
                    Latest airport weather report
                  </p>

                  {weather === null ? (
                    <p className="mt-6 text-sm text-muted">
                      {loading ? "Loading…" : "No weather reports recorded yet."}
                    </p>
                  ) : (
                    <>
                      <div className="mt-6 flex items-end gap-2">
                        <p className="font-mono text-4xl font-semibold text-white">
                          {weather.temperature}°
                        </p>
                        <p className="mb-1.5 text-xs text-muted">C</p>
                      </div>
                      <p className="mt-1 text-sm text-muted-light">
                        {capitalise(weather.condition)}
                      </p>

                      <div className="mt-6 grid grid-cols-2 divide-x divide-white/10 border-y border-white/10 py-4">
                        <div className="pr-3">
                          <p className="flex items-center gap-1.5 text-xs text-muted">
                            <Wind className="h-3.5 w-3.5" />
                            Wind
                          </p>
                          <p className="mt-1 font-mono text-sm text-white">
                            {weather.wind_speed} kt
                          </p>
                        </div>

                        <div className="pl-4">
                          <p className="flex items-center gap-1.5 text-xs text-muted">
                            <Eye className="h-3.5 w-3.5" />
                            Visibility
                          </p>
                          <p className="mt-1 font-mono text-sm text-white">
                            {weather.visibility} km
                          </p>
                        </div>
                      </div>
                    </>
                  )}
                </article>

                <article className="rounded-2xl border border-white/10 bg-white/[0.04] p-5 backdrop-blur-xl">
                  <p className="text-sm font-semibold text-white">
                    Recent incidents
                  </p>

                  {incidents.length === 0 ? (
                    <p className="mt-4 text-sm text-muted">
                      {loading ? "Loading…" : "No incidents logged."}
                    </p>
                  ) : (
                    <ul className="mt-4 space-y-3">
                      {incidents.slice(0, 3).map((incident) => (
                        <li
                          key={incident.id}
                          className="rounded-xl border border-white/10 bg-black/15 p-3"
                        >
                          <div className="flex items-start justify-between gap-3">
                            <p className="text-sm font-medium text-white">
                              {incident.title}
                            </p>
                            <span
                              className={`shrink-0 rounded-md px-2 py-0.5 text-xs font-medium ${
                                URGENT_SEVERITIES.includes(incident.severity)
                                  ? "bg-alert/10 text-alert"
                                  : "bg-warning/10 text-warning"
                              }`}
                            >
                              {capitalise(incident.severity)}
                            </span>
                          </div>
                          <p className="mt-1 text-xs text-muted">
                            {incident.location}
                          </p>
                        </li>
                      ))}
                    </ul>
                  )}
                </article>
              </div>
            </section>
          </main>
        </div>
      </div>

      {selectedFlight && (
        <FlightDetailsDrawer
          flightId={selectedFlight.id}
          initialFlight={selectedFlight}
          onClose={() => setSelectedFlight(null)}
        />
      )}
    </div>
  );
}
