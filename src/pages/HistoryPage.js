import React, { useState, useEffect, useMemo } from 'react';
import { subHours, subDays, startOfDay, endOfDay, format } from 'date-fns';
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
import { useApp } from '../context/AppContext';
import { historyMaxItems, prepareHistoryChart } from '../utils/historyChart';
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
    case '24H': return { from: startOfDay(now), to: endOfDay(now) };
    case '7D': return { from: startOfDay(subDays(now, 6)), to: endOfDay(now) };
    case '30D': return { from: startOfDay(subDays(now, 29)), to: endOfDay(now) };
    default: return { from: subHours(now, 24), to: now };
  }
}

export default function HistoryPage() {
  const { selectedFarmId } = useApp();
  const [range, setRange] = useState('7D');
  const [activeKeys, setActiveKeys] = useState(Object.keys(SENSOR_COLORS));
  const [selectedDay, setSelectedDay] = useState(null);
  const [chartData, setChartData] = useState([]);
  const [dailyAgg, setDailyAgg] = useState([]);
  const [dailySource, setDailySource] = useState('loading');
  const [hourlyData, setHourlyData] = useState([]);
  const [loading, setLoading] = useState(false);
  const [loadingHour, setLoadingHour] = useState(false);
  const [chartError, setChartError] = useState('');
  const [hourlyError, setHourlyError] = useState('');

  // Load chart data when range changes
  useEffect(() => {
    setLoading(true);
    setChartError('');
    const { from, to } = getRangeDates(range);
    getAllSensorsHistory(from, to, historyMaxItems(range), selectedFarmId)
      .then(data => {
        if (data.length === 0) {
          setChartData([]);
          setChartError('Belum ada history sensor pada rentang waktu ini.');
          return;
        }
        setChartData(data);
      })
      .catch(() => {
        setChartData([]);
        setChartError('History RTDB tidak dapat dibaca. Periksa koneksi Firebase dan aturan database.');
      })
      .finally(() => setLoading(false));
  }, [range, selectedFarmId]);

  // Load daily summary
  useEffect(() => {
    setDailySource('loading');
    getDailySummary(30, selectedFarmId)
      .then(data => {
        setDailyAgg(data);
        setDailySource(data.length ? 'rtdb' : 'empty');
      })
      .catch(() => {
        setDailyAgg([]);
        setDailySource('error');
      });
  }, [selectedFarmId]);

  // Load hourly data when a day is selected
  useEffect(() => {
    if (!selectedDay) return;
    setLoadingHour(true);
    setHourlyError('');
    getHourlyData(selectedDay, selectedFarmId)
      .then(data => {
        setHourlyData(data);
        if (!data.some(row => Object.keys(SENSOR_COLORS).some(key => row[key] != null))) {
          setHourlyError('Tidak ada data RTDB untuk tanggal ini.');
        }
      })
      .catch(() => {
        setHourlyData([]);
        setHourlyError('Detail per jam RTDB tidak dapat dibaca.');
      })
      .finally(() => setLoadingHour(false));
  }, [selectedDay, selectedFarmId]);

  const rangeDates = useMemo(() => getRangeDates(range), [range]);
  const formattedChart = useMemo(() =>
    prepareHistoryChart(chartData, range, rangeDates.from)
      .map(d => ({ ...d, ts: new Date(d.timestamp).getTime() })),
    [chartData, range, rangeDates]
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
            Historical data is stored in Firebase Realtime Database
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
        ) : chartError ? (
          <div className="history-empty">{chartError}</div>
        ) : (
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={formattedChart} margin={{ top: 5, right: 10, left: -10, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
              <XAxis
                dataKey="ts"
                type="number"
                domain={[rangeDates.from.getTime(), rangeDates.to.getTime()]}
                tickFormatter={formatX}
                tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }}
                axisLine={false} tickLine={false}
                interval="preserveStartEnd" minTickGap={50}
              />
              <YAxis tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }} axisLine={false} tickLine={false} width={38} />
              <Tooltip
                contentStyle={{ fontSize: 12, borderRadius: 8, border: '1px solid #e2e8f0', boxShadow: '0 4px 16px rgba(0,0,0,0.08)' }}
                labelFormatter={v => {
                  const d = new Date(v);
                  if (range === '1H') return `Detail ${format(d, 'HH:mm')} (interval 2 menit)`;
                  if (range === '24H') return `Rata-rata ${format(d, 'HH:00')}–${format(d, 'HH:59')}`;
                  if (range === '7D' || range === '30D') return `Rata-rata harian ${format(d, 'dd MMM yyyy')}`;
                  return d.toLocaleString('id-ID');
                }}
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
          <h3 className="card-label">Daily Summary</h3>
          <div className="daily-table-meta">
            <span className={`daily-source-badge ${dailySource}`}>
              {dailySource === 'rtdb' && 'Firebase RTDB'}
              {dailySource === 'empty' && 'No RTDB history yet'}
              {dailySource === 'error' && 'RTDB unavailable'}
              {dailySource === 'loading' && 'Loading RTDB…'}
            </span>
            <span className="card-sub">Click a row to view hourly details</span>
          </div>
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
              {dailyAgg.length === 0 ? (
                <tr><td className="table-empty" colSpan="6">Belum ada data history dari RTDB.</td></tr>
              ) : dailyAgg.slice(-30).reverse().map(row => (
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
          ) : hourlyError ? (
            <div className="history-empty">{hourlyError}</div>
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
