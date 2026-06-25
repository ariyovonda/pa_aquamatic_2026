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

export default function SensorCard({ sensorKey, sensor, meta }) {
  const isOffline = sensor.status === "offline" || sensor.connected === false;
  const isWarning = sensor.status === "warning";
  const displayValue = isOffline ? "-" : sensor.value;

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
      className={`sensor-card ${isWarning ? "warning" : ""} ${isOffline ? "offline" : ""}`}
    >
      <div className="sc-header">
        <div className="sc-icon" style={{ background: meta.bg }}>
          {ICONS[meta.icon]?.(meta.color)}
        </div>
        <div className="sc-title-block">
          <span className="sc-title">{meta.fullLabel}</span>
          <div className="sc-status-dot-wrap">
            <span
              className={`sc-dot ${isOffline ? "offline" : isWarning ? "warn" : "ok"}`}
              style={{ backgroundColor: dotColor }}
            />
          </div>
        </div>
        <span className={`sensor-live-badge ${isOffline ? "offline" : "on"}`}>
          {isOffline ? "OFFLINE" : "LIVE"}
        </span>
      </div>

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
        <div className="sc-desc">
          {meta.desc}
        </div>
      )}
    </div>
  );
}
