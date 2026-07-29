import { useCallback, useEffect, useMemo, useState } from "react";
import {
  AlertTriangle,
  ArrowLeft,
  Bot,
  Map as MapIcon,
  RefreshCw,
} from "lucide-react";
import { Link, useSearchParams } from "react-router-dom";

import FlightDetailsDrawer from "../components/flight-details/FlightDetailsDrawer";
import GateDetailsDrawer from "../components/operations-map/GateDetailsDrawer";
import TerminalSection from "../components/operations-map/TerminalSection";
import AirportLegend from "../components/operations-map/AirportLegend";
import AirportSummaryCards from "../components/operations-map/AirportSummaryCards";
import RunwaySection from "../components/operations-map/RunwaySection";
import { api, ApiError } from "../api/client";
import type {
  Flight,
  Gate,
  ListResponse,
  Runway,
  Terminal,
} from "../types/api";

type ResourceName = "flights" | "gates" | "runways" | "terminals";
type ResourceErrors = Partial<Record<ResourceName, string>>;

const INACTIVE_FLIGHT_STATUSES = new Set(["arrived", "cancelled", "departed"]);

function errorMessage(reason: unknown, resource: string): string {
  return reason instanceof ApiError
    ? reason.message
    : `Could not load ${resource}.`;
}

function MapSkeleton() {
  return (
    <div aria-label="Loading airport map" className="space-y-5">
      {[0, 1].map((section) => (
        <div
          key={section}
          className="rounded-3xl border border-white/10 bg-white/[0.025] p-5"
        >
          <div className="h-10 w-48 animate-pulse-soft rounded-xl bg-white/[0.07]" />
          <div className="mt-5 grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
            {[0, 1, 2, 3].map((card) => (
              <div
                key={card}
                className="h-32 animate-pulse-soft rounded-2xl bg-white/[0.055]"
              />
            ))}
          </div>
        </div>
      ))}
    </div>
  );
}

function ResourceWarning({ resource, message }: { resource: string; message: string }) {
  return (
    <div className="flex items-start gap-3 rounded-2xl border border-warning/25 bg-warning/[0.07] px-4 py-3">
      <AlertTriangle className="mt-0.5 h-4 w-4 shrink-0 text-warning" />
      <p className="text-xs leading-5 text-warning">
        <span className="font-semibold">{resource} unavailable.</span> {message}
      </p>
    </div>
  );
}

