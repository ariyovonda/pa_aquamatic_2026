import React, { useState, useEffect, useMemo } from 'react';
import { subHours, subDays, format } from 'date-fns';
import { enUS } from 'date-fns/locale';
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, Legend, ResponsiveContainer
} from 'recharts';
import {
  getDailySummary,
  getHourlyData,
  getAllSensorsHistory,
} from '../firebase/firebaseService';
import { generateHistoryData } from '../utils/mockData';
import './HistoryPage.css';

const SENSOR_COLORS = {
  temperature: '#ef4444',
  ph: '#22c55e',
  tds: '#a855f7',
  do: '#3b82f6',
  turbidity: '#f97316',
};

const SENSOR_LABELS = {
  temperature: 'Temperature (°C)',
  ph: 'pH',
  tds: 'TDS (ppm)',
  do: 'DO (mg/L)',
  turbidity: 'Turbidity (NTU)',
};

const RANGES = [
  { label: '1 Hour', value: '1H' },
  { label: '24 Hours', value: '24H' },
  { label: '7 Days', value: '7D' },
  { label: '30 Days', value: '30D' },
];

function getRangeDates(range) {
  const now = new Date();
  switch (range) {
    case '1H': return { from: subHours(now, 1), to: now };
    case '24H': return { from: subHours(now, 24), to: now };
    case '7D': return { from: subDays(now, 7), to: now };
    case '30D': return { from: subDays(now, 30), to: now };
    default: return { from: subHours(now, 24), to: now };
  }
}

