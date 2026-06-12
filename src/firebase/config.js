// src/firebase/config.js
// ─────────────────────────────────────────────────────────────
//  CARA SETUP:
//  1. Buka https://console.firebase.google.com
//  2. Buat project baru → nama: "aquamonitor"
//  3. Tambah Web App → salin firebaseConfig ke sini
//  4. Aktifkan Firestore Database (mode test untuk dev)
//  5. Aktifkan Realtime Database
// ─────────────────────────────────────────────────────────────

import { initializeApp } from "firebase/app";
import { getFirestore } from "firebase/firestore";
import { getDatabase } from "firebase/database";
import { getAuth } from 'firebase/auth';

// 🔴 GANTI dengan config Firebase project kamu!
const firebaseConfig = {
  apiKey: "AIzaSyDYmk_9OD1AnWP3c-1CjzVrojLEFG2sYnM",
  authDomain: "akuaponik-5bdfa.firebaseapp.com",
  databaseURL: "https://akuaponik-5bdfa-default-rtdb.firebaseio.com",
  projectId: "akuaponik-5bdfa",
  storageBucket: "akuaponik-5bdfa.appspot.com",
  messagingSenderId: "273363250481",
  appId: "1:273363250481:web:00d40c957d358a28a71410",
};

const app = initializeApp(firebaseConfig);
export const db = getFirestore(app); // Firestore  → riwayat historis
export const rtdb = getDatabase(app); // Realtime DB → data live sensor
export const auth = getAuth(app);

export default app;
