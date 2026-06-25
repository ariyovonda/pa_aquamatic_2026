const SENSOR_KEYS = ["temperature", "ph", "tds", "do", "turbidity"];

const average = (values) =>
  values.length
    ? +(values.reduce((sum, value) => sum + value, 0) / values.length).toFixed(2)
    : null;

function bucketTimestamp(timestamp, minutes) {
  const date = new Date(timestamp);
  if (minutes >= 24 * 60) {
    date.setHours(0, 0, 0, 0);
    return date.getTime();
  }
  date.setSeconds(0, 0);
  date.setMinutes(Math.floor(date.getMinutes() / minutes) * minutes);
  return date.getTime();
}

/**
 * 1H: satu titik untuk setiap bucket dua menit.
 * 24H: satu titik rata-rata untuk setiap jam pada hari yang dipilih.
 * 7D/30D: satu titik rata-rata untuk setiap hari kalender.
 */
export function prepareHistoryChart(data, range, rangeStart) {
  if (range !== "1H" && range !== "24H" && range !== "7D" && range !== "30D") return data;

  const bucketMinutes = range === "1H" ? 2 : range === "24H" ? 60 : 24 * 60;
  const buckets = new Map();

  data.forEach((row) => {
    const timestamp = bucketTimestamp(row.timestamp, bucketMinutes);
    if (!buckets.has(timestamp)) {
      buckets.set(timestamp, Object.fromEntries(SENSOR_KEYS.map((key) => [key, []])));
    }
    const values = buckets.get(timestamp);
    SENSOR_KEYS.forEach((key) => {
      if (Number.isFinite(Number(row[key]))) values[key].push(Number(row[key]));
    });
  });

  // Untuk 24H, tampilkan seluruh jam kalender, termasuk jam yang belum memiliki data.
  if (range === "24H") {
    const start = new Date(rangeStart);
    start.setHours(0, 0, 0, 0);
    return Array.from({ length: 24 }, (_, hour) => {
      const timestamp = new Date(start);
      timestamp.setHours(hour);
      const values = buckets.get(timestamp.getTime());
      return {
        timestamp: timestamp.toISOString(),
        ...Object.fromEntries(
          SENSOR_KEYS.map((key) => [key, values ? average(values[key]) : null]),
        ),
      };
    });
  }

  if (range === "7D" || range === "30D") {
    const days = range === "7D" ? 7 : 30;
    const start = new Date(rangeStart);
    start.setHours(0, 0, 0, 0);
    return Array.from({ length: days }, (_, day) => {
      const timestamp = new Date(start);
      timestamp.setDate(start.getDate() + day);
      const values = buckets.get(timestamp.getTime());
      return {
        timestamp: timestamp.toISOString(),
        ...Object.fromEntries(
          SENSOR_KEYS.map((key) => [key, values ? average(values[key]) : null]),
        ),
      };
    });
  }

  return [...buckets.entries()]
    .map(([timestamp, values]) => ({
      timestamp: new Date(timestamp).toISOString(),
      ...Object.fromEntries(SENSOR_KEYS.map((key) => [key, average(values[key])])),
    }))
    .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
}

export function historyMaxItems(range) {
  if (range === "1H") return 300;
  if (range === "24H") return 4000;
  return 600;
}