export default function HistoryPage() {
  const [range, setRange] = useState('7D');
  const [activeKeys, setActiveKeys] = useState(Object.keys(SENSOR_COLORS));
  const [selectedDay, setSelectedDay] = useState(null);
  const [chartData, setChartData] = useState([]);
  const [dailyAgg, setDailyAgg] = useState([]);
  const [hourlyData, setHourlyData] = useState([]);
  const [loading, setLoading] = useState(false);
  const [loadingHour, setLoadingHour] = useState(false);
  const [usingMock, setUsingMock] = useState(false);

  // Load chart data when range changes
  useEffect(() => {
    setLoading(true);
    const { from, to } = getRangeDates(range);
    getAllSensorsHistory(from, to, 600)
      .then(data => {
        if (data.length === 0) throw new Error('empty');
        setChartData(data);
        setUsingMock(false);
      })
      .catch(() => {
        // Fallback to mock data
        const mock = generateHistoryData(30);
        const cutoff = from.getTime();
        const filtered = mock.filter(d => new Date(d.timestamp).getTime() >= cutoff);
        const step = Math.max(1, Math.floor(filtered.length / 150));
        setChartData(filtered.filter((_, i) => i % step === 0));
        setUsingMock(true);
      })
      .finally(() => setLoading(false));
  }, [range]);

  // Load daily summary
  useEffect(() => {
    getDailySummary(30)
      .then(data => {
        if (data.length === 0) throw new Error('empty');
        setDailyAgg(data);
      })
      .catch(() => {
        // Fallback mock daily
        const mock = generateHistoryData(30);
        const groups = {};
        mock.forEach(d => {
          const day = d.timestamp.split('T')[0];
          if (!groups[day]) groups[day] = { temperature: [], ph: [], tds: [], do: [], turbidity: [] };
          ['temperature', 'ph', 'tds', 'do', 'turbidity'].forEach(k => { if (d[k]) groups[day][k].push(d[k]); });
        });
        const avg = arr => arr.length ? +(arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(2) : null;
        const rows = Object.entries(groups).map(([date, vals]) => ({
          date,
          temperature: avg(vals.temperature),
          ph: avg(vals.ph),
          tds: avg(vals.tds),
          do: avg(vals.do),
          turbidity: avg(vals.turbidity),
        }));
        setDailyAgg(rows.sort((a, b) => a.date.localeCompare(b.date)));
      });
  }, []);

  // Load hourly data when a day is selected
  useEffect(() => {
    if (!selectedDay) return;
    setLoadingHour(true);
    getHourlyData(selectedDay)
      .then(data => setHourlyData(data))
      .catch(() => {
        const mock = generateHistoryData(30);
        const dayData = mock.filter(d => d.timestamp.startsWith(selectedDay));
        const groups = {};
        dayData.forEach(d => {
          const h = new Date(d.timestamp).getHours();
          if (!groups[h]) groups[h] = { temperature: [], ph: [], tds: [], do: [], turbidity: [] };
          ['temperature', 'ph', 'tds', 'do', 'turbidity'].forEach(k => { if (d[k]) groups[h][k].push(d[k]); });
        });
        const avg = arr => arr.length ? +(arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(2) : null;
        setHourlyData(Array.from({ length: 24 }, (_, h) => ({
          hour: `${String(h).padStart(2, '0')}:00`,
          temperature: avg((groups[h] || {}).temperature || []),
          ph: avg((groups[h] || {}).ph || []),
          tds: avg((groups[h] || {}).tds || []),
          do: avg((groups[h] || {}).do || []),
          turbidity: avg((groups[h] || {}).turbidity || []),
        })));
      })
      .finally(() => setLoadingHour(false));
  }, [selectedDay]);

  const formattedChart = useMemo(() =>
    chartData.map(d => ({ ...d, ts: new Date(d.timestamp).getTime() })),
    [chartData]
  );

  const toggleKey = (key) => {
    setActiveKeys(prev =>
      prev.includes(key)
        ? prev.length > 1 ? prev.filter(k => k !== key) : prev
        : [...prev, key]
    );
  };

  const formatX = (ts) => {
    const d = new Date(ts);
    if (range === '1H' || range === '24H') return format(d, 'HH:mm');
    if (range === '7D') return format(d, 'EEE', { locale: enUS });
    return format(d, 'dd/MM');
  };

  return (
    <div className="history-page">
      <div className="history-header">
        <div>
          <h1 className="page-title">History</h1>
          <p className="page-subtitle">
            Historical data is stored in Firebase Firestore
            {usingMock && <span className="mock-badge"> · Simulation Mode</span>}
          </p>
        </div>
      </div>

      {/* Range selector */}
      <div className="range-bar">
        {RANGES.map(r => (
          <button
            key={r.value}
            className={`range-btn ${range === r.value ? 'active' : ''}`}
            onClick={() => setRange(r.value)}
          >{r.label}</button>
        ))}
      </div>

      {/* Key toggles */}
      <div className="key-toggles">
        {Object.keys(SENSOR_LABELS).map(key => (
          <button
            key={key}
            className={`key-toggle ${activeKeys.includes(key) ? 'on' : 'off'}`}
            style={activeKeys.includes(key) ? { borderColor: SENSOR_COLORS[key], color: SENSOR_COLORS[key] } : {}}
            onClick={() => toggleKey(key)}
          >
            <span className="kt-dot" style={{ background: SENSOR_COLORS[key] }} />
            {SENSOR_LABELS[key]}
          </button>
        ))}
      </div>

      {/* Main chart */}
      <div className="chart-card">
        <h3 className="card-label">
          Sensor Trends – {RANGES.find(r => r.value === range)?.label}
        </h3>
        {loading ? (
          <div className="chart-loading">
            <span className="spinner" />
            <span>Loading data from Firebase…</span>
          </div>
        ) : (
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={formattedChart} margin={{ top: 5, right: 10, left: -10, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
              <XAxis
                dataKey="ts"
                tickFormatter={formatX}
                tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }}
                axisLine={false} tickLine={false}
                interval="preserveStartEnd" minTickGap={50}
              />
              <YAxis tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }} axisLine={false} tickLine={false} width={38} />
              <Tooltip
                contentStyle={{ fontSize: 12, borderRadius: 8, border: '1px solid #e2e8f0', boxShadow: '0 4px 16px rgba(0,0,0,0.08)' }}
                labelFormatter={v => new Date(v).toLocaleString('en-US')}
              />
              {activeKeys.map(key => (
                <Line key={key} type="monotone" dataKey={key} stroke={SENSOR_COLORS[key]}
                  strokeWidth={1.8} dot={false} activeDot={{ r: 4, strokeWidth: 0 }} connectNulls />
              ))}
            </LineChart>
          </ResponsiveContainer>
        )}
      </div>

      {/* Daily summary table */}
      <div className="chart-card">
        <div className="card-header-row">
          <h3 className="card-label">Daily Summary – Firebase Firestore</h3>
          <span className="card-sub">Click a row to view hourly details</span>
        </div>
        <div className="table-wrap">
          <table className="data-table">
            <thead>
              <tr>
                <th>Date</th>
                <th>Temperature (°C)</th>
                <th>pH</th>
                <th>TDS (ppm)</th>
                <th>DO (mg/L)</th>
                <th>Turbidity (NTU)</th>
              </tr>
            </thead>
            <tbody>
              {dailyAgg.slice(-30).reverse().map(row => (
                <tr
                  key={row.date}
                  className={`data-row ${selectedDay === row.date ? 'selected' : ''}`}
                  onClick={() => setSelectedDay(p => p === row.date ? null : row.date)}
                >
                  <td className="date-cell">
                    {format(new Date(row.date), 'EEE, dd MMM yyyy', { locale: enUS })}
                  </td>
                  <td><span className="val-pill temp">{row.temperature ?? '—'}</span></td>
                  <td><span className="val-pill ph">{row.ph ?? '—'}</span></td>
                  <td><span className="val-pill tds">{row.tds ?? '—'}</span></td>
                  <td><span className="val-pill do">{row.do ?? '—'}</span></td>
                  <td><span className="val-pill turb">{row.turbidity ?? '—'}</span></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Hourly detail */}
      {selectedDay && (
        <div className="chart-card hourly-detail">
          <h3 className="card-label">
            Hourly Details – {format(new Date(selectedDay), 'EEEE, dd MMMM yyyy', { locale: enUS })}
          </h3>
          {loadingHour ? (
            <div className="chart-loading"><span className="spinner" /><span>Loading…</span></div>
          ) : (
            <ResponsiveContainer width="100%" height={260}>
              <LineChart data={hourlyData} margin={{ top: 5, right: 10, left: -10, bottom: 5 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
                <XAxis dataKey="hour" tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }} axisLine={false} tickLine={false} width={38} />
                <Tooltip contentStyle={{ fontSize: 12, borderRadius: 8, border: '1px solid #e2e8f0' }} />
                <Legend wrapperStyle={{ fontSize: '11px' }} formatter={v => SENSOR_LABELS[v] || v} />
                {Object.keys(SENSOR_COLORS).map(key => (
                  <Line key={key} type="monotone" dataKey={key} stroke={SENSOR_COLORS[key]}
                    strokeWidth={1.8} dot={false} activeDot={{ r: 4 }} connectNulls />
                ))}
              </LineChart>
            </ResponsiveContainer>
          )}
        </div>
      )}
    </div>
  );
}
