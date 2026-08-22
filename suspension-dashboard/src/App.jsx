import { useEffect, useRef, useState } from "react";
import { connectWebSocket, disconnectWebSocket } from "./services/websocket";
import "./App.css";

const MAX_HISTORY = 30;

function Gauge({ title, value, unit, min, max, warning = false }) {
  const numeric = Number(value) || 0;
  const percent = Math.min(
    100,
    Math.max(0, ((numeric - min) / (max - min)) * 100)
  );

  return (
    <div className={`gauge-card ${warning ? "warning" : ""}`}>
      <div className="gauge-title">{title}</div>

      <div
        className="gauge"
        style={{
          "--progress": `${percent}%`,
        }}
      >
        <div className="gauge-inner">
          <strong>{numeric.toFixed(2)}</strong>
          <span>{unit}</span>
        </div>
      </div>

      <div className="gauge-range">
        <span>{min}</span>
        <span>{max}</span>
      </div>
    </div>
  );
}

function Status({ name, online }) {
  return (
    <div className="status-row">
      <span>{name}</span>
      <span className={online ? "online" : "offline"}>
        <i></i>
        {online ? "CONNECTED" : "DISCONNECTED"}
      </span>
    </div>
  );
}

function App() {
  const [telemetry, setTelemetry] = useState(null);
  const [connected, setConnected] = useState(false);
  const [history, setHistory] = useState([]);
  const [events, setEvents] = useState([]);

  const previousRef = useRef(null);

  useEffect(() => {
    // 1. Activate the STOMP WebSocket connection
    connectWebSocket();

    // 2. Handle incoming telemetry dispatched from websocket.js
    const handleTelemetry = (event) => {
      const data = event.detail;
      console.log("Dashboard telemetry:", data);

      setTelemetry(data);
      setConnected(true);

      setHistory((previous) => [
        ...previous.slice(-(MAX_HISTORY - 1)),
        {
          timestamp: data.timestamp,
          acceleration: Number(data.acceleration) || 0,
          force: Number(data.force) || 0,
          temperature: Number(data.temperature) || 0,
        },
      ]);

      const previous = previousRef.current;

      if (!previous) {
        setEvents((old) => [
          {
            type: "normal",
            message: "Telemetry stream started",
            timestamp: Date.now(),
          },
          ...old,
        ].slice(0, 10));
      } else {
        if (!previous.safeMode && data.safeMode) {
          setEvents((old) => [
            {
              type: "fault",
              message: "SAFE MODE ACTIVATED",
              timestamp: Date.now(),
            },
            ...old,
          ].slice(0, 10));
        }

        if (previous.safeMode && !data.safeMode) {
          setEvents((old) => [
            {
              type: "recovery",
              message: "System recovered from safe mode",
              timestamp: Date.now(),
            },
            ...old,
          ].slice(0, 10));
        }

        if (!previous.sensorError && data.sensorError) {
          setEvents((old) => [
            {
              type: "fault",
              message: "Sensor fault detected",
              timestamp: Date.now(),
            },
            ...old,
          ].slice(0, 10));
        }

        if (!previous.coilError && data.coilError) {
          setEvents((old) => [
            {
              type: "fault",
              message: "Coil fault detected",
              timestamp: Date.now(),
            },
            ...old,
          ].slice(0, 10));
        }
      }

      previousRef.current = data;
    };

    window.addEventListener("telemetry", handleTelemetry);

    return () => {
      window.removeEventListener("telemetry", handleTelemetry);
      disconnectWebSocket();
    };
  }, []);

  const safeMode = telemetry?.safeMode ?? false;
  const sensorError = telemetry?.sensorError ?? 0;
  const coilError = telemetry?.coilError ?? 0;

  return (
    <div className="dashboard">

      {/* HEADER */}
      <header className="topbar">
        <div>
          <div className="eyebrow">EMBEDDED VEHICLE CONTROL</div>
          <h1>Suspension Control Dashboard</h1>
          <p>STM32F4 • FreeRTOS • QEMU • MQTT</p>
        </div>

        <div className={`live-badge ${connected ? "live" : "dead"}`}>
          <span></span>
          {connected ? "LIVE" : "DISCONNECTED"}
        </div>
      </header>

      {/* CONNECTION BAR */}
      <section className="connection-panel">
        <Status name="QEMU / STM32" online={connected} />
        <Status name="MQTT Broker :1883" online={connected} />
        <Status name="Spring Boot" online={connected} />
        <Status name="WebSocket" online={connected} />
        <Status name="MySQL :3306" online={connected} />
      </section>

      {/* SAFE MODE ALERT */}
      {safeMode && (
        <div className="safe-alert">
          <div className="alert-icon">!</div>
          <div>
            <strong>SAFE MODE ACTIVE</strong>
            <span>
              Vehicle suspension protection has been activated.
            </span>
          </div>
        </div>
      )}

      {/* GAUGES */}
      <section className="section">
        <div className="section-heading">
          <div>
            <span className="eyebrow">LIVE TELEMETRY</span>
            <h2>Vehicle Parameters</h2>
          </div>

          {telemetry && (
            <span className="timestamp">
              T+ {telemetry.timestamp} ms
            </span>
          )}
        </div>

        <div className="gauges">

          <Gauge
            title="ACCELERATION"
            value={telemetry?.acceleration ?? 0}
            unit="g"
            min={-1}
            max={1}
          />

          <Gauge
            title="FORCE"
            value={telemetry?.force ?? 0}
            unit="N"
            min={-50}
            max={50}
          />

          <Gauge
            title="COIL CURRENT"
            value={telemetry?.current ?? 0}
            unit="A"
            min={0}
            max={1}
          />

          <Gauge
            title="TEMPERATURE"
            value={telemetry?.temperature ?? 0}
            unit="°C"
            min={20}
            max={60}
            warning={(telemetry?.temperature ?? 0) > 45}
          />

        </div>
      </section>

      {/* MAIN GRID */}
      <section className="main-grid">

        {/* SUSPENSION GRAPHIC */}
        <div className="panel suspension-panel">
          <div className="panel-header">
            <div>
              <span className="eyebrow">SYSTEM MODEL</span>
              <h2>Suspension System</h2>
            </div>

            <span className={`system-state ${safeMode ? "fault" : "normal"}`}>
              {safeMode ? "PROTECTION" : "NORMAL"}
            </span>
          </div>

          <div className="suspension">

            <div 
              className="vehicle-body"
              style={{
                transform: `translateY(${Math.max(-12, Math.min(12, (telemetry?.acceleration ?? 0) * 15))}px)`
              }}
            >
              VEHICLE
            </div>

            <div className="shock">

              <div className="mount"></div>

              <div 
                className="spring"
                style={{
                  transform: `scaleY(${Math.max(0.6, Math.min(1.4, 1 + (telemetry?.force ?? 0) / 80))})`
                }}
              >
                <span></span>
                <span></span>
                <span></span>
                <span></span>
                <span></span>
                <span></span>
                <span></span>
              </div>

              <div className="damper"></div>

              <div className="wheel"></div>

            </div>

            <div className="ground"></div>

          </div>

          <div className="system-description">
            <div>
              <strong>CONTROL</strong>
              <span>100 Hz</span>
            </div>

            <div>
              <strong>TELEMETRY</strong>
              <span>1 Hz</span>
            </div>

            <div>
              <strong>RTOS</strong>
              <span>FreeRTOS</span>
            </div>
          </div>
        </div>

        {/* STATUS */}
        <div className="panel status-panel">

          <div className="panel-header">
            <div>
              <span className="eyebrow">HEALTH</span>
              <h2>System Status</h2>
            </div>
          </div>

          <div className="health-grid">

            <div className="health-card">
              <span>SAFE MODE</span>
              <strong className={safeMode ? "red" : "green"}>
                {safeMode ? "ACTIVE" : "OFF"}
              </strong>
            </div>

            <div className="health-card">
              <span>SENSOR ERROR</span>
              <strong className={sensorError ? "red" : "green"}>
                {sensorError}
              </strong>
            </div>

            <div className="health-card">
              <span>COIL ERROR</span>
              <strong className={coilError ? "red" : "green"}>
                {coilError}
              </strong>
            </div>

            <div className="health-card">
              <span>TELEMETRY</span>
              <strong className={connected ? "green" : "red"}>
                {connected ? "STREAMING" : "STOPPED"}
              </strong>
            </div>

          </div>

          <div className="last-reading">
            <span>LAST TELEMETRY</span>

            <strong>
              {telemetry
                ? new Date().toLocaleTimeString()
                : "--:--:--"}
            </strong>
          </div>

        </div>
      </section>

      {/* TELEMETRY TREND */}
      <section className="panel trend-panel">

        <div className="panel-header">
          <div>
            <span className="eyebrow">REAL-TIME DATA</span>
            <h2>Acceleration Trend</h2>
          </div>

          <span className="sample-count">
            {history.length} samples
          </span>
        </div>

        <div className="chart">

          {history.length > 1 ? (
            <svg
              viewBox="0 0 1000 260"
              preserveAspectRatio="none"
            >

              <line
                x1="0"
                y1="130"
                x2="1000"
                y2="130"
                className="zero-line"
              />

              <polyline
                points={history
                  .map((item, index) => {

                    const x =
                      (index / (history.length - 1)) * 1000;

                    const value =
                      Math.max(
                        -1,
                        Math.min(1, item.acceleration)
                      );

                    const y =
                      130 - value * 100;

                    return `${x},${y}`;
                  })
                  .join(" ")}
                className="trend-line"
              />

            </svg>
          ) : (
            <div className="chart-empty">
              Waiting for telemetry...
            </div>
          )}

        </div>

        <div className="chart-footer">
          <span>-1.0 g</span>
          <span>0 g</span>
          <span>+1.0 g</span>
        </div>

      </section>

      {/* FAULT TIMELINE */}
      <section className="panel events-panel">

        <div className="panel-header">
          <div>
            <span className="eyebrow">DIAGNOSTICS</span>
            <h2>Fault Event Timeline</h2>
          </div>

          <span className="sample-count">
            {events.length} events
          </span>
        </div>

        {events.length === 0 ? (
          <div className="no-events">
            <span>✓</span>
            No faults detected
          </div>
        ) : (
          <div className="timeline">

            {events.map((event, index) => (

              <div
                className={`event ${event.type}`}
                key={`${event.timestamp}-${index}`}
              >

                <div className="event-dot"></div>

                <div className="event-content">
                  <strong>{event.message}</strong>

                  <span>
                    {new Date(
                      event.timestamp
                    ).toLocaleTimeString()}
                  </span>
                </div>

              </div>

            ))}

          </div>
        )}

      </section>

      {/* FOOTER */}
      <footer>
        <span>Vehicle Suspension Safety Controller</span>
        <span>STM32F4 • C++17 • FreeRTOS • MQTT • Spring Boot • React</span>
      </footer>

    </div>
  );
}

export default App;