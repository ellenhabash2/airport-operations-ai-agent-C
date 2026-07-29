import { CircleParking, DoorOpen, Plane, Route } from "lucide-react";

interface AirportSummaryCardsProps {
  availableGates: number;
  occupiedGates: number;
  closedRunways: number;
  assignedFlights: number;
}

export default function AirportSummaryCards({
  availableGates,
  occupiedGates,
  closedRunways,
  assignedFlights,
}: AirportSummaryCardsProps) {
  const summaries = [
    {
      label: "Available gates",
      value: availableGates,
      icon: DoorOpen,
      color: "bg-clear/10 text-clear",
    },
    {
      label: "Occupied gates",
      value: occupiedGates,
      icon: CircleParking,
      color: "bg-warning/10 text-warning",
    },
    {
      label: "Closed runways",
      value: closedRunways,
      icon: Route,
      color: "bg-alert/10 text-alert",
    },
    {
      label: "Flights at gates",
      value: assignedFlights,
      icon: Plane,
      color: "bg-cyan/10 text-cyan",
    },
  ];

  return (
    <section aria-label="Airport map summary" className="mt-6 grid gap-3 sm:grid-cols-2 xl:grid-cols-4">
      {summaries.map((summary) => (
        <article
          key={summary.label}
          className="rounded-2xl border border-white/10 bg-white/[0.04] p-4 backdrop-blur-xl transition-all duration-200 hover:-translate-y-0.5 hover:border-cyan/20"
        >
          <div className="flex items-center gap-3">
            <span
              className={`flex h-10 w-10 items-center justify-center rounded-xl ${summary.color}`}
            >
              <summary.icon className="h-5 w-5" />
            </span>
            <div>
              <p className="text-xs text-muted">{summary.label}</p>
              <p className="mt-0.5 font-mono text-2xl font-semibold text-white">
                {summary.value}
              </p>
            </div>
          </div>
        </article>
      ))}
    </section>
  );
}
