interface StatusBadgeProps {
  status?: string | null;
  kind?: "flight" | "gate" | "runway" | "incident";
}

const styles: Record<string, string> = {
  available: "border-clear/20 bg-clear/10 text-clear", open: "border-clear/20 bg-clear/10 text-clear",
  "on time": "border-clear/20 bg-clear/10 text-clear", landed: "border-clear/20 bg-clear/10 text-clear",
  arrived: "border-clear/20 bg-clear/10 text-clear", low: "border-clear/20 bg-clear/10 text-clear",
  delayed: "border-warning/20 bg-warning/10 text-warning", occupied: "border-warning/20 bg-warning/10 text-warning",
  medium: "border-warning/20 bg-warning/10 text-warning", boarding: "border-accent/20 bg-accent/10 text-cyan",
  scheduled: "border-white/15 bg-white/[0.07] text-muted-light", departed: "border-violet/20 bg-violet/10 text-violet",
  cancelled: "border-alert/20 bg-alert/10 text-alert", closed: "border-alert/20 bg-alert/10 text-alert",
  maintenance: "border-alert/20 bg-alert/10 text-alert", high: "border-alert/20 bg-alert/10 text-alert",
  critical: "border-alert/20 bg-alert/10 text-alert", success: "border-clear/20 bg-clear/10 text-clear",
  failed: "border-alert/20 bg-alert/10 text-alert",
};

export default function StatusBadge({ status }: StatusBadgeProps) {
  const normalized = String(status || "unknown").trim().toLowerCase().replaceAll("_", " ");
  const label = normalized.replace(/\b\w/g, (letter) => letter.toUpperCase());
  return <span className={`inline-flex items-center gap-1.5 rounded-full border px-2.5 py-1 text-xs font-medium ${styles[normalized] ?? "border-white/10 bg-white/[0.05] text-muted"}`}>
    <span aria-hidden="true" className="h-1.5 w-1.5 rounded-full bg-current" />{label}
  </span>;
}
