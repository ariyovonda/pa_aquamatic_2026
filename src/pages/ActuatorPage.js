import React, { useState } from "react";
import { useApp } from "../context/AppContext";
import "./ActuatorPage.css";

const ACTUATOR_ICONS = {
  waterPump: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2z" />
      <path d="M12 6v6l4 2" />
    </svg>
  ),
  aerator: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M5 12h14M12 5l7 7-7 7" />
      <path d="M5 5l7 7-7 7" />
    </svg>
  ),
  heater: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z" />
    </svg>
  ),
  buzzer: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M4 11h4l6-6v14l-6-6H4z" />
      <path d="M20 8c1.5 1.5 1.5 5 0 6.5" />
    </svg>
  ),
  phPumpDown: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M6 4h12v16H6z" />
      <path d="M8 8h8M8 12h8M8 16h8" />
      <path d="M12 20v-8" />
      <path d="M10 18l2 2 2-2" />
    </svg>
  ),
  phPumpUp: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M6 4h12v16H6z" />
      <path d="M8 8h8M8 12h8M8 16h8" />
      <path d="M12 4v8" />
      <path d="M10 6l2-2 2 2" />
    </svg>
  ),
  nutritionPump: (
    <svg
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M5 5h14v14H5z" />
      <path d="M8 12h8M12 8v8" />
      <path d="M9 4l2 4 2-4" />
    </svg>
  ),
};

const ACTUATOR_COLORS = {
  waterPump: "#3b82f6",
  aerator: "#22c55e",
  heater: "#ef4444",
  buzzer: "#f97316",
  phPumpDown: "#0f766e",
  phPumpUp: "#a855f7",
  nutritionPump: "#14b8a6",
};

const ACTUATOR_BG = {
  waterPump: "#dbeafe",
  aerator: "#dcfce7",
  heater: "#fee2e2",
  buzzer: "#fff7ed",
  phPumpDown: "#d1fae5",
  phPumpUp: "#ede9fe",
  nutritionPump: "#cffafe",
};

const ACTUATOR_RUNTIME_LABEL = {
  waterPump: { on: "Cycling", off: "Idle" },
  aerator: { on: "Cycling", off: "Idle" },
  heater: { on: "Pulsing", off: "Idle" },
  buzzer: { on: "Pulsing", off: "Idle" },
  phPumpDown: { on: "Dosing", off: "Idle" },
  phPumpUp: { on: "Dosing", off: "Idle" },
  nutritionPump: { on: "Dosing", off: "Idle" },
};

export default function ActuatorPage() {
  const { actuators, toggleActuator, updateActuatorAutomation } = useApp();

  const handleToggle = (id) => {
    toggleActuator(id);
  };

  const handleSaveAutomation = (id, changes) => {
    updateActuatorAutomation(id, changes);
  };

  return (
    <div className="actuator-page">
      <div className="act-page-header">
        <div>
          <h1 className="page-title">Actuator Control</h1>
          <p className="page-subtitle">
            Manage pond pumps, pH pumps, nutrition pump, aerator, heater, and
            buzzer in the aquaponics system
          </p>
        </div>
      </div>

      <div className="act-grid">
        {Object.entries(actuators).map(([id, act]) => (
          <ActuatorCard
            key={id}
            id={id}
            act={act}
            color={ACTUATOR_COLORS[id]}
            bg={ACTUATOR_BG[id]}
            icon={ACTUATOR_ICONS[id]}
            onToggle={() => handleToggle(id)}
            onSaveAutomation={(changes) => handleSaveAutomation(id, changes)}
          />
        ))}
      </div>

      {/* Info panel */}
      <div className="act-info-panel">
        <div className="info-panel-icon">
          <svg
            width="18"
            height="18"
            viewBox="0 0 24 24"
            fill="none"
            stroke="#0d9488"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <circle cx="12" cy="12" r="10" />
            <line x1="12" y1="8" x2="12" y2="12" />
            <line x1="12" y1="16" x2="12.01" y2="16" />
          </svg>
        </div>
        <div>
          <div className="info-panel-title">About Actuator Control</div>
          <div className="info-panel-body">
            Sistem memakai mode pulse/cycle agar relay dan aktuator tidak ON
            permanen. Threshold disimpan ke RTDB, dibaca Node-RED, lalu Arduino
            menjalankan aktuator sebentar sesuai durasi aman firmware.
          </div>
        </div>
      </div>
    </div>
  );
}

const SENSOR_OPTIONS = {
  temperature: { label: "Suhu", unit: "C" },
  ph: { label: "pH", unit: "pH" },
  tds: { label: "TDS", unit: "ppm" },
  do: { label: "DO", unit: "mg/L" },
  turbidity: { label: "Turbidity", unit: "NTU" },
};

// Batas fisik tiap aktuator. Jangan menawarkan sensor yang tidak punya
// hubungan dengan aktuatornya karena itu menghasilkan aturan auto yang salah.
const ACTUATOR_CAPABILITIES = {
  waterPump: { automation: false, manualLabel: "Cycle ON/OFF" },
  aerator: { automation: false, manualLabel: "Cycle ON/OFF" },
  heater: { automation: true, sensors: ["temperature"], condition: "below" },
  buzzer: { automation: true, sensors: ["tds", "turbidity"], condition: "above" },
  phPumpDown: { automation: true, sensors: ["ph"], condition: "above" },
  phPumpUp: { automation: true, sensors: ["ph"], condition: "below" },
  nutritionPump: { automation: true, sensors: ["tds"], condition: "below" },
};

