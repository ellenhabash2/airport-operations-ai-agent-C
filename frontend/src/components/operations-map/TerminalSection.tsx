import { Building2 } from "lucide-react";

import type { Flight, Gate } from "../../types/api";
import GateCard from "./GateCard";

interface TerminalSectionProps {
  name: string;
  gates: Gate[];
  flightsByGate: Map<string, Flight>;
  onSelectGate: (gate: Gate) => void;
}

export default function TerminalSection({
  name,
  gates,
  flightsByGate,
  onSelectGate,
}: TerminalSectionProps) {
  const occupied = gates.filter((gate) => gate.status === "occupied").length;
  const available = gates.filter((gate) => gate.status === "available").length;

  return (
    <article className="overflow-hidden rounded-3xl border border-white/10 bg-white/[0.035] backdrop-blur-xl">
      <div className="flex flex-wrap items-center justify-between gap-4 border-b border-white/10 px-5 py-4 sm:px-6">
        <div className="flex items-center gap-3">
          <span className="flex h-9 w-9 items-center justify-center rounded-xl bg-accent/10 text-cyan">
            <Building2 className="h-4 w-4" />
          </span>
          <div>
            <h2 className="font-semibold text-white">{name}</h2>
            <p className="text-xs text-muted">{gates.length} gates</p>
          </div>
        </div>

        <dl className="flex items-center gap-4 text-xs">
          <div>
            <dt className="text-muted">Occupied</dt>
            <dd className="mt-0.5 font-mono font-semibold text-warning">
              {occupied}
            </dd>
          </div>
          <div>
            <dt className="text-muted">Available</dt>
            <dd className="mt-0.5 font-mono font-semibold text-clear">
              {available}
            </dd>
          </div>
        </dl>
      </div>

      {gates.length === 0 ? (
        <p className="px-6 py-10 text-center text-sm text-muted">
          No gates are configured for this terminal.
        </p>
      ) : (
        <div className="grid gap-3 p-4 sm:grid-cols-2 sm:p-5 lg:grid-cols-3 2xl:grid-cols-4">
          {gates.map((gate) => (
            <GateCard
              key={gate.id}
              gate={gate}
              flight={flightsByGate.get(gate.gate_number)}
              onSelect={onSelectGate}
            />
          ))}
        </div>
      )}
    </article>
  );
}
