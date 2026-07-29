import { ArrowRight, Clipboard, Plane, Route } from "lucide-react";
import DateTimeDisplay from "../common/DateTimeDisplay";
import StatusBadge from "../common/StatusBadge";
import type { AgentPresentation, Flight } from "../../types/api";

type Data = Record<string, unknown>;
const text = (value: unknown, fallback = "Not available") => value == null || value === "" ? fallback : String(value);
const optionalText = (value: unknown, fallback = "Not assigned") => value == null || value === "" ? fallback : String(value);

function asFlight(data: Data): Flight {
  return {
    id: Number(data.id || 0), flight_number: text(data.flight_number), airline_name: data.airline_name as string | null,
    aircraft_registration: data.aircraft_registration as string | null, aircraft_type: data.aircraft_type as string | null,
    gate_number: data.gate_number as string | null, terminal: data.terminal as string | null, runway_code: data.runway_code as string | null,
    origin: text(data.origin), destination: text(data.destination), departure_time: data.departure_time as string | null,
    arrival_time: data.arrival_time as string | null, estimated_departure_time: data.estimated_departure_time as string | null,
    actual_departure_time: data.actual_departure_time as string | null, estimated_arrival_time: data.estimated_arrival_time as string | null,
    actual_arrival_time: data.actual_arrival_time as string | null, delay_duration_minutes: data.delay_duration_minutes as number | null,
    delay_reason: data.delay_reason as string | null, status: text(data.status, "unknown"),
  };
}

interface ActionProps { onOpenFlight?: (flight: Flight) => void; onPrompt?: (prompt: string) => void; }

function FlightStatusCard({ data, onOpenFlight, onPrompt }: { data: Data } & ActionProps) {
  const flight = asFlight(data);
  return <section aria-labelledby={`flight-${flight.flight_number}`} className="overflow-hidden rounded-2xl border border-cyan/15 bg-black/20">
    <div className="flex flex-wrap items-start justify-between gap-3 border-b border-white/10 p-4 sm:p-5">
      <div><h3 id={`flight-${flight.flight_number}`} className="text-xl font-bold text-white">{flight.flight_number}</h3><p className="mt-1 text-sm text-muted">{text(flight.airline_name)}</p></div>
      <StatusBadge status={flight.status} />
    </div>
    <div className="space-y-5 p-4 sm:p-5">
      <div className="grid grid-cols-[1fr_auto_1fr] items-center gap-3">
        <div><p className="text-xl font-semibold text-white">{flight.origin}</p><p className="text-xs text-muted">Origin</p></div>
        <ArrowRight aria-hidden="true" className="h-5 w-5 text-cyan" />
        <div className="text-right"><p className="text-xl font-semibold text-white">{flight.destination}</p><p className="text-xs text-muted">Destination</p></div>
      </div>
      <dl className="grid gap-4 border-y border-white/10 py-4 sm:grid-cols-2">
        <div><dt className="label">Departure</dt><dd className="mt-1 text-sm text-muted-light"><DateTimeDisplay value={flight.actual_departure_time || flight.estimated_departure_time || flight.departure_time} /></dd></div>
        <div><dt className="label">Arrival</dt><dd className="mt-1 text-sm text-muted-light"><DateTimeDisplay value={flight.actual_arrival_time || flight.estimated_arrival_time || flight.arrival_time} /></dd></div>
      </dl>
      <dl className="grid grid-cols-2 gap-4 sm:grid-cols-3">
        <div><dt className="label">Terminal</dt><dd className="mt-1 text-sm text-white">{optionalText(flight.terminal)}</dd></div>
        <div><dt className="label">Gate</dt><dd className="mt-1 text-sm text-white">{optionalText(flight.gate_number)}</dd></div>
        <div><dt className="label">Runway</dt><dd className="mt-1 text-sm text-white">{optionalText(flight.runway_code)}</dd></div>
      </dl>
      <div><p className="label">Aircraft</p><p className="mt-1 text-sm text-muted-light">{text(flight.aircraft_type)}{flight.aircraft_registration ? ` · ${flight.aircraft_registration}` : ""}</p></div>
      {flight.delay_duration_minutes != null && <p className="rounded-xl bg-warning/10 p-3 text-sm text-warning">Delayed {flight.delay_duration_minutes} minutes{flight.delay_reason ? ` · ${flight.delay_reason}` : ""}</p>}
      <div className="flex flex-wrap gap-2 border-t border-white/10 pt-4">
        {flight.id > 0 && onOpenFlight && <button type="button" onClick={() => onOpenFlight(flight)} className="rounded-lg bg-accent px-3 py-2 text-xs font-medium text-white">Open Flight Details</button>}
        {onPrompt && <button type="button" onClick={() => onPrompt(`What is causing delays for flight ${flight.flight_number}?`)} className="rounded-lg border border-white/10 px-3 py-2 text-xs text-muted-light">Ask About Delays</button>}
        <button type="button" onClick={() => void navigator.clipboard?.writeText(flight.flight_number)} className="flex items-center gap-1.5 rounded-lg border border-white/10 px-3 py-2 text-xs text-muted-light"><Clipboard className="h-3 w-3" /> Copy Flight Number</button>
      </div>
    </div>
  </section>;
}

