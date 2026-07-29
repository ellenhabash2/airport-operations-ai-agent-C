import { AlertTriangle, LoaderCircle, X } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { useNavigate } from "react-router-dom";

import { api, ApiError } from "../../api/client";
import type {
  Flight,
  Gate,
  Incident,
  ItemResponse,
  ListResponse,
  Runway,
  WeatherReport,
} from "../../types/api";
import FlightHeader from "./FlightHeader";
import FlightInfoSections from "./FlightInfoSections";
import IncidentSection from "./IncidentSection";
import QuickActionsSection from "./QuickActionsSection";
import WeatherSection from "./WeatherSection";

interface FlightDetailsDrawerProps {
  flightId: number;
  initialFlight?: Flight;
  onClose: () => void;
}

interface OptionalDataErrors {
  gates?: boolean;
  runways?: boolean;
  weather?: boolean;
  incidents?: boolean;
}

function messageFromError(error: unknown): string {
  return error instanceof ApiError
    ? error.message
    : "Flight details could not be loaded.";
}

function relatedToFlight(incident: Incident, flightNumber: string): boolean {
  const searchable = [incident.title, incident.description, incident.location]
    .join(" ")
    .toLowerCase();

  return searchable.includes(flightNumber.toLowerCase());
}

export default function FlightDetailsDrawer({
  flightId,
  initialFlight,
  onClose,
}: FlightDetailsDrawerProps) {
  const navigate = useNavigate();
  const [flight, setFlight] = useState<Flight | null>(initialFlight ?? null);
  const [gates, setGates] = useState<Gate[]>([]);
  const [runways, setRunways] = useState<Runway[]>([]);
  const [weather, setWeather] = useState<WeatherReport | null>(null);
  const [incidents, setIncidents] = useState<Incident[]>([]);
  const [optionalErrors, setOptionalErrors] = useState<OptionalDataErrors>({});
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(initialFlight == null);
  const drawerRef = useRef<HTMLElement>(null);
  const closeButtonRef = useRef<HTMLButtonElement>(null);
  const incidentSectionRef = useRef<HTMLElement>(null);

  const loadDetails = useCallback(async () => {
    if (!initialFlight) {
      setLoading(true);
    }
    setError(null);

    const results = await Promise.allSettled([
      api.get<ItemResponse<Flight>>(`/flights/${flightId}`),
      api.get<ListResponse<Gate>>("/gates"),
      api.get<ListResponse<Runway>>("/runways"),
      api.get<ListResponse<WeatherReport>>("/weather"),
      api.get<ListResponse<Incident>>("/incidents"),
    ]);
    const [flightResult, gateResult, runwayResult, weatherResult, incidentResult] =
      results;

    if (flightResult.status === "fulfilled") {
      setFlight(flightResult.value.data);
    } else if (!initialFlight) {
      setFlight(null);
      setError(messageFromError(flightResult.reason));
    }

    const nextOptionalErrors: OptionalDataErrors = {};

    if (gateResult.status === "fulfilled") {
      setGates(gateResult.value.data);
    } else {
      setGates([]);
      nextOptionalErrors.gates = true;
    }

    if (runwayResult.status === "fulfilled") {
      setRunways(runwayResult.value.data);
    } else {
      setRunways([]);
      nextOptionalErrors.runways = true;
    }

    if (weatherResult.status === "fulfilled") {
      setWeather(weatherResult.value.data[0] ?? null);
    } else {
      setWeather(null);
      nextOptionalErrors.weather = true;
    }

    if (incidentResult.status === "fulfilled") {
      setIncidents(incidentResult.value.data);
    } else {
      setIncidents([]);
      nextOptionalErrors.incidents = true;
    }

    setOptionalErrors(nextOptionalErrors);
    setLoading(false);
  }, [flightId, initialFlight]);

  useEffect(() => {
    const request = window.setTimeout(() => void loadDetails(), 0);
    return () => window.clearTimeout(request);
  }, [loadDetails]);

  useEffect(() => {
    const previousFocus = document.activeElement as HTMLElement | null;
    const previousOverflow = document.body.style.overflow;
    document.body.style.overflow = "hidden";

    const focusRequest = window.setTimeout(() => closeButtonRef.current?.focus(), 0);

    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") {
        event.preventDefault();
        onClose();
        return;
      }

      if (event.key !== "Tab" || !drawerRef.current) {
        return;
      }

      const focusable = Array.from(
        drawerRef.current.querySelectorAll<HTMLElement>(
          'a[href], button:not([disabled]), [tabindex]:not([tabindex="-1"])',
        ),
      ).filter((element) => !element.hasAttribute("aria-hidden"));

      if (focusable.length === 0) {
        event.preventDefault();
        drawerRef.current.focus();
        return;
      }

      const first = focusable[0];
      const last = focusable[focusable.length - 1];
      const focusIsInside = drawerRef.current.contains(document.activeElement);

      if (!focusIsInside) {
        event.preventDefault();
        (event.shiftKey ? last : first).focus();
        return;
      }

      if (event.shiftKey && document.activeElement === first) {
        event.preventDefault();
        last.focus();
      } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault();
        first.focus();
      }
    }

    window.addEventListener("keydown", handleKeyDown);

    return () => {
      window.clearTimeout(focusRequest);
      document.body.style.overflow = previousOverflow;
      window.removeEventListener("keydown", handleKeyDown);
      previousFocus?.focus();
    };
  }, [onClose]);

  const assignedGate = flight?.gate_number
    ? gates.find((gate) => gate.gate_number === flight.gate_number)
    : undefined;
  const assignedRunway = flight?.runway_code
    ? runways.find((runway) => runway.runway_code === flight.runway_code)
    : undefined;
  const relatedIncidents = flight
    ? incidents.filter((incident) => relatedToFlight(incident, flight.flight_number))
    : [];

  return (
    <div className="fixed inset-0 z-50">
      <button
        type="button"
        aria-label="Close flight details"
        onClick={onClose}
        className="animate-overlay-in absolute inset-0 h-full w-full cursor-default bg-black/65 backdrop-blur-sm"
      />
      <aside
        ref={drawerRef}
        role="dialog"
        aria-modal="true"
        aria-labelledby="flight-details-title"
        tabIndex={-1}
        className="animate-drawer-in absolute inset-y-0 right-0 flex w-full flex-col border-l border-white/10 bg-surface/95 shadow-[-24px_0_80px_rgba(0,0,0,0.45)] backdrop-blur-2xl sm:w-4/5 sm:max-w-[500px]"
      >
        {flight ? (
          <FlightHeader
            flight={flight}
            closeButtonRef={closeButtonRef}
            onClose={onClose}
          />
        ) : (
          <header className="flex items-start justify-between gap-4 border-b border-white/10 px-5 py-5 sm:px-6">
            <div>
              <p className="label">Flight details</p>
              <h2 id="flight-details-title" className="mt-2 text-xl font-semibold text-white">
                {loading ? "Loading flight…" : "Flight unavailable"}
              </h2>
            </div>
            <button
              ref={closeButtonRef}
              type="button"
              onClick={onClose}
              aria-label="Close flight details"
              className="flex h-9 w-9 items-center justify-center rounded-xl border border-white/10 bg-white/[0.04] text-muted hover:text-alert"
            >
              <X className="h-4 w-4" />
            </button>
          </header>
        )}

        <div className="flex-1 overflow-y-auto px-5 py-6 sm:px-6">
          {loading && !flight ? (
            <div aria-label="Loading flight details" className="space-y-5">
              <div className="flex items-center gap-3 text-sm text-muted">
                <LoaderCircle className="h-4 w-4 animate-spin text-cyan" />
                Loading current flight information…
              </div>
              {[1, 2, 3, 4].map((item) => (
                <div key={item} className="animate-pulse-soft rounded-2xl border border-white/10 p-4">
                  <div className="h-4 w-32 rounded bg-white/[0.08]" />
                  <div className="mt-4 grid grid-cols-2 gap-3">
                    <div className="h-16 rounded-xl bg-white/[0.05]" />
                    <div className="h-16 rounded-xl bg-white/[0.05]" />
                  </div>
                </div>
              ))}
            </div>
          ) : error && !flight ? (
            <div className="rounded-2xl border border-alert/25 bg-alert/[0.07] p-5 text-center">
              <AlertTriangle className="mx-auto h-6 w-6 text-alert" />
              <p className="mt-3 text-sm font-medium text-white">Could not load this flight</p>
              <p className="mt-1 text-xs text-alert">{error}</p>
              <button
                type="button"
                onClick={() => void loadDetails()}
                className="mt-4 rounded-xl border border-alert/25 px-4 py-2 text-xs font-medium text-alert hover:bg-alert/10"
              >
                Try again
              </button>
            </div>
          ) : flight ? (
            <div className="space-y-7">
              <FlightInfoSections
                flight={flight}
                gate={optionalErrors.gates ? undefined : assignedGate}
                runway={optionalErrors.runways ? undefined : assignedRunway}
              />
              <WeatherSection weather={weather} unavailable={optionalErrors.weather} />
              <IncidentSection
                ref={incidentSectionRef}
                incidents={relatedIncidents}
                unavailable={optionalErrors.incidents}
              />
              <QuickActionsSection
                flight={flight}
                hasIncident={relatedIncidents.length > 0}
                onViewIncident={() => {
                  incidentSectionRef.current?.scrollIntoView({ behavior: "smooth" });
                  incidentSectionRef.current?.focus();
                }}
                onViewGate={() => {
                  if (!flight.gate_number) {
                    return;
                  }

                  const gatePath = `/operations-map?gate=${encodeURIComponent(
                    flight.gate_number,
                  )}`;
                  onClose();
                  navigate(gatePath);
                }}
              />
            </div>
          ) : null}
        </div>
      </aside>
    </div>
  );
}
