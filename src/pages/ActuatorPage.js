import React, { useState } from 'react';
import { useApp } from '../context/AppContext';
import { useMqtt } from '../context/MqttContext';
import './ActuatorPage.css';

const ACTUATOR_ICONS = {
  waterPump: (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
      <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2z" />
      <path d="M12 6v6l4 2" />
    </svg>
  ),
  aerator: (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
      <path d="M5 12h14M12 5l7 7-7 7" />
      <path d="M5 5l7 7-7 7" />
    </svg>
  ),
  heater: (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
      <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z" />
    </svg>
  ),
  buzzer: (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
      <path d="M4 11h4l6-6v14l-6-6H4z" />
      <path d="M20 8c1.5 1.5 1.5 5 0 6.5" />
    </svg>
  ),
};

const ACTUATOR_COLORS = {
  waterPump: '#3b82f6',
  aerator: '#22c55e',
  heater: '#ef4444',
  buzzer: '#f97316',
};

const ACTUATOR_BG = {
  waterPump: '#dbeafe',
  aerator: '#dcfce7',
  heater: '#fee2e2',
  buzzer: '#fff7ed',
};

export default function ActuatorPage() {
  const { actuators, toggleActuator, updateActuator } = useApp();
  const { publish } = useMqtt();
  const [editing, setEditing] = useState(null);

  const handleToggle = (id) => {
    toggleActuator(id);
    publish(`actuators/${id}/command`, { action: actuators[id].enabled ? 'disable' : 'enable' });
  };

  const handleRunToggle = (id) => {
    const act = actuators[id];
    if (!act.enabled) return;
    const newRunning = !act.running;
    updateActuator(id, { running: newRunning, lastRun: new Date().toISOString() });
    publish(`actuators/${id}/command`, { action: newRunning ? 'on' : 'off' });
  };

  const handleSaveInterval = (id, vals) => {
    updateActuator(id, vals);
    publish(`actuators/${id}/config`, vals);
    setEditing(null);
  };

  return (
    <div className="actuator-page">
      <div className="act-page-header">
        <div>
          <h1 className="page-title">Actuator Control</h1>
          <p className="page-subtitle">Manage pump, aerator, heater and buzzer for the aquaponics system</p>
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
            onRunToggle={() => handleRunToggle(id)}
            editing={editing === id}
            onEdit={() => setEditing(id)}
            onSave={(vals) => handleSaveInterval(id, vals)}
            onCancelEdit={() => setEditing(null)}
          />
        ))}
      </div>

      {/* Info panel */}
      <div className="act-info-panel">
        <div className="info-panel-icon">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#0d9488" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
            <circle cx="12" cy="12" r="10" />
            <line x1="12" y1="8" x2="12" y2="12" />
            <line x1="12" y1="16" x2="12.01" y2="16" />
          </svg>
        </div>
        <div>
          <div className="info-panel-title">About Actuator Control</div>
          <div className="info-panel-body">
            Control commands are sent via MQTT to Node-RED and forwarded to the ESP/Arduino.
            <b>Continuous</b> mode: actuator runs continuously. <b>Interval</b> mode: actuator runs on a schedule.
            <b>Auto</b> mode: actuator is controlled automatically based on sensor values (e.g. heater on when temperature below threshold).
          </div>
        </div>
      </div>
    </div>
  );
}

function ActuatorCard({ id, act, color, bg, icon, onToggle, onRunToggle, editing, onEdit, onSave, onCancelEdit }) {
  const [localInterval, setLocalInterval] = useState(act.intervalMinutes);
  const [localDuration, setLocalDuration] = useState(act.durationMinutes);
  const [localMode, setLocalMode] = useState(act.mode);

  const lastRunStr = act.lastRun
    ? new Date(act.lastRun).toLocaleString('en-US', { dateStyle: 'short', timeStyle: 'short' })
    : '—';

  return (
    <div className={`act-card ${!act.enabled ? 'disabled' : ''} ${act.running && act.enabled ? 'running' : ''}`}>
      {/* Header */}
      <div className="act-card-header">
        <div className="act-icon" style={{ background: bg, color }}>
          {icon}
        </div>
        <div className="act-info">
          <span className="act-label">{act.label}</span>
          <div className="act-status-row">
            {act.enabled ? (
              act.running
                ? <span className="act-badge running">Running</span>
                : <span className="act-badge standby">Standby</span>
            ) : (
              <span className="act-badge off">Off</span>
            )}
            <span className="act-mode-badge">{act.mode === 'continuous' ? 'Continuous' : act.mode === 'interval' ? 'Interval' : 'Auto'}</span>
          </div>
        </div>
        {/* Enable toggle */}
        <button
          className={`toggle-switch ${act.enabled ? 'on' : 'off'}`}
          style={act.enabled ? { background: color } : {}}
          onClick={onToggle}
          title={act.enabled ? 'Disable' : 'Enable'}
        >
          <span className="toggle-thumb" />
        </button>
      </div>

      {/* Stats row */}
      <div className="act-stats">
        <div className="act-stat">
          <span className="stat-label">Interval</span>
          <span className="stat-val">{act.mode === 'continuous' ? '—' : `${act.intervalMinutes} min`}</span>
        </div>
        <div className="act-stat">
          <span className="stat-label">Duration</span>
          <span className="stat-val">{act.mode === 'continuous' ? 'Continuous' : `${act.durationMinutes} min`}</span>
        </div>
        <div className="act-stat">
          <span className="stat-label">Last Active</span>
          <span className="stat-val">{lastRunStr}</span>
        </div>
      </div>

      {/* Edit panel */}
      {editing ? (
        <div className="act-edit-panel">
          <div className="edit-row">
            <label>Mode</label>
            <select value={localMode} onChange={e => setLocalMode(e.target.value)}>
              <option value="continuous">Continuous</option>
              <option value="interval">Interval</option>
              <option value="auto">Auto</option>
            </select>
          </div>
          {localMode === 'interval' && (
            <>
              <div className="edit-row">
                <label>Interval (min)</label>
                <div className="edit-input-wrap">
                  <input
                    type="number"
                    min={1} max={999}
                    value={localInterval}
                    onChange={e => setLocalInterval(+e.target.value)}
                  />
                  <span className="unit">min</span>
                </div>
              </div>
              <div className="edit-row">
                <label>Active duration (min)</label>
                <div className="edit-input-wrap">
                  <input
                    type="number"
                    min={1} max={localInterval}
                    value={localDuration}
                    onChange={e => setLocalDuration(+e.target.value)}
                  />
                  <span className="unit">min</span>
                </div>
              </div>
            </>
          )}
          <div className="edit-actions">
            <button className="btn-cancel" onClick={onCancelEdit}>Cancel</button>
            <button className="btn-save" style={{ background: color }} onClick={() => onSave({ mode: localMode, intervalMinutes: localInterval, durationMinutes: localDuration })}>Save</button>
          </div>
        </div>
      ) : (
        <div className="act-footer">
          <button
            className={`btn-run ${act.running ? 'stop' : 'start'}`}
            disabled={!act.enabled}
            style={act.enabled && !act.running ? { background: color, borderColor: color } : {}}
            onClick={onRunToggle}
          >
            {act.running ? (
              <><svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="6" width="12" height="12" /></svg> Stop</>
            ) : (
              <><svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3" /></svg> Run</>
            )}
          </button>
          <button className="btn-edit" onClick={onEdit}>
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
              <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
              <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
            </svg>
            Schedule
          </button>
        </div>
      )}
    </div>
  );
}