export default function OperationsMapPage() {
  const [searchParams] = useSearchParams();
  const [flights, setFlights] = useState<Flight[]>([]);
  const [gates, setGates] = useState<Gate[]>([]);
  const [runways, setRunways] = useState<Runway[]>([]);
  const [terminals, setTerminals] = useState<Terminal[]>([]);
  const [selectedGate, setSelectedGate] = useState<Gate | null>(null);
  const [selectedFlightDetails, setSelectedFlightDetails] = useState<Flight | null>(
    null,
  );
  const [errors, setErrors] = useState<ResourceErrors>({});
  const [loading, setLoading] = useState(true);

  const loadMap = useCallback(async () => {
    setLoading(true);
    setErrors({});

    const results = await Promise.allSettled([
      api.get<ListResponse<Flight>>("/flights"),
      api.get<ListResponse<Gate>>("/gates"),
      api.get<ListResponse<Runway>>("/runways"),
      api.get<ListResponse<Terminal>>("/terminals"),
    ]);
    const nextErrors: ResourceErrors = {};
    const [flightResult, gateResult, runwayResult, terminalResult] = results;

    if (flightResult.status === "fulfilled") {
      setFlights(flightResult.value.data);
    } else {
      setFlights([]);
      nextErrors.flights = errorMessage(flightResult.reason, "flights");
    }

    if (gateResult.status === "fulfilled") {
      setGates(gateResult.value.data);
    } else {
      setGates([]);
      nextErrors.gates = errorMessage(gateResult.reason, "gates");
    }

    if (runwayResult.status === "fulfilled") {
      setRunways(runwayResult.value.data);
    } else {
      setRunways([]);
      nextErrors.runways = errorMessage(runwayResult.reason, "runways");
    }

    if (terminalResult.status === "fulfilled") {
      setTerminals(terminalResult.value.data);
    } else {
      setTerminals([]);
      nextErrors.terminals = errorMessage(terminalResult.reason, "terminals");
    }

    setErrors(nextErrors);
    setLoading(false);
  }, []);

  useEffect(() => {
    const request = window.setTimeout(() => void loadMap(), 0);

    return () => window.clearTimeout(request);
  }, [loadMap]);

  useEffect(() => {
    const gateNumber = searchParams.get("gate");

    if (!gateNumber || gates.length === 0 || selectedFlightDetails) {
      return;
    }

    const request = window.setTimeout(() => {
      const matchingGate = gates.find((gate) => gate.gate_number === gateNumber);
      if (matchingGate) {
        setSelectedGate(matchingGate);
      }
    }, 0);

    return () => window.clearTimeout(request);
  }, [gates, searchParams, selectedFlightDetails]);

  const flightsByGate = useMemo(() => {
    const sortedFlights = flights
      .filter((flight) => !INACTIVE_FLIGHT_STATUSES.has(flight.status))
      .sort((first, second) =>
        new Date(first.departure_time ?? 0).getTime() -
        new Date(second.departure_time ?? 0).getTime(),
      );
    const assignments = new Map<string, Flight>();

    sortedFlights.forEach((flight) => {
      if (flight.gate_number && !assignments.has(flight.gate_number)) {
        assignments.set(flight.gate_number, flight);
      }
    });

    return assignments;
  }, [flights]);

  const terminalNames = useMemo(() => {
    const names = terminals.map((terminal) => terminal.name);

    gates.forEach((gate) => {
      if (gate.terminal && !names.includes(gate.terminal)) {
        names.push(gate.terminal);
      }
    });

    return names.sort((first, second) => first.localeCompare(second));
  }, [gates, terminals]);

  const selectedFlight = selectedGate?.status === "occupied"
    ? flightsByGate.get(selectedGate.gate_number)
    : undefined;

  return (
    <div className="min-h-screen px-3 py-3 sm:px-5 sm:py-5">
      <div className="glass-panel mx-auto min-h-[calc(100vh-1.5rem)] max-w-[1540px] overflow-hidden rounded-[28px] sm:min-h-[calc(100vh-2.5rem)]">
        <header className="sticky top-0 z-30 border-b border-white/10 bg-paper/80 backdrop-blur-2xl">
          <div className="flex flex-wrap items-center justify-between gap-4 px-4 py-4 sm:px-6 lg:px-8">
            <div className="flex items-center gap-3">
              <span className="flex h-10 w-10 items-center justify-center rounded-xl bg-gradient-to-br from-cyan to-accent text-white shadow-[0_0_24px_rgba(29,214,245,0.25)]">
                <MapIcon className="h-5 w-5" />
              </span>
              <div>
                <p className="font-semibold text-white">Airport Operations Map</p>
                <p className="text-xs text-muted">Operational airside layout</p>
              </div>
            </div>

            <div className="flex items-center gap-2">
              <Link
                to="/"
                className="flex items-center gap-2 rounded-xl border border-white/10 bg-white/[0.035] px-3.5 py-2 text-xs font-medium text-muted transition-all hover:border-cyan/25 hover:text-cyan"
              >
                <ArrowLeft className="h-3.5 w-3.5" />
                <span className="hidden sm:inline">Overview</span>
              </Link>
              <Link
                to="/chat"
                className="flex items-center gap-2 rounded-xl border border-accent/20 bg-accent/10 px-3.5 py-2 text-xs font-medium text-cyan transition-all hover:bg-accent/20"
              >
                <Bot className="h-3.5 w-3.5" />
                <span className="hidden sm:inline">AI Assistant</span>
              </Link>
              <button
                type="button"
                onClick={() => void loadMap()}
                disabled={loading}
                className="flex items-center gap-2 rounded-xl border border-white/10 bg-white/[0.035] px-3.5 py-2 text-xs font-medium text-muted transition-all hover:border-cyan/25 hover:text-cyan disabled:opacity-50"
              >
                <RefreshCw
                  className={`h-3.5 w-3.5 ${loading ? "animate-spin" : ""}`}
                />
                <span className="hidden sm:inline">Refresh</span>
              </button>
            </div>
          </div>
        </header>

        <main className="px-4 py-6 sm:px-6 lg:px-8 lg:py-8">
          <section>
            <p className="label">Operations center</p>
            <h1 className="mt-2 text-2xl font-bold tracking-[-0.03em] text-white sm:text-3xl">
              Airport Operations Map
            </h1>
            <p className="mt-2 max-w-2xl text-sm leading-6 text-muted">
              A visual overview of terminal gates, assigned flights, and
              runway availability.
            </p>
          </section>

          {Object.keys(errors).length > 0 && (
            <section
              aria-label="Map data warnings"
              className="mt-6 grid gap-3 md:grid-cols-2"
            >
              {Object.entries(errors).map(([resource, message]) => (
                <ResourceWarning
                  key={resource}
                  resource={resource.charAt(0).toUpperCase() + resource.slice(1)}
                  message={message}
                />
              ))}
            </section>
          )}

          {!loading && (
            <>
              <AirportSummaryCards
                availableGates={
                  gates.filter((gate) => gate.status === "available").length
                }
                occupiedGates={
                  gates.filter((gate) => gate.status === "occupied").length
                }
                closedRunways={
                  runways.filter((runway) => runway.status === "closed").length
                }
                assignedFlights={flightsByGate.size}
              />
              <div className="mt-3 flex justify-start sm:justify-end">
                <AirportLegend />
              </div>
            </>
          )}

          <section className="soft-grid mt-6 rounded-3xl border border-white/10 bg-black/10 p-3 sm:p-5 lg:p-6">
            {loading ? (
              <MapSkeleton />
            ) : terminalNames.length === 0 ? (
              <div className="flex min-h-72 items-center justify-center rounded-2xl border border-dashed border-white/10 px-5 text-center">
                <div>
                  <MapIcon className="mx-auto h-8 w-8 text-muted" />
                  <p className="mt-3 text-sm font-medium text-white">
                    No terminals to display
                  </p>
                  <p className="mt-1 text-xs text-muted">
                    Add terminals and gates to populate the operations map.
                  </p>
                </div>
              </div>
            ) : (
              <div className="space-y-5">
                {terminalNames.map((terminalName) => (
                  <TerminalSection
                    key={terminalName}
                    name={terminalName}
                    gates={gates.filter((gate) => gate.terminal === terminalName)}
                    flightsByGate={flightsByGate}
                    onSelectGate={setSelectedGate}
                  />
                ))}
              </div>
            )}

            {!loading && <RunwaySection runways={runways} />}
          </section>
        </main>
      </div>

      {selectedGate && (
        <GateDetailsDrawer
          gate={selectedGate}
          flight={selectedFlight}
          onClose={() => setSelectedGate(null)}
          onSelectFlight={(flight) => {
            setSelectedGate(null);
            setSelectedFlightDetails(flight);
          }}
        />
      )}

      {selectedFlightDetails && (
        <FlightDetailsDrawer
          flightId={selectedFlightDetails.id}
          initialFlight={selectedFlightDetails}
          onClose={() => setSelectedFlightDetails(null)}
        />
      )}
    </div>
  );
}
