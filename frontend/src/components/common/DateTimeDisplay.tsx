interface DateTimeDisplayProps {
  value?: string | null;
  fallback?: string;
}

function formatUtc(value?: string | null, fallback = "Not available") {
  if (!value) return fallback;
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return fallback;
  return `${new Intl.DateTimeFormat("en-GB", {
    day: "2-digit",
    month: "short",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    hour12: false,
    timeZone: "UTC",
  }).format(date).replace(",", ",")} UTC`;
}

export default function DateTimeDisplay({ value, fallback }: DateTimeDisplayProps) {
  return <time dateTime={value ?? undefined}>{formatUtc(value, fallback)}</time>;
}
