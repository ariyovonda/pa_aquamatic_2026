import React, { useState, useEffect } from "react";
import { subHours, subDays, startOfDay, endOfDay } from "date-fns";
import { useApp } from "../context/AppContext";
import SensorCard from "../components/Dashboard/SensorCard";
import SensorChart from "../components/Dashboard/SensorChart";
import NotifBanner from "../components/Dashboard/NotifBanner";
import { getAllSensorsHistory } from "../firebase/firebaseService";
import { generateHistoryData } from "../utils/mockData";
import { historyMaxItems, prepareHistoryChart } from "../utils/historyChart";
import "./DashboardPage.css";

const SENSOR_COLORS = {
  temperature: "#ef4444",
  ph: "#22c55e",
  tds: "#a855f7",
  do: "#3b82f6",
  turbidity: "#f97316",
};

const SENSOR_META = {
  temperature: {
    label: "Temperature",
    fullLabel: "Water Temperature",
    icon: "thermometer",
    color: "#ef4444",
    bg: "#fee2e2",
    desc: "Pond water temperature",
  },
  ph: {
    label: "pH Level",
    fullLabel: "pH Level",
    icon: "ph",
    color: "#22c55e",
    bg: "#dcfce7",
    desc: "Water acidity level",
  },
  tds: {
    label: "TDS Level",
    fullLabel: "TDS Level",
    icon: "tds",
    color: "#a855f7",
    bg: "#f3e8ff",
    desc: "Nutrient concentration",
  },
  do: {
    label: "Dissolved O2",
    fullLabel: "Dissolved Oxygen",
    icon: "oxygen",
    color: "#3b82f6",
    bg: "#dbeafe",
    desc: "Oxygen level in water",
  },
  turbidity: {
    label: "Turbidity",
    fullLabel: "Turbidity",
    icon: "turbidity",
    color: "#f97316",
    bg: "#ffedd5",
    desc: "Water clarity",
  },
};

function getRangeDates(range) {
  const now = new Date();
  switch (range) {
    case "1H":
      return { from: subHours(now, 1), to: now };
    case "24H":
      return { from: startOfDay(now), to: endOfDay(now) };
    case "7D":
      return { from: startOfDay(subDays(now, 6)), to: endOfDay(now) };
    case "30D":
      return { from: startOfDay(subDays(now, 29)), to: endOfDay(now) };
    default:
      return { from: startOfDay(now), to: endOfDay(now) };
  }
}

export default function DashboardPage() {
  const { sensorData, firebaseConnected, selectedFarmId } = useApp();
  const [chartRange, setChartRange] = useState("24H");
  const [activeTab, setActiveTab] = useState("temperature");
  const [chartData, setChartData] = useState([]);
  const [loadingChart, setLoadingChart] = useState(false);
  const [usingMock, setUsingMock] = useState(false);

  // Fetch chart history from Firebase (or mock fallback)
  useEffect(() => {
    setLoadingChart(true);
    const { from, to } = getRangeDates(chartRange);
    getAllSensorsHistory(from, to, historyMaxItems(chartRange), selectedFarmId)
      .then((data) => {
        if (data.length === 0) throw new Error("empty");
        setChartData(prepareHistoryChart(data, chartRange, from));
        setUsingMock(false);
      })
      .catch(() => {
        console.warn("[Dashboard] No RTDB history found, using mock data");
        const mock = generateHistoryData(30);
        const cutoff = from.getTime();
        const filtered = mock.filter(
          (d) => new Date(d.timestamp).getTime() >= cutoff,
        );
        const step = Math.max(1, Math.floor(filtered.length / 120));
        setChartData(prepareHistoryChart(filtered.filter((_, i) => i % step === 0), chartRange, from));
        setUsingMock(true);
      })
      .finally(() => setLoadingChart(false));
  }, [chartRange, selectedFarmId]);

  const allNormal = Object.values(sensorData).every(
    (s) => s.status === "normal",
  );

  return (
    <div className="dashboard-page">
      <NotifBanner
        type={allNormal ? "success" : "warning"}
        title={allNormal ? "All Systems Optimal" : "Attention Required"}
        message={
          allNormal
            ? "All sensor readings are within optimal range"
            : "Some sensors are outside normal bounds. Check notification panel."
        }
      />

      {/* Firebase status badge */}
      <div className="db-status-bar">
        <span className={`db-badge ${firebaseConnected ? "live" : "sim"}`}>
          <span className="db-dot" />
          {firebaseConnected
            ? "Firebase Realtime DB – Live"
            : "Simulation Mode (Firebase not configured)"}
        </span>
      </div>

      {/* Sensor Cards */}
      <div className="sensor-grid">
        {Object.entries(sensorData).map(([key, sensor]) => (
          <SensorCard
            key={key}
            sensorKey={key}
            sensor={sensor}
            meta={SENSOR_META[key]}
          />
        ))}
      </div>

      {/* Chart */}
      <div className="chart-section">
        <div className="chart-header">
          <h2 className="chart-title">Sensor Readings</h2>
          <div className="chart-header-controls">
            <div className="chart-range-tabs">
              {["1H", "24H", "7D", "30D"].map((r) => (
                <button
                  key={r}
                  className={`range-tab ${chartRange === r ? "active" : ""}`}
                  onClick={() => setChartRange(r)}
                >
                  {r}
                </button>
              ))}
            </div>
            {usingMock && (
              <span className="data-source-badge mock">
                <span className="badge-dot" />
                Mock Data
              </span>
            )}
            {!usingMock && chartData.length > 0 && (
              <span className="data-source-badge live">
                <span className="badge-dot" />
                RTDB History
              </span>
            )}
          </div>
        </div>

        <div className="sensor-tabs">
          {Object.keys(SENSOR_META).map((key) => (
            <button
              key={key}
              className={`sensor-tab ${activeTab === key ? "active" : ""}`}
              style={
                activeTab === key
                  ? {
                      color: SENSOR_COLORS[key],
                      borderColor: SENSOR_COLORS[key],
                    }
                  : {}
              }
              onClick={() => setActiveTab(key)}
            >
              {SENSOR_META[key].label}
            </button>
          ))}
          <button
            className={`sensor-tab ${activeTab === "all" ? "active" : ""}`}
            style={
              activeTab === "all"
                ? { color: "#0d9488", borderColor: "#0d9488" }
                : {}
            }
            onClick={() => setActiveTab("all")}
          >
            All
          </button>
        </div>

        {loadingChart ? (
          <div className="chart-loading-inline">
            <span className="spinner-sm" />
            <span>Loading data from Firebase…</span>
          </div>
        ) : (
          <SensorChart
            data={chartData}
            activeTab={activeTab}
            colors={SENSOR_COLORS}
            meta={SENSOR_META}
            range={chartRange}
            rangeStart={getRangeDates(chartRange).from}
            rangeEnd={getRangeDates(chartRange).to}
          />
        )}
      </div>
    </div>
  );
}
