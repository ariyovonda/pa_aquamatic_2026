import { subDays, subHours, format } from 'date-fns';

export function generateHistoryData(days = 30) {
  const data = [];
  const now = new Date();
  const totalPoints = days * 24 * 6; // every 10 minutes

  for (let i = totalPoints; i >= 0; i--) {
    const ts = new Date(now.getTime() - i * 10 * 60 * 1000);
    const hour = ts.getHours();
    // Simulate day/night variation
    const dayFactor = Math.sin((hour - 6) * Math.PI / 12); // peaks midday

    data.push({
      timestamp: ts.toISOString(),
      temperature: +(24 + dayFactor * 3 + (Math.random() - 0.5) * 1.5).toFixed(1),
      ph: +(6.8 + (Math.random() - 0.5) * 0.6).toFixed(2),
      tds: +(620 + (Math.random() - 0.5) * 80).toFixed(0),
      do: +(7.0 + dayFactor * 1.5 + (Math.random() - 0.5) * 0.8).toFixed(1),
      turbidity: +(2.5 + (Math.random() - 0.5) * 1.5).toFixed(1),
    });
  }
  return data;
}

export function generateMockData() {
  return {
    temperature: 25.7,
    ph: 6.8,
    tds: 628,
    do: 7.1,
    turbidity: 2.3,
  };
}

export function filterHistoryByRange(data, range) {
  const now = new Date();
  let cutoff;
  switch (range) {
    case '1H': cutoff = subHours(now, 1); break;
    case '24H': cutoff = subHours(now, 24); break;
    case '7D': cutoff = subDays(now, 7); break;
    case '30D': cutoff = subDays(now, 30); break;
    default: cutoff = subHours(now, 24);
  }
  return data.filter(d => new Date(d.timestamp) >= cutoff);
}

export function getDailyAggregates(data) {
  const groups = {};
  data.forEach(point => {
    const day = format(new Date(point.timestamp), 'yyyy-MM-dd');
    if (!groups[day]) groups[day] = { temperature: [], ph: [], tds: [], do: [], turbidity: [] };
    ['temperature', 'ph', 'tds', 'do', 'turbidity'].forEach(k => {
      if (point[k] != null) groups[day][k].push(point[k]);
    });
  });
  return Object.entries(groups).map(([date, vals]) => {
    const avg = arr => arr.length ? +(arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(2) : null;
    return {
      date,
      temperature: avg(vals.temperature),
      ph: avg(vals.ph),
      tds: avg(vals.tds),
      do: avg(vals.do),
      turbidity: avg(vals.turbidity),
    };
  }).sort((a, b) => a.date.localeCompare(b.date));
}
