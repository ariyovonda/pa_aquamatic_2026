// src/firebase/firebaseService.js
// Semua operasi baca/tulis ke Firebase

import {
  collection,
  addDoc,
  query,
  where,
  orderBy,
  limit,
  getDocs,
  Timestamp,
  doc,
  setDoc,
  getDoc,
} from "firebase/firestore";

import { ref, set, onValue, update } from "firebase/database";

import { db, rtdb } from "./config";

// ─────────────────────────────────────────────────────────────
//  REALTIME DATABASE  (data terkini sensor & aktuator)
// ─────────────────────────────────────────────────────────────

/**
 * Subscribe data sensor terkini (real-time)
 * Path RTDB: /sensors/{sensorKey}  → { value, unit, status, updatedAt }
 * @param {function} callback - dipanggil setiap ada perubahan data
 * @returns unsubscribe function
 */
export function subscribeSensors(callback, farmId = null) {
  const path = farmId ? `farms/${farmId}/sensors` : "sensors";
  const sensorsRef = ref(rtdb, path);
  return onValue(sensorsRef, (snapshot) => {
    const data = snapshot.val();
    if (data) callback(data);
  });
}

/**
 * Subscribe status aktuator (real-time)
 * Path RTDB: /actuators/{deviceId}  → { enabled, running, mode, ... }
 */
export function subscribeActuators(callback, farmId = null) {
  const path = farmId ? `farms/${farmId}/actuators` : "actuators";
  const actRef = ref(rtdb, path);
  return onValue(actRef, (snapshot) => {
    const data = snapshot.val();
    if (data) callback(data);
  });
}

/**
 * Update status aktuator di RTDB
 */
export async function updateActuatorRTDB(deviceId, changes, farmId = null) {
  const path = farmId
    ? `farms/${farmId}/actuators/${deviceId}`
    : `actuators/${deviceId}`;
  const actRef = ref(rtdb, path);
  await update(actRef, { ...changes, updatedAt: Date.now() });
}

/**
 * Tulis data sensor terkini ke RTDB
 * Dipanggil oleh Node-RED / ESP langsung
 */
export async function writeSensorRTDB(
  sensorKey,
  value,
  unit,
  status,
  farmId = null,
) {
  const path = farmId
    ? `farms/${farmId}/sensors/${sensorKey}`
    : `sensors/${sensorKey}`;
  const sRef = ref(rtdb, path);
  await set(sRef, { value, unit, status, updatedAt: Date.now() });
}

// ─────────────────────────────────────────────────────────────
//  FIRESTORE  (riwayat historis sensor)
// ─────────────────────────────────────────────────────────────

/**
 * Simpan satu pembacaan sensor ke Firestore
 * Collection: sensor_readings
 */
export async function saveSensorReading(sensorKey, value, unit, status) {
  await addDoc(collection(db, "sensor_readings"), {
    sensor: sensorKey,
    value,
    unit,
    status,
    timestamp: Timestamp.now(),
  });
}

/**
 * Ambil riwayat sensor dalam rentang waktu
 * @param {string} sensorKey - 'temperature' | 'ph' | 'tds' | 'do' | 'turbidity'
 * @param {Date}   fromDate
 * @param {Date}   toDate
 * @param {number} maxDocs
 */
export async function getSensorHistory(
  sensorKey,
  fromDate,
  toDate,
  maxDocs = 500,
) {
  const q = query(
    collection(db, "sensor_readings"),
    where("sensor", "==", sensorKey),
    where("timestamp", ">=", Timestamp.fromDate(fromDate)),
    where("timestamp", "<=", Timestamp.fromDate(toDate)),
    orderBy("timestamp", "asc"),
    limit(maxDocs),
  );
  const snap = await getDocs(q);
  return snap.docs.map((d) => ({
    id: d.id,
    ...d.data(),
    timestamp: d.data().timestamp.toDate().toISOString(),
  }));
}

/**
 * Ambil semua sensor dalam rentang (untuk chart multi-sensor)
 */
export async function getAllSensorsHistory(fromDate, toDate, maxDocs = 1000) {
  const sensors = ["temperature", "ph", "tds", "do", "turbidity"];
  const results = await Promise.all(
    sensors.map((s) =>
      getSensorHistory(s, fromDate, toDate, maxDocs / sensors.length),
    ),
  );
  // Gabungkan dan sort berdasarkan timestamp
  const merged = results
    .flat()
    .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
  // Pivot ke format { timestamp, temperature, ph, tds, do, turbidity }
  const byTime = {};
  merged.forEach((r) => {
    if (!byTime[r.timestamp]) byTime[r.timestamp] = { timestamp: r.timestamp };
    byTime[r.timestamp][r.sensor] = r.value;
  });
  return Object.values(byTime).sort(
    (a, b) => new Date(a.timestamp) - new Date(b.timestamp),
  );
}

/**
 * Ambil ringkasan harian (rata-rata per hari)
 * Firestore tidak punya native GROUP BY, jadi kita agregasi di client
 */
