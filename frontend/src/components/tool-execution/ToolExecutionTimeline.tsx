import { Check, ChevronDown, CircleX, Wrench } from "lucide-react";
import type { ToolExecution } from "../../types/api";

const names: Record<string, string> = {
  get_flight_by_number: "Get flight by number", get_flight_by_id: "Get flight details", get_flight_details: "Get flight details",
  get_all_flights: "Review all flights", find_delayed_flights: "Find delayed flights", search_flights: "Search flights",
  get_available_gates: "Find available gates", assign_flight_to_gate: "Assign flight to gate",
  get_runway_status: "Check runway status", get_runway_by_code: "Get runway details", update_runway_status: "Update runway status",
  get_all_incidents: "Review incidents", get_active_incidents: "Get active incidents", get_latest_weather: "Check latest weather",
};
const access: Record<string, "read" | "write"> = {
  assign_flight_to_gate: "write", update_runway_status: "write", update_flight_status: "write", create_incident: "write", resolve_incident: "write",
};
const sensitive = /token|secret|password|credential|api.?key|authorization|sql|query/i;
const safeEntries = (value: Record<string, unknown>) => Object.entries(value).filter(([key]) => !sensitive.test(key));
const humanName = (tool: string) => names[tool] ?? tool.replaceAll("_", " ").replace(/\b\w/g, (letter) => letter.toUpperCase());

export default function ToolExecutionTimeline({ executions }: { executions?: ToolExecution[] | null }) {
  if (!executions?.length) return null;
  const ordered = [...executions].sort((a, b) => (a.sequence ?? 0) - (b.sequence ?? 0));
  return <details className="mb-4 rounded-xl border border-white/10 bg-black/15" open={ordered.length <= 2}>
    <summary className="flex cursor-pointer list-none items-center gap-2 p-3 text-xs text-muted-light"><Wrench className="h-3.5 w-3.5 text-cyan" /><span className="font-medium">Operational tools</span><span className="text-muted">{ordered.length} step{ordered.length === 1 ? "" : "s"}</span><ChevronDown className="ml-auto h-3.5 w-3.5" /></summary>
    <ol className="border-t border-white/10 px-3 py-2">{ordered.map((execution, index) => {
      const failed = execution.status === "error";
      const classification = execution.access ?? access[execution.tool];
      return <li key={execution.call_id ?? `${execution.tool}-${execution.sequence ?? index}`} role="status" aria-label={failed ? "Tool failed" : "Tool succeeded"} className="relative flex gap-3 border-l border-white/10 pb-4 pl-5 last:pb-2">
        <span className={`absolute -left-2 top-1 flex h-4 w-4 items-center justify-center rounded-full ${failed ? "bg-alert" : "bg-clear"}`}>{failed ? <CircleX className="h-3 w-3 text-white" /> : <Check className="h-3 w-3 text-paper" />}</span>
        <div className="min-w-0 flex-1"><div className="flex flex-wrap items-center gap-2"><span className="text-xs text-muted">{execution.sequence ?? index + 1}.</span><p className="text-sm font-medium text-white">{humanName(execution.tool)}</p><span className={failed ? "text-xs text-alert" : "text-xs text-clear"}>{failed ? "Failed" : "Completed"}</span>{classification && <span className="rounded bg-white/[0.06] px-1.5 py-0.5 text-[10px] uppercase text-muted">{classification}</span>}<span className="text-xs text-muted">{execution.duration_ms} ms</span></div>
          {safeEntries(execution.arguments).length > 0 && <p className="mt-1 break-words text-xs text-muted">{safeEntries(execution.arguments).map(([key, value]) => `${key.replaceAll("_", " ")}: ${String(value)}`).join(" · ")}</p>}
          {execution.error_code && <p className="mt-1 text-xs text-alert">Error code: {execution.error_code}</p>}
        </div>
      </li>;
    })}</ol>
  </details>;
}
