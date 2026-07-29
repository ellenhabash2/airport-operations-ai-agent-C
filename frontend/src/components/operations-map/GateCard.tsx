import { Plane } from "lucide-react";

import type { Flight, Gate } from "../../types/api";

const gateStatusStyles: Record<string, string> = {
  available: "border-clear/25 bg-clear/[0.07] text-clear",
  occupied: "border-warning/25 bg-warning/[0.07] text-warning",
  closed: "border-alert/25 bg-alert/[0.07] text-alert",
  maintenance: "border-alert/25 bg-alert/[0.07] text-alert",
};

const gateStatusDots: Record<string, string> = {
  available: "bg-clear shadow-[0_0_10px_rgba(36,212,138,0.65)]",
  occupied: "bg-warning shadow-[0_0_10px_rgba(247,169,40,0.65)]",
  closed: "bg-alert shadow-[0_0_10px_rgba(243,79,118,0.65)]",
  maintenance: "bg-alert shadow-[0_0_10px_rgba(243,79,118,0.65)]",
};

function capitalise(value: string): string {
  return value.charAt(0).toUpperCase() + value.slice(1);
}

interface GateCardProps {
  gate: Gate;
  flight?: Flight;
  onSelect: (gate: Gate) => void;
}

export default function GateCard({ gate, flight, onSelect }: GateCardProps) {
  const status = gate.status.toLowerCase();
  const displayStatus =
    status === "closed"
      ? "Closed"
      : status === "occupied" && flight
        ? flight.flight_number
        : status === "available"
          ? "Available"
          : capitalise(status);

  return (
    <button
      type="button"
      onClick={() => onSelect(gate)}
      aria-label={`Gate ${gate.gate_number}, ${capitalise(status)}${
        flight ? `, assigned to flight ${flight.flight_number}` : ""
      }`}
      className={`group min-h-32 w-full rounded-2xl border p-4 text-left transition-all duration-200 hover:-translate-y-1 hover:shadow-[0_16px_30px_rgba(0,0,0,0.2)] ${
        gateStatusStyles[status] ?? "border-white/10 bg-white/[0.04] text-muted"
      }`}
    >
      <div className="flex items-start justify-between gap-3">
        <div>
          <p className="text-xs text-muted">Gate</p>
          <p className="mt-0.5 font-mono text-xl font-semibold text-white">
            {gate.gate_number}
          </p>
        </div>
        <span
          aria-hidden="true"
          className={`mt-1 h-2.5 w-2.5 rounded-full ${
            gateStatusDots[status] ?? "bg-muted"
          }`}
        />
      </div>

      <div className="mt-5 flex items-center gap-2">
        {flight && status === "occupied" && <Plane className="h-3.5 w-3.5" />}
        <span className="font-mono text-sm font-medium">{displayStatus}</span>
      </div>
      <p className="mt-1 truncate text-xs text-muted">
        {flight && status === "occupied"
          ? `To ${flight.destination}`
          : gate.terminal ?? "Terminal unavailable"}
      </p>
    </button>
  );
}
