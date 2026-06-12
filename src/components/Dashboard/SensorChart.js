import React, { useMemo } from 'react';
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, Legend, ResponsiveContainer
} from 'recharts';
import { format } from 'date-fns';
import { id } from 'date-fns/locale';
import './SensorChart.css';

const SENSOR_KEYS = ['temperature', 'ph', 'tds', 'do', 'turbidity'];

const LABEL_MAP = {
  temperature: 'Suhu (°C)',
  ph: 'pH',
  tds: 'TDS (ppm)',
  do: 'DO (mg/L)',
  turbidity: 'Kekeruhan (NTU)',
};

function formatXAxis(ts, range) {
  const d = new Date(ts);
  if (range === '1H' || range === '24H') return format(d, 'HH:mm');
  if (range === '7D') return format(d, 'EEE HH:mm', { locale: id });
  return format(d, 'dd MMM', { locale: id });
}

const CustomTooltip = ({ active, payload, label }) => {
  if (!active || !payload || !payload.length) return null;
  return (
    <div className="chart-tooltip">
      <div className="tooltip-time">{new Date(label).toLocaleString('id-ID')}</div>
      {payload.map(p => (
        <div key={p.dataKey} className="tooltip-row">
          <span className="tooltip-dot" style={{ background: p.color }} />
          <span className="tooltip-label">{LABEL_MAP[p.dataKey] || p.dataKey}:</span>
          <span className="tooltip-val">{p.value}</span>
        </div>
      ))}
    </div>
  );
};

export default function SensorChart({ data, activeTab, colors, meta, range }) {
  const visibleKeys = activeTab === 'all' ? SENSOR_KEYS : [activeTab];

  const formatted = useMemo(() =>
    data.map(d => ({ ...d, ts: new Date(d.timestamp).getTime() })),
    [data]
  );

  if (!formatted.length) {
    return (
      <div className="chart-empty">
        <span>Belum ada data untuk rentang waktu ini</span>
      </div>
    );
  }

  return (
    <div className="sensor-chart">
      <ResponsiveContainer width="100%" height={300}>
        <LineChart data={formatted} margin={{ top: 5, right: 10, left: -10, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
          <XAxis
            dataKey="ts"
            tickFormatter={(v) => formatXAxis(v, range)}
            tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }}
            axisLine={false}
            tickLine={false}
            interval="preserveStartEnd"
            minTickGap={40}
          />
          <YAxis
            tick={{ fontSize: 10, fill: '#94a3b8', fontFamily: 'DM Mono' }}
            axisLine={false}
            tickLine={false}
            width={40}
          />
          <Tooltip content={<CustomTooltip />} />
          {activeTab === 'all' && (
            <Legend
              wrapperStyle={{ fontSize: '11px', paddingTop: '12px' }}
              formatter={(value) => LABEL_MAP[value] || value}
            />
          )}
          {visibleKeys.map(key => (
            <Line
              key={key}
              type="monotone"
              dataKey={key}
              stroke={colors[key]}
              strokeWidth={2}
              dot={false}
              activeDot={{ r: 4, strokeWidth: 0 }}
              connectNulls
            />
          ))}
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
