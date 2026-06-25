// Semua operasi baca/tulis aplikasi menggunakan Firebase Realtime Database.
import {
  ref,
  set,
  get,
  push,
  onValue,
  update,
  query,
  orderByChild,
  startAt,
  endAt,
  limitToLast,
} from "firebase/database";
import { rtdb } from "./config";

const SENSOR_KEYS = ["temperature", "ph", "tds", "do", "turbidity"];

// Node-RED untuk instalasi ini menulis data kolam utama pada root RTDB
// (/sensors, /actuators, /history). Profil pengguna membuat farm "default",
// sehingga ia harus menunjuk jalur root yang sama, bukan /farms/default.
// Farm tambahan tetap dapat memakai cabang /farms/<id> setelah flow Node-RED
// mereka dikonfigurasi ke path tersebut.
const farmPath = (farmId, path) =>
  farmId && farmId !== "default" ? `farms/${farmId}/${path}` : path;
const asMillis = (value) => {
  if (typeof value === "number") return value;
  const parsed = new Date(value).getTime();
  return Number.isFinite(parsed) ? parsed : 0;
};

export function subscribeSensors(callback, farmId = null) {
  return onValue(ref(rtdb, farmPath(farmId, "sensors")), (snapshot) => {
    callback(snapshot.val() || {});
  });
}

export function subscribeActuators(callback, farmId = null) {
  return onValue(ref(rtdb, farmPath(farmId, "actuators")), (snapshot) => {
    callback(snapshot.val() || {});
  });
}

export async function updateSensorRTDB(sensorKey, changes, farmId = null) {
  await update(ref(rtdb, farmPath(farmId, `sensors/${sensorKey}`)), {
    ...changes,
    updatedAt: Date.now(),
  });
}

export async function updateActuatorRTDB(deviceId, changes, farmId = null) {
  await update(ref(rtdb, farmPath(farmId, `actuators/${deviceId}`)), {
    ...changes,
    updatedAt: Date.now(),
  });
}

// RPC web -> Node-RED. Browser tidak mengirim MQTT langsung; Node-RED yang
// meneruskan command ke ESP sehingga kontrol tetap bekerja tanpa WebSocket MQTT.
export async function requestActuatorCommand(deviceId, action, farmId = null) {
  const normalizedAction = action === "on" || action === "enable" ? "on" : "off";
  const requestedAt = Date.now();
  await update(ref(rtdb, farmPath(farmId, `actuators/${deviceId}`)), {
    enabled: normalizedAction === "on",
    command: {
      id: `web-${deviceId}-${requestedAt}`,
      action: normalizedAction,
      source: "web",
      requestedAt,
    },
    updatedAt: requestedAt,
  });
}

// Menggunakan update agar flag enabled tidak terhapus oleh pembacaan sensor baru.
export async function writeSensorRTDB(sensorKey, value, unit, status, farmId = null) {
  await updateSensorRTDB(sensorKey, { value, unit, status }, farmId);
}

/** Simpan satu titik historis pada /history/sensors/{sensorKey}/{pushId}. */
export async function saveSensorReading(
  sensorKey,
  value,
  unit,
  status,
  farmId = null,
) {
  const historyRef = push(ref(rtdb, farmPath(farmId, `history/sensors/${sensorKey}`)));
  const timestamp = Date.now();
  await set(historyRef, { value, unit, status, timestamp });
  return historyRef.key;
}

export async function getSensorHistory(
  sensorKey,
  fromDate,
  toDate,
  maxItems = 500,
  farmId = null,
) {
  const from = fromDate.getTime();
  const to = toDate.getTime();
  const historyQuery = query(
    ref(rtdb, farmPath(farmId, `history/sensors/${sensorKey}`)),
    orderByChild("timestamp"),
    startAt(from),
    endAt(to),
    limitToLast(maxItems),
  );
  const snap = await get(historyQuery);
  return Object.entries(snap.val() || {})
    .map(([id, value]) => ({
      id,
      sensor: sensorKey,
      ...value,
      timestamp: new Date(asMillis(value.timestamp)).toISOString(),
    }))
    .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
}

