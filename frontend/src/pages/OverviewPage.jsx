import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import Brand from "../components/Brand";
import "../styles/overview.css";
import api from "../services/api";
import {
    Plane,
    DoorOpen,
    Route,
    CloudSun,
} from "lucide-react";

function OverviewPage() {

    const navigate = useNavigate();
    const [username, setUsername] = useState("Operator");
    const [flightsCount, setFlightsCount] = useState(0);
    const [delayedFlightsCount, setDelayedFlightsCount] = useState(0);
    const [flightsLoading, setFlightsLoading] = useState(true);
    const [gatesCount, setGatesCount] = useState(0);
    const [availableGatesCount, setAvailableGatesCount] = useState(0);
    const [gatesLoading, setGatesLoading] = useState(true);
    const [runwaysCount, setRunwaysCount] = useState(0);
    const [operationalRunwaysCount, setOperationalRunwaysCount] = useState(0);
    const [runwaysLoading, setRunwaysLoading] = useState(true);
    const [temperature, setTemperature] = useState("--");
    const [weatherCondition, setWeatherCondition] = useState("Unknown");
    const [weatherLoading, setWeatherLoading] = useState(true);
    const [landedFlightsCount, setLandedFlightsCount] = useState(0);
    const [occupiedGatesCount, setOccupiedGatesCount] = useState(0);
    const [maintenanceRunwaysCount, setMaintenanceRunwaysCount] = useState(0);
    const [priorityIncident, setPriorityIncident] = useState(null);
    const [incidentLoading, setIncidentLoading] = useState(true);

    useEffect(() => {
        try {
            const storedUser = localStorage.getItem("user");

            if (!storedUser) {
                return;
            }

            const user = JSON.parse(storedUser);

            if (user?.username) {
                setUsername(user.username);
            }
        } catch (error) {
            console.error("Failed to read user from localStorage:", error);
            localStorage.removeItem("user");
        }
    }, []);

    useEffect(() => {
    loadFlightsData();
    loadGatesData();
    loadRunwaysData();
    loadWeatherData();
    loadPriorityIncident();
    }, []);

    function handleLogout() {
    localStorage.removeItem("token");
    localStorage.removeItem("user");

    navigate("/", { replace: true });
    }

    const today = new Date().toLocaleDateString("en-GB", {
        weekday: "long",
        day: "numeric",
        month: "long",
    });

    async function loadFlightsData() {
    try {
        setFlightsLoading(true);

        const [flightsResponse, delayedResponse] = await Promise.all([
            api.get("/flights"),
            api.get("/flights/delayed"),
        ]);

        const flights = flightsResponse.data?.data ?? [];
        const delayedCount = delayedResponse.data?.count ?? 0;

        setFlightsCount(flights.length);
        setDelayedFlightsCount(delayedCount);
        const landed = flights.filter(
            flight => flight.status?.toUpperCase() === "LANDED"
        ).length;

        setLandedFlightsCount(landed);
    } catch (error) {
        console.error("Failed to load flights data:", error);

        setFlightsCount(0);
        setDelayedFlightsCount(0);
        setLandedFlightsCount(0);
    } finally {
        setFlightsLoading(false);
    }
    }

    async function loadGatesData() {
    try {
        setGatesLoading(true);

        const response = await api.get("/gates");

        const gates = response.data?.data ?? [];

        setGatesCount(gates.length);

        const available = gates.filter(
            gate => gate.status?.toUpperCase() === "AVAILABLE"
        ).length;

        setAvailableGatesCount(available);
        const occupied = gates.filter(
            gate => gate.status?.toUpperCase() === "OCCUPIED"
        ).length;

        setOccupiedGatesCount(occupied);

    } catch (error) {
        console.error("Failed to load gates:", error);

        setGatesCount(0);
        setAvailableGatesCount(0);
        setOccupiedGatesCount(0);
    } finally {
        setGatesLoading(false);
    }
   }

   async function loadRunwaysData() {
    try {
        setRunwaysLoading(true);

        const response = await api.get("/runways");

        const runways = response.data?.data ?? [];

        setRunwaysCount(runways.length);

        const operational = runways.filter(
            runway => runway.status?.toUpperCase() === "OPERATIONAL"
        ).length;

        setOperationalRunwaysCount(operational);
        const maintenance = runways.filter(
            runway => runway.status?.toUpperCase() === "MAINTENANCE"
        ).length;

        setMaintenanceRunwaysCount(maintenance);

    } catch (error) {
        console.error("Failed to load runways:", error);

        setRunwaysCount(0);
        setOperationalRunwaysCount(0);
        setMaintenanceRunwaysCount(0);
    } finally {
        setRunwaysLoading(false);
    }
    }

    async function loadWeatherData() {
    try {
        setWeatherLoading(true);

        const response = await api.get("/weather");

        const weatherReports = response.data?.data ?? [];
        const weather = weatherReports[0];

        if (weather) {
            setTemperature(Math.round(weather.temperature_c));
            setWeatherCondition(weather.condition);
        } else {
            setTemperature("--");
            setWeatherCondition("Unavailable");
        }

    } catch (error) {
        console.error("Failed to load weather:", error);

        setTemperature("--");
        setWeatherCondition("Unavailable");

    } finally {
        setWeatherLoading(false);
    }
   }

   async function loadPriorityIncident() {
    try {
        setIncidentLoading(true);

        const response = await api.get("/incidents");

        const incidents = response.data?.data ?? [];

        const activeIncidents = incidents.filter(
            incident =>
                incident.status === "OPEN" ||
                incident.status === "INVESTIGATING"
        );

        if (activeIncidents.length === 0) {
            setPriorityIncident(null);
            return;
        }

        const severityOrder = {
            CRITICAL: 4,
            HIGH: 3,
            MEDIUM: 2,
            LOW: 1,
        };

        const highestPriority = activeIncidents.reduce((best, current) => {

            if (!best) {
                return current;
            }

            return severityOrder[current.severity] >
                severityOrder[best.severity]
                ? current
                : best;

        }, null);

        setPriorityIncident(highestPriority);

    } catch (error) {

        console.error("Failed to load incidents:", error);

        setPriorityIncident(null);

    } finally {

        setIncidentLoading(false);
    }
    }

    return (
        <div className="overview-page">

            <header className="dashboard-header">

                <Brand />

                <div className="header-right">

                    <span className="status-pill">
                        ● All systems operational
                    </span>

                    <button
                        type="button"
                        className="logout-btn"
                        onClick={handleLogout}
                    >
                        Logout
                    </button>

                </div>

            </header>

            <main className="dashboard-content">

                <section className="left-panel">

                    <p className="dashboard-date">
                        {today}
                    </p>

                    <h1>
                        Welcome back,
                        <br />
                        {username}
                    </h1>

                    <p className="dashboard-description">
                        Here is the current airport status.
                    </p>


                </section>

                <section className="right-panel">

                  <div className="stats-grid">

                        <div className="stat-card">
                            <div className="stat-card-header">
                                <div className="stat-icon">
                                    <Plane size={22} />
                                </div>

                                <p className="stat-title">Flights</p>
                            </div>

                            <h2>
                                {flightsLoading ? "..." : flightsCount}
                            </h2>

                            <span
                                className={
                                    delayedFlightsCount > 0
                                        ? "stat-warning"
                                        : "stat-green"
                                }
                            >
                                {flightsLoading
                                    ? "Loading..."
                                    : `${delayedFlightsCount} delayed`}
                            </span>
                        </div>

                        <div className="stat-card">
                            <div className="stat-card-header">
                                <div className="stat-icon">
                                    <DoorOpen size={22} />
                                </div>

                                <p className="stat-title">Gates</p>
                            </div>

                            <h2>
                                {gatesLoading ? "..." : gatesCount}
                            </h2>

                            <span className="stat-green">
                                {gatesLoading
                                    ? "Loading..."
                                    : `${availableGatesCount} available`}
                            </span>
                        </div>

                        <div className="stat-card">
                            <div className="stat-card-header">
                                <div className="stat-icon">
                                    <Route size={22} />
                                </div>

                                <p className="stat-title">Runways</p>
                            </div>

                            <h2>
                                {runwaysLoading ? "..." : runwaysCount}
                            </h2>

                            <span className="stat-green">
                                {runwaysLoading
                                    ? "Loading..."
                                    : `${operationalRunwaysCount} operational`}
                            </span>
                        </div>

                        <div className="stat-card">
                            <div className="stat-card-header">
                                <div className="stat-icon">
                                    <CloudSun size={22} />
                                </div>

                                <p className="stat-title">Weather</p>
                            </div>

                            <h2>
                                {weatherLoading
                                    ? "..."
                                    : `${temperature}°C`}
                            </h2>

                            <span className="stat-blue">
                                {weatherLoading
                                    ? "Loading..."
                                    : weatherCondition}
                            </span>
                        </div>

                    </div>
                  <div className="bottom-grid">

                    <div className="operations-card">

                        <h3>Live Operations</h3>

                        <div className="operation-row">
                            <span>Landed Flights</span>

                            <strong>
                                {flightsLoading ? "..." : landedFlightsCount}
                            </strong>
                        </div>

                        <div className="operation-row">
                            <span>Occupied Gates</span>

                            <strong>
                                {gatesLoading ? "..." : occupiedGatesCount}
                            </strong>
                        </div>

                        <div className="operation-row">
                            <span>Runways in Maintenance</span>

                            <strong>
                                {runwaysLoading ? "..." : maintenanceRunwaysCount}
                            </strong>
                        </div>

                    </div>

                    <div className="assistant-card">

                        <h3>AI Operations Assistant</h3>

                        <p>
                            Ask AeroMind about flights, gates, incidents,
                            weather or airport operations.
                        </p>

                        <button
                            type="button"
                            className="assistant-btn"
                            onClick={() => navigate("/chat")}
                        >
                            Open AI Chat
                        </button>

                    </div>

                  </div>
                  <div className="priority-alert">

                        {incidentLoading ? (

                            <p>Loading incident...</p>

                        ) : !priorityIncident ? (

                            <div className="no-incidents">

                                <h3>No Active Incidents</h3>

                                <p>
                                    All airport operations are currently running normally.
                                </p>

                            </div>

                        ) : (

                            <>

                                <div className="alert-header">

                                    <span className={`alert-badge severity-${priorityIncident.severity.toLowerCase()}`}>
                                        {priorityIncident.severity}
                                    </span>

                                    <span className="alert-time">
                                        {priorityIncident.status}
                                    </span>

                                </div>

                                <h3>{priorityIncident.title}</h3>

                                <p>{priorityIncident.description}</p>
                                <div className="incident-details">

                                    <div>
                                        <strong>Location:</strong>{" "}
                                        {priorityIncident.location || "Unknown"}
                                    </div>

                                    <div>
                                        <strong>Reported:</strong>{" "}
                                        {new Date(priorityIncident.created_at).toLocaleString()}
                                    </div>

                                </div>
                                

                            </>

                        )}

                    </div>

                </section>

            </main>

        </div>
    );
}

export default OverviewPage;