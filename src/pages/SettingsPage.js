import React, { useState } from "react";
import { useApp } from "../context/AppContext";
import "./SettingsPage.css";

const SENSOR_LABELS = {
  temperature: { label: "Suhu Air", unit: "°C", color: "#ef4444" },
  ph: { label: "pH Level", unit: "pH", color: "#22c55e" },
  tds: { label: "TDS / Nutrisi", unit: "ppm", color: "#a855f7" },
  do: { label: "Oksigen Terlarut (DO)", unit: "mg/L", color: "#3b82f6" },
  turbidity: { label: "Kekeruhan (Turbidity)", unit: "NTU", color: "#f97316" },
};

export default function SettingsPage() {
  const { thresholds, setThresholds, sensorEnabled, toggleSensor } = useApp();
  const DEFAULT_THRESH = {
    temperature: { min: 20, max: 30 },
    ph: { min: 5.5, max: 8.0 },
    tds: { min: 400, max: 800 },
    do: { min: 4, max: 10 },
    turbidity: { min: 0.5, max: 5 },
  };
  const [localThresh, setLocalThresh] = useState(() =>
    JSON.parse(JSON.stringify(thresholds || DEFAULT_THRESH)),
  );
  const [saved, setSaved] = useState(false);
  const [saving, setSaving] = useState(false);

  const handleThreshChange = (key, bound, val) => {
    setLocalThresh((prev) => ({
      ...prev,
      [key]: { ...prev[key], [bound]: +val },
    }));
  };

  const handleSave = async () => {
    setSaving(true);
    await setThresholds(localThresh); // saves to Firestore
    setSaving(false);
    setSaved(true);
    setTimeout(() => setSaved(false), 2500);
  };

  return (
    <div className="settings-page">
      <div className="settings-header">
        <h1 className="page-title">System Settings</h1>
        <p className="page-subtitle">
          Configuration is automatically saved to Firebase Firestore
        </p>
      </div>

      {saved && (
        <div className="save-toast">
          <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2.5"
          >
            <polyline points="20 6 9 17 4 12" />
          </svg>
          Settings saved to Firestore
        </div>
      )}

      <div className="settings-grid">
        {/* Threshold */}
        <div className="settings-card wide">
          <div className="settings-card-header">
            <div className="sc-icon-wrap teal">
              <svg
                width="18"
                height="18"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
              >
                <path d="M18 20V10M12 20V4M6 20v-6" />
              </svg>
            </div>
            <div>
              <h2 className="sc-title">Thresholds Sensor</h2>
              <p className="sc-sub">
                Notifications are sent when values fall outside these ranges ·
                Saved to Firestore
              </p>
            </div>
          </div>
          <div className="threshold-list">
            {Object.entries(SENSOR_LABELS).map(([key, meta]) => (
              <div key={key} className="thresh-row">
                <div className="thresh-label-col">
                  <span
                    className="thresh-dot"
                    style={{ background: meta.color }}
                  />
                  <div>
                    <div className="thresh-name">{meta.label}</div>
                    <div className="thresh-unit">Unit: {meta.unit}</div>
                  </div>
                </div>
                <div className="thresh-inputs">
                  <div className="thresh-input-group">
                    <label>Min</label>
                    <div className="thresh-input-wrap">
                      <input
                        type="number"
                        step="0.1"
                        value={localThresh[key].min}
                        onChange={(e) =>
                          handleThreshChange(key, "min", e.target.value)
                        }
                      />
                      <span>{meta.unit}</span>
                    </div>
                  </div>
                  <div className="thresh-sep">—</div>
                  <div className="thresh-input-group">
                    <label>Max</label>
                    <div className="thresh-input-wrap">
                      <input
                        type="number"
                        step="0.1"
                        value={localThresh[key].max}
                        onChange={(e) =>
                          handleThreshChange(key, "max", e.target.value)
                        }
                      />
                      <span>{meta.unit}</span>
                    </div>
                  </div>
                </div>
                <div className="thresh-toggle-col">
                  <button
                    className={`small-toggle ${sensorEnabled[key] ? "on" : "off"}`}
                    onClick={() => toggleSensor(key)}
                  >
                    {sensorEnabled[key] ? "On" : "Off"}
                  </button>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Firebase Setup Guide */}
        <div className="settings-card">
          <div className="settings-card-header">
            <div className="sc-icon-wrap orange">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="#f97316">
                <path d="M3.89 15.67L6.6 3.5l5.3 9.19-8.01 2.98zm16.22-.01l-2.71-12.17-5.3 9.19 8.01 2.98zM12 21.5l-8.11-5.83 8.11-3.01 8.11 3.01L12 21.5z" />
              </svg>
            </div>
            <div>
              <h2 className="sc-title">Setup Firebase</h2>
              <p className="sc-sub">Firebase setup steps for this project</p>
            </div>
          </div>
          <div className="setup-steps">
            {[
              {
                n: "1",
                text: "Open console.firebase.google.com and create a project",
              },
              { n: "2", text: "Add a Web App → copy firebaseConfig" },
              { n: "3", text: "Paste config into src/firebase/config.js" },
              { n: "4", text: "Enable Firestore Database (test mode)" },
              { n: "5", text: "Enable Realtime Database" },
              {
                n: "6",
                text: "In Node-RED: install node-red-contrib-firebase-rtdb and connect to Firebase",
              },
              {
                n: "7",
                text: "Node-RED will push sensor data to RTDB → React app updates live",
              },
            ].map((s) => (
              <div key={s.n} className="setup-step">
                <span className="step-num">{s.n}</span>
                <span className="step-text">{s.text}</span>
              </div>
            ))}
          </div>
        </div>

        {/* Struktur Firestore */}
        <div className="settings-card">
          <div className="settings-card-header">
            <div className="sc-icon-wrap blue">
              <svg
                width="18"
                height="18"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
              >
                <ellipse cx="12" cy="5" rx="9" ry="3" />
                <path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3" />
                <path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5" />
              </svg>
            </div>
            <div>
              <h2 className="sc-title">Struktur Database</h2>
              <p className="sc-sub">Firestore + Realtime DB schema</p>
            </div>
          </div>
          <div className="db-schema">
            <div className="schema-section">
              <div className="schema-title">🔴 Realtime Database (live)</div>
              <pre className="schema-code">{`/sensors
  /temperature  { value, unit, status, updatedAt }
  /ph           { value, unit, status, updatedAt }
  /tds          { value, unit, status, updatedAt }
  /do           { value, unit, status, updatedAt }
  /turbidity    { value, unit, status, updatedAt }
/actuators
  /waterPump    { enabled, running, mode, ... }
  /aerator      { enabled, running, mode, ... }
  /heater       { enabled, running, mode, ... }
  /buzzer       { enabled, running, mode, ... }`}</pre>
            </div>
            <div className="schema-section">
              <div className="schema-title">🟡 Firestore (historis)</div>
              <pre className="schema-code">{`sensor_readings/
  { sensor, value, unit, status, timestamp }

actuator_logs/
  { device, action, source, timestamp }

settings/thresholds
  { temperature, ph, tds, do, turbidity }`}</pre>
            </div>
          </div>
        </div>

        {/* Node-RED → Firebase topics */}
        <div className="settings-card">
          <div className="settings-card-header">
            <div className="sc-icon-wrap teal">
              <svg
                width="18"
                height="18"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
              >
                <path d="M5 12.55a11 11 0 0 1 14.08 0" />
                <path d="M1.42 9a16 16 0 0 1 21.16 0" />
                <path d="M8.53 16.11a6 6 0 0 1 6.95 0" />
                <circle cx="12" cy="20" r="1" />
              </svg>
            </div>
            <div>
              <h2 className="sc-title">MQTT → Firebase Flow</h2>
              <p className="sc-sub">
                Alur data dari ESP ke Firebase via Node-RED
              </p>
            </div>
          </div>
          <div className="flow-diagram">
            {[
              {
                from: "ESP8266/ESP32",
                arrow: "→ MQTT →",
                to: "Mosquitto Broker",
              },
              { from: "Mosquitto Broker", arrow: "→ Sub →", to: "Node-RED" },
              { from: "Node-RED", arrow: "→ Push →", to: "Firebase RTDB" },
              {
                from: "Firebase RTDB",
                arrow: "→ onValue →",
                to: "React App (Live)",
              },
              {
                from: "Node-RED",
                arrow: "→ addDoc →",
                to: "Firestore (Historis)",
              },
            ].map((f, i) => (
              <div key={i} className="flow-row">
                <span className="flow-from">{f.from}</span>
                <span className="flow-arrow">{f.arrow}</span>
                <span className="flow-to">{f.to}</span>
              </div>
            ))}
          </div>
        </div>
      </div>

      <div className="settings-footer">
        <button
          className="btn-reset"
          onClick={() => setLocalThresh(JSON.parse(JSON.stringify(thresholds)))}
        >
          Reset
        </button>
        <button
          className="btn-save-settings"
          onClick={handleSave}
          disabled={saving}
        >
          {saving ? (
            <>
              <span className="spinner-sm-white" /> Saving…
            </>
          ) : (
            <>
              <svg
                width="14"
                height="14"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2.5"
              >
                <polyline points="20 6 9 17 4 12" />
              </svg>{" "}
              Save to Firestore
            </>
          )}
        </button>
      </div>
    </div>
  );
}
