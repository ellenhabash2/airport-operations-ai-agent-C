import { CloudSun, Eye, Wind } from "lucide-react";

import type { WeatherReport } from "../../types/api";

export default function WeatherSection({
  weather,
  unavailable = false,
}: {
  weather: WeatherReport | null;
  unavailable?: boolean;
}) {
  return (
    <section aria-labelledby="flight-weather-heading">
      <h3
        id="flight-weather-heading"
        className="flex items-center gap-2 text-sm font-semibold text-white"
      >
        <CloudSun className="h-4 w-4 text-cyan" /> Weather
      </h3>

      {!weather ? (
        <div className="mt-4 rounded-xl border border-white/10 bg-white/[0.03] px-4 py-5 text-sm text-muted">
          {unavailable
            ? "Weather information could not be loaded."
            : "No weather report is available for this flight."}
        </div>
      ) : (
        <div className="mt-4 rounded-2xl border border-white/10 bg-gradient-to-br from-cyan/[0.07] to-accent/[0.04] p-4">
          <div className="flex items-end justify-between gap-4">
            <div>
              <p className="text-xs text-muted">Condition</p>
              <p className="mt-1 text-sm font-medium capitalize text-white">
                {weather.condition}
              </p>
            </div>
            <p className="font-mono text-3xl font-semibold text-white">
              {weather.temperature}°C
            </p>
          </div>
          <div className="mt-4 grid grid-cols-2 gap-3 border-t border-white/10 pt-4">
            <div>
              <p className="flex items-center gap-1.5 text-xs text-muted">
                <Wind className="h-3.5 w-3.5" /> Wind
              </p>
              <p className="mt-1 font-mono text-sm text-white">
                {weather.wind_speed} kt
              </p>
            </div>
            <div>
              <p className="flex items-center gap-1.5 text-xs text-muted">
                <Eye className="h-3.5 w-3.5" /> Visibility
              </p>
              <p className="mt-1 font-mono text-sm text-white">
                {weather.visibility} km
              </p>
            </div>
          </div>
        </div>
      )}
    </section>
  );
}