export async function getAllSensorsHistory(
  fromDate,
  toDate,
  maxItems = 1000,
  farmId = null,
) {
  const perSensor = Math.max(1, Math.floor(maxItems / SENSOR_KEYS.length));
  const readings = await Promise.all(
    SENSOR_KEYS.map((sensor) =>
      getSensorHistory(sensor, fromDate, toDate, perSensor, farmId),
    ),
  );
  const byTime = {};
  readings.flat().forEach((reading) => {
    if (!byTime[reading.timestamp]) byTime[reading.timestamp] = { timestamp: reading.timestamp };
    byTime[reading.timestamp][reading.sensor] = reading.value;
  });
  return Object.values(byTime).sort(
    (a, b) => new Date(a.timestamp) - new Date(b.timestamp),
  );
}

export async function getDailySummary(days = 30, farmId = null) {
  const from = new Date();
  from.setDate(from.getDate() - days);
  from.setHours(0, 0, 0, 0);
  const readings = await Promise.all(
    SENSOR_KEYS.map((sensor) => getSensorHistory(sensor, from, new Date(), 1000, farmId)),
  );
  const groups = {};
  readings.flat().forEach((reading) => {
    const date = reading.timestamp.slice(0, 10);
    if (!groups[date]) groups[date] = {};
    if (!groups[date][reading.sensor]) groups[date][reading.sensor] = [];
    groups[date][reading.sensor].push(Number(reading.value));
  });
  const average = (items = []) =>
    items.length
      ? +(items.reduce((sum, item) => sum + item, 0) / items.length).toFixed(2)
      : null;
  return Object.entries(groups)
    .map(([date, values]) => ({
      date,
      ...Object.fromEntries(SENSOR_KEYS.map((sensor) => [sensor, average(values[sensor])])),
    }))
    .sort((a, b) => a.date.localeCompare(b.date));
}

export async function getHourlyData(dateStr, farmId = null) {
  const from = new Date(`${dateStr}T00:00:00`);
  const to = new Date(`${dateStr}T23:59:59.999`);
  const readings = await Promise.all(
    SENSOR_KEYS.map((sensor) => getSensorHistory(sensor, from, to, 2000, farmId)),
  );
  const groups = {};
  readings.flat().forEach((reading) => {
    const hour = new Date(reading.timestamp).getHours();
    if (!groups[hour]) groups[hour] = {};
    if (!groups[hour][reading.sensor]) groups[hour][reading.sensor] = [];
    groups[hour][reading.sensor].push(Number(reading.value));
  });
  const average = (items = []) =>
    items.length
      ? +(items.reduce((sum, item) => sum + item, 0) / items.length).toFixed(2)
      : null;
  return Array.from({ length: 24 }, (_, hour) => ({
    hour: `${String(hour).padStart(2, "0")}:00`,
    ...Object.fromEntries(
      SENSOR_KEYS.map((sensor) => [sensor, average(groups[hour]?.[sensor])]),
    ),
  }));
}

export async function logActuatorAction(deviceId, action, source = "web", farmId = null) {
  const logRef = push(ref(rtdb, farmPath(farmId, "history/actuators")));
  await set(logRef, { device: deviceId, action, source, timestamp: Date.now() });
  return logRef.key;
}

export async function getActuatorLogs(limitN = 50, farmId = null) {
  const logsQuery = query(
    ref(rtdb, farmPath(farmId, "history/actuators")),
    orderByChild("timestamp"),
    limitToLast(limitN),
  );
  const snap = await get(logsQuery);
  return Object.entries(snap.val() || {})
    .map(([id, log]) => ({ ...log, id, timestamp: new Date(asMillis(log.timestamp)).toISOString() }))
    .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
}

// Profil pengguna juga berada di RTDB; Firebase Authentication tetap dipakai untuk login.
export async function getUserProfile(uid) {
  const snap = await get(ref(rtdb, `users/${uid}`));
  return snap.exists() ? { id: uid, ...snap.val() } : null;
}

export async function createUserProfile(uid, email) {
  const existing = await getUserProfile(uid);
  if (existing) return existing;
  const defaultFarm = { id: "default", name: "Kolam Utama", createdAt: Date.now() };
  const profile = { email, farms: [defaultFarm], selectedFarm: defaultFarm.id, createdAt: Date.now() };
  await set(ref(rtdb, `users/${uid}`), profile);
  return { id: uid, ...profile };
}

export async function updateUserProfile(uid, updates) {
  await update(ref(rtdb, `users/${uid}`), updates);
}
