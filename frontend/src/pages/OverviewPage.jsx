import { useEffect, useState } from "react";
import Brand from "../components/Brand";
import "../styles/overview.css";

function OverviewPage() {
    const [username, setUsername] = useState("Operator");

    useEffect(() => {
        const storedUser = JSON.parse(localStorage.getItem("user"));

        if (storedUser?.username) {
            setUsername(storedUser.username);
        }
    }, []);

    const today = new Date().toLocaleDateString("en-GB", {
        weekday: "long",
        day: "numeric",
        month: "long",
    });

    return (
        <div className="overview-page">

            <header className="dashboard-header">

                <Brand />

                <div className="header-right">

                    <span className="status-pill">
                        ● All systems operational
                    </span>

                    <button className="logout-btn">
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
                          <p className="stat-title">Flights</p>
                          <h2>128</h2>
                          <span className="stat-green">12 delayed</span>
                      </div>

                      <div className="stat-card">
                          <p className="stat-title">Gates</p>
                          <h2>34</h2>
                          <span className="stat-green">29 available</span>
                      </div>

                      <div className="stat-card">
                          <p className="stat-title">Runways</p>
                          <h2>3</h2>
                          <span className="stat-green">All active</span>
                      </div>

                      <div className="stat-card">
                          <p className="stat-title">Weather</p>
                          <h2>24°C</h2>
                          <span className="stat-blue">Clear skies</span>
                      </div>

                  </div>
                  <div className="bottom-grid">

                    <div className="operations-card">

                        <h3>Live Operations</h3>

                        <div className="operation-row">
                            <span>Scheduled Flights</span>
                            <strong>128</strong>
                        </div>

                        <div className="operation-row">
                            <span>Delayed Flights</span>
                            <strong>12</strong>
                        </div>

                        <div className="operation-row">
                            <span>Available Gates</span>
                            <strong>29</strong>
                        </div>

                        <div className="operation-row">
                            <span>Active Runways</span>
                            <strong>3</strong>
                        </div>

                    </div>

                    <div className="assistant-card">

                        <h3>AI Operations Assistant</h3>

                        <p>
                            Ask AeroMind about flights, gates, incidents,
                            weather or airport operations.
                        </p>

                        <button className="assistant-btn">
                            Open AI Chat
                        </button>

                    </div>

                  </div>
                  <div className="priority-alert">

                      <div className="alert-header">

                          <span className="alert-badge">
                              HIGH PRIORITY
                          </span>

                          <span className="alert-time">
                              Updated 2 min ago
                          </span>

                      </div>

                       <h3>Runway Inspection Required</h3>

                      <p>
                          Runway RWY-27 has been flagged for a scheduled safety inspection.
                          Airport operations continue normally, but maintenance staff should
                          complete the inspection before the next departure window.
                      </p>

                      <button className="alert-btn">
                          View Incident Details
                      </button>

                  </div>

                </section>

            </main>

        </div>
    );
}

export default OverviewPage;