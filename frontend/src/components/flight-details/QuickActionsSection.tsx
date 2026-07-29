import { Bot, Check, Clipboard, DoorOpen, TriangleAlert } from "lucide-react";
import { useState } from "react";
import { Link } from "react-router-dom";

import type { Flight } from "../../types/api";

interface QuickActionsSectionProps {
  flight: Flight;
  hasIncident: boolean;
  onViewIncident: () => void;
  onViewGate: () => void;
}

export default function QuickActionsSection({
  flight,
  hasIncident,
  onViewIncident,
  onViewGate,
}: QuickActionsSectionProps) {
  const [copied, setCopied] = useState(false);
  const prompt = `Explain the current operational status of flight ${flight.flight_number}.`;

  async function copyFlightNumber() {
    try {
      await navigator.clipboard.writeText(flight.flight_number);
      setCopied(true);
      window.setTimeout(() => setCopied(false), 1800);
    } catch {
      setCopied(false);
    }
  }

  const actionClass =
    "flex min-h-11 items-center gap-2 rounded-xl border border-white/10 bg-white/[0.035] px-3.5 py-2.5 text-left text-xs font-medium text-muted-light transition-all hover:border-cyan/25 hover:bg-cyan/[0.06] hover:text-cyan";

  return (
    <section aria-labelledby="flight-actions-heading">
      <h3 id="flight-actions-heading" className="text-sm font-semibold text-white">
        AI actions
      </h3>
      <div className="mt-4 grid gap-2 sm:grid-cols-2">
        <Link to={`/chat?prompt=${encodeURIComponent(prompt)}`} className={actionClass}>
          <Bot className="h-4 w-4 shrink-0" /> Ask AeroMind
        </Link>
        {flight.gate_number ? (
          <button
            type="button"
            onClick={onViewGate}
            className={actionClass}
          >
            <DoorOpen className="h-4 w-4 shrink-0" /> View assigned gate
          </button>
        ) : (
          <span className={`${actionClass} cursor-not-allowed opacity-50`}>
            <DoorOpen className="h-4 w-4 shrink-0" /> No assigned gate
          </span>
        )}
        {hasIncident && (
          <button type="button" onClick={onViewIncident} className={actionClass}>
            <TriangleAlert className="h-4 w-4 shrink-0" /> View related incident
          </button>
        )}
        <button type="button" onClick={() => void copyFlightNumber()} className={actionClass}>
          {copied ? (
            <Check className="h-4 w-4 shrink-0 text-clear" />
          ) : (
            <Clipboard className="h-4 w-4 shrink-0" />
          )}
          {copied ? "Copied" : "Copy flight number"}
        </button>
      </div>
    </section>
  );
}
