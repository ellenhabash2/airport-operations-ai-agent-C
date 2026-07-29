const legendItems = [
  { label: "Available", color: "bg-clear" },
  { label: "Occupied", color: "bg-warning" },
  { label: "Closed", color: "bg-alert" },
];

export default function AirportLegend() {
  return (
    <div
      aria-label="Airport status legend"
      className="flex flex-wrap items-center gap-x-4 gap-y-2 rounded-2xl border border-white/10 bg-white/[0.035] px-4 py-3"
    >
      <span className="text-xs font-semibold text-white">Legend</span>
      {legendItems.map((item) => (
        <span key={item.label} className="flex items-center gap-1.5 text-xs text-muted">
          <span
            aria-hidden="true"
            className={`h-2 w-2 rounded-full ${item.color}`}
          />
          {item.label}
        </span>
      ))}
    </div>
  );
}
