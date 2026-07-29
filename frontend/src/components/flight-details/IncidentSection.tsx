import { AlertTriangle } from "lucide-react";
import { forwardRef } from "react";

import type { Incident } from "../../types/api";
import StatusBadge from "./StatusBadge";

function formatTime(value: string | null): string {
  if (!value) {
    return "Not Available";
  }

  return new Date(value).toLocaleString([], {
    dateStyle: "medium",
    timeStyle: "short",
  });
}

interface IncidentSectionProps {
  incidents: Incident[];
  unavailable?: boolean;
}

const IncidentSection = forwardRef<HTMLElement, IncidentSectionProps>(
  function IncidentSection({ incidents, unavailable = false }, ref) {
    return (
      <section ref={ref} tabIndex={-1} aria-labelledby="related-incidents-heading">
        <h3
          id="related-incidents-heading"
          className="flex items-center gap-2 text-sm font-semibold text-white"
        >
          <AlertTriangle className="h-4 w-4 text-cyan" /> Related incidents
        </h3>

        {incidents.length === 0 ? (
          <div className="mt-4 rounded-xl border border-white/10 bg-white/[0.03] px-4 py-5 text-sm text-muted">
            {unavailable
              ? "Related incidents could not be loaded."
              : "No incidents affecting this flight."}
          </div>
        ) : (
          <ul className="mt-4 space-y-3">
            {incidents.map((incident) => (
              <li
                key={incident.id}
                className="rounded-xl border border-white/10 bg-white/[0.03] p-4"
              >
                <div className="flex flex-wrap items-start justify-between gap-2">
                  <p className="text-sm font-medium text-white">{incident.title}</p>
                  <StatusBadge status={incident.severity} kind="incident" />
                </div>
                <dl className="mt-3 grid gap-2 text-xs sm:grid-cols-2">
                  <div>
                    <dt className="text-muted">Status</dt>
                    <dd className="mt-0.5 text-muted-light">
                      {incident.status ?? "Not Available"}
                    </dd>
                  </div>
                  <div>
                    <dt className="text-muted">Time</dt>
                    <dd className="mt-0.5 text-muted-light">
                      {formatTime(incident.created_at)}
                    </dd>
                  </div>
                </dl>
              </li>
            ))}
          </ul>
        )}
      </section>
    );
  },
);

export default IncidentSection;
