import { Plane, X } from "lucide-react";
import type { RefObject } from "react";

import type { Flight } from "../../types/api";
import StatusBadge from "./StatusBadge";

interface FlightHeaderProps {
  flight: Flight;
  closeButtonRef: RefObject<HTMLButtonElement | null>;
  onClose: () => void;
}

export default function FlightHeader({
  flight,
  closeButtonRef,
  onClose,
}: FlightHeaderProps) {
  return (
    <header className="border-b border-white/10 px-5 py-5 sm:px-6">
      <div className="flex items-start justify-between gap-4">
        <div className="flex min-w-0 items-start gap-3">
          <span className="flex h-10 w-10 shrink-0 items-center justify-center rounded-xl bg-gradient-to-br from-cyan to-accent text-white">
            <Plane className="h-5 w-5" />
          </span>
          <div className="min-w-0">
            <p className="label">Flight details</p>
            <h2
              id="flight-details-title"
              className="mt-1 font-mono text-2xl font-semibold text-white"
            >
              {flight.flight_number}
            </h2>
            <p className="mt-1 truncate text-sm text-muted-light">
              {flight.airline_name ?? "Airline not available"}
            </p>
            <div className="mt-3">
              <StatusBadge status={flight.status} />
            </div>
          </div>
        </div>
        <button
          ref={closeButtonRef}
          type="button"
          onClick={onClose}
          aria-label="Close flight details"
          className="flex h-9 w-9 shrink-0 items-center justify-center rounded-xl border border-white/10 bg-white/[0.04] text-muted transition-colors hover:border-alert/30 hover:text-alert"
        >
          <X className="h-4 w-4" />
        </button>
      </div>
    </header>
  );
}