function FlightListCard({ data, onOpenFlight }: { data: Data } & ActionProps) {
  const flights = Array.isArray(data.flights) ? data.flights.map((item) => asFlight(item as Data)) : [];
  return <section aria-labelledby="flight-list-title" className="overflow-hidden rounded-2xl border border-white/10 bg-black/20">
    <div className="flex items-center gap-2 border-b border-white/10 p-4"><Plane className="h-4 w-4 text-cyan" /><h3 id="flight-list-title" className="font-semibold text-white">Flights</h3><span className="text-xs text-muted">{flights.length} results</span></div>
    <div className="divide-y divide-white/10">{flights.map((flight) => <button key={`${flight.id}-${flight.flight_number}`} type="button" disabled={!onOpenFlight || flight.id <= 0} onClick={() => onOpenFlight?.(flight)} className="grid w-full grid-cols-[1fr_auto] gap-3 p-4 text-left hover:bg-white/[0.04] disabled:cursor-default">
      <div className="min-w-0"><p className="font-semibold text-white">{flight.flight_number}</p><p className="mt-1 truncate text-xs text-muted">{flight.origin} → {flight.destination} · <DateTimeDisplay value={flight.departure_time} /></p></div>
      <div className="text-right"><StatusBadge status={flight.status} /><p className="mt-1 text-xs text-muted">Gate {optionalText(flight.gate_number)}</p></div>
    </button>)}</div>
  </section>;
}

function GateAssignmentCard({ data }: { data: Data }) {
  const flight = data.flight as Data;
  const previous = data.previous_gate as Data | null;
  const next = data.new_gate as Data;
  return <section className="rounded-2xl border border-clear/20 bg-black/20 p-5"><div className="flex justify-between gap-3"><div><p className="label">Gate assignment</p><h3 className="mt-1 text-lg font-semibold text-white">{text(flight.flight_number)}</h3></div><StatusBadge status="success" kind="gate" /></div><dl className="mt-5 grid grid-cols-2 gap-4"><div><dt className="label">Previous gate</dt><dd className="mt-1 text-white">{optionalText(previous?.gate_number)}</dd></div><div><dt className="label">New gate</dt><dd className="mt-1 text-white">{optionalText(next.gate_number)}</dd></div></dl><p className="mt-4 text-sm text-muted">Assignment committed{next.terminal_code ? ` in Terminal ${text(next.terminal_code)}` : ""}.</p></section>;
}

function RunwayStatusCard({ data }: { data: Data }) {
  const runways = Array.isArray(data.runways) ? data.runways as Data[] : [data];
  return <section className="rounded-2xl border border-white/10 bg-black/20 p-4"><div className="mb-3 flex items-center gap-2"><Route className="h-4 w-4 text-cyan" /><h3 className="font-semibold text-white">Runway status</h3></div><div className="space-y-2">{runways.map((runway, index) => <div key={text(runway.id ?? runway.runway_code, String(index))} className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-white/10 p-3"><div><p className="font-semibold text-white">{text(runway.runway_code)}</p>{runway.closure_reason != null && <p className="mt-1 text-xs text-muted">{text(runway.closure_reason)}</p>}</div><StatusBadge status={text(runway.status)} kind="runway" /></div>)}</div></section>;
}

function IncidentListCard({ data }: { data: Data }) {
  const incidents = Array.isArray(data.incidents) ? data.incidents as Data[] : [];
  return <section className="rounded-2xl border border-white/10 bg-black/20 p-4"><h3 className="font-semibold text-white">Incidents</h3>{incidents.length === 0 ? <p className="mt-3 text-sm text-muted">No incidents found.</p> : <div className="mt-3 space-y-3">{incidents.map((incident, index) => <article key={text(incident.id, String(index))} className="rounded-xl border border-white/10 p-3"><div className="flex justify-between gap-3"><p className="font-medium text-white">{text(incident.title)}</p><StatusBadge status={text(incident.severity)} /></div><p className="mt-1 text-xs text-muted">{text(incident.location)} · {text(incident.status, "active")}</p><p className="mt-2 text-sm text-muted-light">{text(incident.description)}</p></article>)}</div>}</section>;
}

function OperationsOverviewCard({ data }: { data: Data }) {
  const flights = Array.isArray(data.delayed_flights) ? data.delayed_flights as Data[] : [];
  const incidents = Array.isArray(data.active_incidents) ? data.active_incidents as Data[] : [];
  const weather = data.weather as Data | null;
  return <section className="rounded-2xl border border-cyan/15 bg-black/20 p-4"><h3 className="font-semibold text-white">Operations overview</h3><div className="mt-4 grid gap-3 sm:grid-cols-3"><div className="rounded-xl bg-warning/10 p-3"><p className="label">Delayed flights</p><p className="mt-1 text-2xl font-semibold text-warning">{flights.length}</p></div><div className="rounded-xl bg-alert/10 p-3"><p className="label">Active incidents</p><p className="mt-1 text-2xl font-semibold text-alert">{incidents.length}</p></div><div className="rounded-xl bg-cyan/10 p-3"><p className="label">Weather</p><p className="mt-1 text-sm font-semibold text-cyan">{weather ? text(weather.condition) : "Unavailable"}</p>{weather?.temperature_c != null && <p className="mt-1 text-xs text-muted">{text(weather.temperature_c)}°C</p>}</div></div></section>;
}

export default function StructuredAnswerRenderer({ presentation, ...actions }: { presentation?: AgentPresentation | null } & ActionProps) {
  if (!presentation?.data) return null;
  if (presentation.type === "flight_status") return <FlightStatusCard data={presentation.data.flight as unknown as Data} {...actions} />;
  if (presentation.type === "flight_list") return <FlightListCard data={presentation.data} {...actions} />;
  if (presentation.type === "gate_assignment") return <GateAssignmentCard data={presentation.data as unknown as Data} />;
  if (presentation.type === "runway_status") return <RunwayStatusCard data={presentation.data} />;
  if (presentation.type === "incident_list") return <IncidentListCard data={presentation.data} />;
  if (presentation.type === "operations_overview") return <OperationsOverviewCard data={presentation.data} />;
  return null;
}