function ActuatorCard({ id, act, color, bg, icon, onToggle, onSaveAutomation }) {
  const capability = ACTUATOR_CAPABILITIES[id] || { automation: false };
  const displayMode = capability.automation ? act.mode || "manual" : "manual";
  const runtimeText = act.running
    ? ACTUATOR_RUNTIME_LABEL[id]?.on || "Active"
    : ACTUATOR_RUNTIME_LABEL[id]?.off || "Idle";
  const supportedSensors = capability.sensors || [];
  const normalizeDraft = (source) => {
    const fallbackSensor = supportedSensors[0] || "temperature";
    const automation = source.automation || {};
    return {
      mode: capability.automation ? source.mode || "manual" : "manual",
      automation: {
        sensor: supportedSensors.includes(automation.sensor)
          ? automation.sensor
          : fallbackSensor,
        condition: capability.condition || automation.condition || "below",
        value: Number(automation.value) || 0,
        hysteresis: Number(automation.hysteresis) || 0,
      },
    };
  };
  const [editing, setEditing] = useState(false);
  const [draft, setDraft] = useState(() => normalizeDraft(act));
  const lastRunStr = act.lastRun
    ? new Date(act.lastRun).toLocaleString("en-US", {
        dateStyle: "short",
        timeStyle: "short",
      })
    : "—";

  return (
    <div className={`act-card ${!act.enabled ? "disabled" : ""} ${act.running ? "running" : ""}`}>
      {/* Header */}
      <div className="act-card-header">
        <div className="act-icon" style={{ background: bg, color }}>
          {icon}
        </div>
        <div className="act-info">
          <span className="act-label">{act.label}</span>
          <div className="act-status-row">
            {act.enabled ? (
              <span className="act-badge enabled">Enabled</span>
            ) : (
              <span className="act-badge off">Disabled</span>
            )}
            <span className={`act-badge ${act.running ? "enabled" : "off"}`}>
              {runtimeText}
            </span>
            <span className="act-mode-badge">
              {capability.automation
                ? displayMode
                : capability.manualLabel || "manual"}
            </span>
          </div>
        </div>
        {/* Enable toggle */}
        <button
          className={`toggle-switch ${act.enabled ? "on" : "off"}`}
          style={act.enabled ? { background: color } : {}}
          onClick={onToggle}
          title={act.enabled ? "Disable" : "Enable"}
        >
          <span className="toggle-thumb" />
        </button>
      </div>

      {/* Stats row */}
      <div className="act-stats">
        <div className="act-stat">
          <span className="stat-label">Last Active</span>
          <span className="stat-val">{lastRunStr}</span>
        </div>
        <div className="act-stat">
          <span className="stat-label">Auto Rule</span>
          <span className="stat-val">
            {displayMode === "auto" && act.automation
              ? `${SENSOR_OPTIONS[act.automation.sensor]?.label || act.automation.sensor} ${act.automation.condition === "below" ? "<" : ">"} ${act.automation.value}`
              : !capability.automation ? capability.manualLabel || "Cycle ON/OFF" : "Manual"}
          </span>
        </div>
      </div>

      {editing && capability.automation && (
        <div className="act-edit-panel">
          <div className="edit-row">
            <label>Mode</label>
            <select
              value={draft.mode}
              onChange={(e) => setDraft((prev) => ({ ...prev, mode: e.target.value }))}
            >
              <option value="manual">Manual</option>
              <option value="auto">Auto</option>
            </select>
          </div>
          {draft.mode === "auto" && (
            <>
              <div className="edit-row">
                <label>Sensor</label>
                <select
                  value={draft.automation.sensor}
                  onChange={(e) => setDraft((prev) => ({
                    ...prev,
                    automation: { ...prev.automation, sensor: e.target.value },
                  }))}
                >
                  {supportedSensors.map((key) => (
                    <option key={key} value={key}>{SENSOR_OPTIONS[key].label}</option>
                  ))}
                </select>
              </div>
              <div className="edit-row">
                <label>Kondisi</label>
                <span>{draft.automation.condition === "below" ? "Di bawah threshold" : "Di atas threshold"}</span>
              </div>
              <div className="edit-row">
                <label>Threshold</label>
                <div className="edit-input-wrap">
                  <input
                    type="number"
                    step="0.1"
                    value={draft.automation.value}
                    onChange={(e) => setDraft((prev) => ({
                      ...prev,
                      automation: { ...prev.automation, value: Number(e.target.value) },
                    }))}
                  />
                  <span className="unit">{SENSOR_OPTIONS[draft.automation.sensor]?.unit}</span>
                </div>
              </div>
              <div className="edit-row">
                <label>Hysteresis</label>
                <div className="edit-input-wrap">
                  <input
                    type="number"
                    min="0"
                    step="0.1"
                    value={draft.automation.hysteresis}
                    onChange={(e) => setDraft((prev) => ({
                      ...prev,
                      automation: { ...prev.automation, hysteresis: Number(e.target.value) },
                    }))}
                  />
                  <span className="unit">{SENSOR_OPTIONS[draft.automation.sensor]?.unit}</span>
                </div>
              </div>
            </>
          )}
          <div className="edit-actions">
            <button className="btn-cancel" onClick={() => setEditing(false)}>Batal</button>
            <button
              className="btn-save"
              style={{ background: color }}
              onClick={() => {
                onSaveAutomation(normalizeDraft(draft));
                setEditing(false);
              }}
            >
              Simpan
            </button>
          </div>
        </div>
      )}

      {!editing && capability.automation && (
        <div className="act-footer">
          <button className="btn-edit" onClick={() => {
            setDraft(normalizeDraft(act));
            setEditing(true);
          }}>
            Atur threshold
          </button>
        </div>
      )}
    </div>
  );
}
