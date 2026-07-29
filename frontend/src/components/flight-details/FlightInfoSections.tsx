import { Clock3, MapPin, Plane } from "lucide-react";

import type { Flight, Gate, Runway } from "../../types/api";
import StatusBadge from "./StatusBadge";

const NOT_AVAILABLE = "Not Available";

function formatDateTime(value: string | null | undefined): string {
  if (!value) {
    return NOT_AVAILABLE;
  }

  return new Date(value).toLocaleString([], {
    dateStyle: "medium",
    timeStyle: "short",
  });
}

function DetailGrid({ entries }: { entries: Array<[string, string]> }) {
  return (
    <dl className="mt-4 grid gap-3 sm:grid-cols-2">
      {entries.map(([label, value]) => (
        <div key={label} className="rounded-xl border border-white/10 bg-white/[0.03] p-3">
          <dt className="text-xs text-muted">{label}</dt>
          <dd className="mt-1 break-words text-sm text-white">{value}</dd>
        </div>
      ))}
    </dl>
  );
}

interface FlightInfoSectionsProps {
  flight: Flight;
  gate?: Gate;
  runway?: Runway;
}

export default function FlightInfoSections({
  flight,
  gate,
  runway,
}: FlightInfoSectionsProps) {
  const runwayStatus = runway?.status === "available" ? "open" : runway?.status;
  const delayed = flight.status.toLowerCase() === "delayed";

  return (
    <>
      <section aria-labelledby="flight-information-heading">
        <h3
          id="flight-information-heading"
          className="flex items-center gap-2 text-sm font-semibold text-white"
        >
          <Plane className="h-4 w-4 text-cyan" /> Flight information
        </h3>
        <DetailGrid
          entries={[
            ["Flight number", flight.flight_number],
            ["Airline", flight.airline_name ?? NOT_AVAILABLE],
            ["Aircraft", flight.aircraft_type ?? NOT_AVAILABLE],
            ["Registration", flight.aircraft_registration ?? NOT_AVAILABLE],
            ["Origin", flight.origin || NOT_AVAILABLE],
            ["Destination", flight.destination || NOT_AVAILABLE],
          ]}
        />
      </section>

      <section aria-labelledby="airport-operations-heading">
        <h3
          id="airport-operations-heading"
          className="flex items-center gap-2 text-sm font-semibold text-white"
        >
          <MapPin className="h-4 w-4 text-cyan" /> Airport operations
        </h3>
        <div className="mt-4 grid gap-3 sm:grid-cols-2">
          <div className="rounded-xl border border-white/10 bg-white/[0.03] p-3">
            <p className="text-xs text-muted">Terminal</p>
            <p className="mt-1 text-sm text-white">{flight.terminal ?? NOT_AVAILABLE}</p>
          </div>
          <div className="rounded-xl border border-white/10 bg-white/[0.03] p-3">
            <p className="text-xs text-muted">Gate</p>
            <p className="mt-1 text-sm text-white">{flight.gate_number ?? NOT_AVAILABLE}</p>
          </div>
          <div className="rounded-xl border border-white/10 bg-white/[0.03] p-3">
            <p className="text-xs text-muted">Runway</p>
            <div className="mt-1 flex flex-wrap items-center gap-2 text-sm text-white">
              {flight.runway_code ?? NOT_AVAILABLE}
              {runwayStatus && <StatusBadge status={runwayStatus} kind="runway" />}
            </div>
          </div>
          <div className="rounded-xl border border-white/10 bg-white/[0.03] p-3">
            <p className="text-xs text-muted">Boarding status</p>
            <div className="mt-1"><StatusBadge status={flight.status} /></div>
          </div>
          <div className="rounded-xl border border-white/10 bg-white/[0.03] p-3 sm:col-span-2">
            <p className="text-xs text-muted">Assigned gate status</p>
            <div className="mt-1">
              {gate ? (
                <StatusBadge status={gate.status} kind="gate" />
              ) : (
                <span className="text-sm text-white">{NOT_AVAILABLE}</span>
              )}
            </div>
          </div>
        </div>
      </section>

      <section aria-labelledby="schedule-heading">
        <h3
          id="schedule-heading"
          className="flex items-center gap-2 text-sm font-semibold text-white"
        >
          <Clock3 className="h-4 w-4 text-cyan" /> Schedule
        </h3>
        <DetailGrid
          entries={[
            ["Scheduled departure", formatDateTime(flight.departure_time)],
            ["Estimated departure", formatDateTime(flight.estimated_departure_time)],
            ["Actual departure", formatDateTime(flight.actual_departure_time)],
            ["Scheduled arrival", formatDateTime(flight.arrival_time)],
            ["Estimated arrival", formatDateTime(flight.estimated_arrival_time)],
            ["Actual arrival", formatDateTime(flight.actual_arrival_time)],
          ]}
        />
      </section>

      <section aria-labelledby="delay-heading">
        <h3 id="delay-heading" className="text-sm font-semibold text-white">
          Delay information
        </h3>
        {delayed ? (
          <DetailGrid
            entries={[
              [
                "Delay duration",
                flight.delay_duration_minutes != null
                  ? `${flight.delay_duration_minutes} minutes`
                  : NOT_AVAILABLE,
              ],
              ["Delay reason", flight.delay_reason ?? NOT_AVAILABLE],
            ]}
          />
        ) : (
          <div className="mt-4 rounded-xl border border-clear/20 bg-clear/[0.06] px-4 py-3 text-sm text-clear">
            On Time
          </div>
        )}
      </section>
    </>
  );
}