export async function getDailySummary(days = 30) {
  const from = new Date();
  from.setDate(from.getDate() - days);
  from.setHours(0, 0, 0, 0);

  const q = query(
    collection(db, "sensor_readings"),
    where("timestamp", ">=", Timestamp.fromDate(from)),
    orderBy("timestamp", "asc"),
    limit(5000),
  );
  const snap = await getDocs(q);
  const docs = snap.docs.map((d) => ({
    ...d.data(),
    timestamp: d.data().timestamp.toDate(),
  }));

  // Group by date + sensor
  const groups = {};
  docs.forEach((d) => {
    const dateKey = d.timestamp.toISOString().split("T")[0];
    if (!groups[dateKey]) groups[dateKey] = {};
    if (!groups[dateKey][d.sensor]) groups[dateKey][d.sensor] = [];
    groups[dateKey][d.sensor].push(d.value);
  });

  const avg = (arr) =>
    arr.length
      ? +(arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(2)
      : null;

  return Object.entries(groups)
    .map(([date, sensors]) => ({
      date,
      temperature: avg(sensors.temperature || []),
      ph: avg(sensors.ph || []),
      tds: avg(sensors.tds || []),
      do: avg(sensors.do || []),
      turbidity: avg(sensors.turbidity || []),
    }))
    .sort((a, b) => a.date.localeCompare(b.date));
}

/**
 * Ambil data per jam untuk satu hari tertentu
 */
export async function getHourlyData(dateStr) {
  const from = new Date(dateStr + "T00:00:00");
  const to = new Date(dateStr + "T23:59:59");

  const q = query(
    collection(db, "sensor_readings"),
    where("timestamp", ">=", Timestamp.fromDate(from)),
    where("timestamp", "<=", Timestamp.fromDate(to)),
    orderBy("timestamp", "asc"),
    limit(2000),
  );
  const snap = await getDocs(q);
  const docs = snap.docs.map((d) => ({
    ...d.data(),
    timestamp: d.data().timestamp.toDate(),
  }));

  // Group by hour + sensor
  const groups = {};
  docs.forEach((d) => {
    const h = d.timestamp.getHours();
    if (!groups[h]) groups[h] = {};
    if (!groups[h][d.sensor]) groups[h][d.sensor] = [];
    groups[h][d.sensor].push(d.value);
  });

  const avg = (arr) =>
    arr.length
      ? +(arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(2)
      : null;

  return Array.from({ length: 24 }, (_, h) => ({
    hour: `${String(h).padStart(2, "0")}:00`,
    temperature: avg((groups[h] || {}).temperature || []),
    ph: avg((groups[h] || {}).ph || []),
    tds: avg((groups[h] || {}).tds || []),
    do: avg((groups[h] || {}).do || []),
    turbidity: avg((groups[h] || {}).turbidity || []),
  }));
}

// ─────────────────────────────────────────────────────────────
//  LOG AKTUATOR (Firestore)
// ─────────────────────────────────────────────────────────────

export async function logActuatorAction(deviceId, action, source = "web") {
  await addDoc(collection(db, "actuator_logs"), {
    device: deviceId,
    action,
    source,
    timestamp: Timestamp.now(),
  });
}

export async function getActuatorLogs(limitN = 50) {
  const q = query(
    collection(db, "actuator_logs"),
    orderBy("timestamp", "desc"),
    limit(limitN),
  );
  const snap = await getDocs(q);
  return snap.docs.map((d) => ({
    id: d.id,
    ...d.data(),
    timestamp: d.data().timestamp.toDate().toISOString(),
  }));
}

// ─────────────────────────────────────────────────────────────
//  THRESHOLD SETTINGS (Firestore)
// ─────────────────────────────────────────────────────────────

export async function saveThresholds(thresholds) {
  await setDoc(doc(db, "settings", "thresholds"), {
    ...thresholds,
    updatedAt: Timestamp.now(),
  });
}

export async function loadThresholds() {
  const snap = await getDoc(doc(db, "settings", "thresholds"));
  if (snap.exists()) {
    const data = snap.data();
    delete data.updatedAt;
    return data;
  }
  return null;
}

export async function getUserProfile(uid) {
  const snap = await getDoc(doc(db, "users", uid));
  if (snap.exists()) {
    return { id: snap.id, ...snap.data() };
  }
  return null;
}

export async function createUserProfile(uid, email) {
  const userRef = doc(db, "users", uid);
  const existing = await getDoc(userRef);
  if (existing.exists()) {
    return { id: existing.id, ...existing.data() };
  }

  const defaultFarm = {
    id: "default",
    name: "Kolam Utama",
    createdAt: Timestamp.now(),
  };

  const profile = {
    email,
    farms: [defaultFarm],
    selectedFarm: defaultFarm.id,
    createdAt: Timestamp.now(),
  };

  await setDoc(userRef, profile);
  return { id: uid, ...profile };
}

export async function updateUserProfile(uid, updates) {
  const userRef = doc(db, "users", uid);
  await setDoc(userRef, updates, { merge: true });
}
