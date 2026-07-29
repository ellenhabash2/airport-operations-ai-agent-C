import { CalendarClock, MapPin, Plane, X } from "lucide-react";
import { useEffect, useRef } from "react";

import type { Flight, Gate } from "../../types/api";

function formatDateTime(value: string | null | undefined): string {
  if (!value) {
    return "Not scheduled";
  }

  return new Date(value).toLocaleString([], {
    dateStyle: "medium",
    timeStyle: "short",
  });
}

function capitalise(value: string): string {
  return value.charAt(0).toUpperCase() + value.slice(1);
}

interface GateDetailsDrawerProps {
  gate: Gate;
  flight?: Flight;
  onClose: () => void;
  onSelectFlight: (flight: Flight) => void;
}

export default function GateDetailsDrawer({
  gate,
  flight,
  onClose,
  onSelectFlight,
}: GateDetailsDrawerProps) {
  const closeButtonRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    const previousOverflow = document.body.style.overflow;
    const previousFocus = document.activeElement as HTMLElement | null;
    document.body.style.overflow = "hidden";
    closeButtonRef.current?.focus();

    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") {
        onClose();
      }
    }

    window.addEventListener("keydown", handleKeyDown);

    return () => {
      document.body.style.overflow = previousOverflow;
      window.removeEventListener("keydown", handleKeyDown);
      previousFocus?.focus();
    };
  }, [onClose]);

  const details = flight
    ? [
        ["Airline", flight.airline_name ?? "Not available"],
        [
          "Aircraft",
          [flight.aircraft_type, flight.aircraft_registration]
            .filter(Boolean)
            .join(" · ") || "Not available",
        ],
        ["Flight status", capitalise(flight.status)],
        ["Destination", flight.destination],
        ["Scheduled departure", formatDateTime(flight.departure_time)],
        ["Scheduled arrival", formatDateTime(flight.arrival_time)],
      ]
    : [];

  return (
    <div className="fixed inset-0 z-50" role="presentation">
      <button
        type="button"
        aria-label="Close gate information"
        onClick={onClose}
        className="absolute inset-0 h-full w-full cursor-default bg-black/65 backdrop-blur-sm"
      />

      <aside
        role="dialog"
        aria-modal="true"
        aria-labelledby="gate-details-title"
        className="animate-drawer-in absolute inset-y-0 right-0 flex w-full max-w-md flex-col border-l border-white/10 bg-surface/95 shadow-[-24px_0_80px_rgba(0,0,0,0.4)] backdrop-blur-2xl"
      >
        <div className="flex items-start justify-between gap-4 border-b border-white/10 px-5 py-5 sm:px-6">
          <div>
            <p className="label">Gate information</p>
            <h2
              id="gate-details-title"
              className="mt-2 font-mono text-2xl font-semibold text-white"
            >
              Gate {gate.gate_number}
            </h2>
          </div>
          <button
            ref={closeButtonRef}
            type="button"
            onClick={onClose}
            aria-label="Close gate information"
            className="flex h-9 w-9 items-center justify-center rounded-xl border border-white/10 bg-white/[0.04] text-muted transition-colors hover:border-alert/30 hover:text-alert"
          >
            <X className="h-4 w-4" />
          </button>
        </div>

        <div className="flex-1 overflow-y-auto px-5 py-6 sm:px-6">
          <div className="grid grid-cols-2 gap-3">
            <div className="rounded-2xl border border-white/10 bg-white/[0.035] p-4">
              <p className="flex items-center gap-1.5 text-xs text-muted">
                <MapPin className="h-3.5 w-3.5" /> Terminal
              </p>
              <p className="mt-2 text-sm font-medium text-white">
                {gate.terminal ?? "Not available"}
              </p>
            </div>
            <div className="rounded-2xl border border-white/10 bg-white/[0.035] p-4">
              <p className="flex items-center gap-1.5 text-xs text-muted">
                <CalendarClock className="h-3.5 w-3.5" /> Gate status
              </p>
              <p className="mt-2 text-sm font-medium text-white">
                {capitalise(gate.status)}
              </p>
            </div>
          </div>

          {!flight ? (
            <div className="mt-5 rounded-2xl border border-clear/20 bg-clear/[0.06] px-5 py-8 text-center">
              <Plane className="mx-auto h-6 w-6 text-clear" />
              <p className="mt-3 text-sm font-medium text-white">
                No active flight assigned.
              </p>
              <p className="mt-1 text-xs text-muted">
                This gate is ready for its next operation.
              </p>
            </div>
          ) : (
            <>
              <button
                type="button"
                onClick={() => onSelectFlight(flight)}
                className="mt-5 flex w-full items-center justify-between gap-4 rounded-2xl border border-cyan/20 bg-cyan/[0.06] px-4 py-3 text-left transition-all hover:border-cyan/35 hover:bg-cyan/10"
              >
                <span>
                  <span className="block text-xs text-muted">Assigned flight</span>
                  <span className="mt-1 block font-mono text-base font-semibold text-white">
                    {flight.flight_number}
                  </span>
                </span>
                <Plane className="h-5 w-5 text-cyan" />
              </button>
              <dl className="mt-3 divide-y divide-white/10 rounded-2xl border border-white/10 bg-white/[0.025] px-4">
                {details.map(([label, value]) => (
                  <div
                    key={label}
                    className="grid gap-1 py-3.5 sm:grid-cols-[145px_1fr] sm:gap-4"
                  >
                    <dt className="text-xs text-muted">{label}</dt>
                    <dd className="text-sm text-white sm:text-right">{value}</dd>
                  </div>
                ))}
              </dl>
            </>
          )}
        </div>
      </aside>
    </div>
  );
}
