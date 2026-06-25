# AquaMonitor - IoT Aquaponics Monitoring System

Sistem monitoring akuaponik berbasis IoT dengan React + Firebase.

## Fitur

- **Dashboard Real-time**: Monitoring sensor suhu, pH, TDS, DO, dan turbidity
- **Grafik Historis**: Visualisasi data sensor dalam rentang waktu
- **Kontrol Aktuator**: Kontrol pompa air, aerator, heater, dan buzzer
- **Notifikasi Alert**: Notifikasi otomatis saat nilai sensor di luar ambang batas
- **Kontrol Sensor**: Sensor dapat dinyalakan atau dimatikan dari Dashboard
- **Otomasi Aktuator**: Threshold per aktuator tersimpan di Realtime Database

## Prasyarat

- Node.js v18+
- npm atau yarn

## Instalasi

### 1. Install dependencies

```bash
npm install
```

### 2. Konfigurasi Firebase

Buka `src/firebas e/config.js` dan isi dengan konfigurasi Firebase Anda:

```javascript`
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "YOUR_PROJECT.firebaseapp.com",
  databaseURL: "https://YOUR_PROJECT-default-rtdb.firebaseio.com",
  projectId: "YOUR_PROJECT_ID",
  storageBucket: "YOUR_PROJECT.appspot.com",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID"
};
```

Untuk mendapatkan config:
1. Buka [Firebase Console](https://console.firebase.google.com)
2. Pilih project → Settings (gear icon) → General
3. Scroll ke "Your apps" → Pilih Web app (</>) → Copy config

### 3. Jalankan development server

```bash
npm start
```

Buka [http://localhost:3000](http://localhost:3000) di browser.

## Struktur Project Saya

```
src/
├── components/
│   ├── Dashboard/        # Komponen dashboard
│   ├── ActuatorCard.js   # Kartu kontrol aktuator
│   └── Header.js         # Header navigasi
├── context/
│   ├── AppContext.js     # State management utama
│   └── MqttContext.js    # Koneksi MQTT
├── firebase/
│   ├── config.js         # Konfigurasi Firebase
│   └── firebaseService.js # Servis Firebase
├── pages/
│   ├── DashboardPage.js  # Halaman utama dashboard
│   └── SettingsPage.js   # Halaman pengaturan
└── utils/
    └── mockData.js       # Data dummy untuk development
```

## Alur Data

```
ESP8266/ESP32 → MQTT Broker → Node-RED → Firebase RTDB
                                               ↓
                                         React App + History
```

## Available Scripts

| Command | Deskripsi |
|---------|-----------|
| `npm start` | Jalankan development server |
| `npm run build` | Build untuk production |
| `npm test` | Jalankan unit tests |
| `npm run lint` | Jalankan ESLint |

## Konsep Penting

### Threshold Otomasi Aktuator
Setiap aktuator dapat memakai mode `auto` dan aturan `automation` sendiri:
sensor sumber, kondisi `below`/`above`, nilai threshold, serta hysteresis.
Aturan disimpan pada `/actuators/{actuatorId}/automation` di RTDB. Sensor tidak
memiliki threshold global; status ON/OFF sensor disimpan pada
`/sensors/{sensorId}/enabled`.

### Status Sensor
- **normal**: Nilai dalam range aman
- **warning**: Mendekati batas (di luar range tapi tidak kritis)
- **critical**: Di luar batas aman

## Troubleshooting

### Error "Firebase config not found"
Pastikan `src/firebase/config.js` sudah diisi dengan config yang valid dari Firebase Console.

### Data tidak update real-time
1. Cek apakah Realtime Database rules mengizinkan read/write
2. Cek console browser untuk error koneksi
3. Pastikan ESP/Node-RED sudah push data ke database

### Build error
```bash
rm -rf node_modules
npm install
```

## Lisensi

MIT
