import React from "react";
import "./SensorCard.css";

const ICONS = {
  thermometer: (color) => (
    <svg
      width="22"
      height="22"
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z" />
    </svg>
  ),
  ph: (color) => (
    <svg
      width="22"
      height="22"
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M12 2a10 10 0 1 0 10 10A10 10 0 0 0 12 2z" />
      <path d="M12 8v8M8 12h8" />
    </svg>
  ),
  tds: (color) => (
    <svg
      width="22"
      height="22"
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M9 3H5a2 2 0 0 0-2 2v4m6-6h10a2 2 0 0 1 2 2v4M9 3v18m0 0h10a2 2 0 0 0 2-2V9M9 21H5a2 2 0 0 1-2-2V9m0 0h18" />
    </svg>
  ),
  oxygen: (color) => (
    <svg
      width="22"
      height="22"
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M5 12h14M12 5l7 7-7 7" />
    </svg>
  ),
  turbidity: (color) => (
    <svg
      width="22"
      height="22"
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <circle cx="12" cy="12" r="10" />
      <path d="M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20" />
      <path d="M2 12h20" />
    </svg>
  ),
};

export default function SensorCard({
  sensorKey,
  sensor,
  meta,
  enabled,
  onToggle,
}) {
  const isOffline = sensor.status === "offline" || sensor.connected === false;
  const isWarning = sensor.status === "warning";
  const displayValue = isOffline ? "-" : sensor.value;
  const pct = isOffline
    ? 0
    : Math.min(
        100,
        Math.max(
          0,
          ((sensor.value - sensor.min) / (sensor.max - sensor.min)) * 100,
        ),
      );

  // Dot and number colors: use gray for offline, red for warnings, otherwise sensor color
  const offlineColor = "#9CA3AF"; // gray
  const warnColor = "#ef4444"; // red
  const dotColor = isOffline
    ? offlineColor
    : isWarning
      ? warnColor
      : meta.color;

  return (
    <div
      className={`sensor-card ${isWarning ? "warning" : ""} ${isOffline ? "offline" : ""} ${!enabled ? "disabled" : ""}`}
    >
      <div className="sc-header">
        <div className="sc-icon" style={{ background: meta.bg }}>
          {ICONS[meta.icon]?.(meta.color)}
        </div>
        <div className="sc-title-block">
          <span className="sc-title">{meta.fullLabel}</span>
          <div className="sc-status-dot-wrap">
            <span
              className={`sc-dot ${isOffline ? "offline" : isWarning ? "warn" : "ok"} ${enabled ? "" : "off"}`}
              style={{ backgroundColor: dotColor }}
            />
          </div>
        </div>
        <button
          className={`sc-arrow ${!enabled ? "off" : ""}`}
          onClick={onToggle}
          title={enabled ? "Disable sensor" : "Enable sensor"}
        >
          <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            {enabled ? (
              <>
                <line x1="5" y1="12" x2="19" y2="12" />
                <polyline points="12 5 19 12 12 19" />
              </>
            ) : (
              <>
                <line x1="18" y1="6" x2="6" y2="18" />
                <line x1="6" y1="6" x2="18" y2="18" />
              </>
            )}
          </svg>
        </button>
      </div>

      {enabled ? (
        <>
          <div className="sc-value">
            <span
              className="sc-num"
              style={{
                color: isOffline
                  ? offlineColor
                  : isWarning
                    ? warnColor
                    : meta.color,
              }}
            >
              {displayValue}
            </span>
            {!isOffline && <span className="sc-unit">{sensor.unit}</span>}
          </div>

          {!isOffline && (
            <>
              <div className="sc-bar-wrap">
                <div className="sc-bar-track">
                  <div
                    className="sc-bar-fill"
                    style={{
                      width: `${pct}%`,
                      background: isWarning ? "#ef4444" : meta.color,
                    }}
                  />
                </div>
                <div className="sc-bar-labels">
                  <span>{sensor.min}</span>
                  <span>{sensor.max}</span>
                </div>
              </div>
              <div className="sc-desc">{meta.desc}</div>
            </>
          )}
        </>
      ) : (
        <div className="sc-disabled-msg">
          <span>Sensor disabled</span>
        </div>
      )}
    </div>
  );
}
