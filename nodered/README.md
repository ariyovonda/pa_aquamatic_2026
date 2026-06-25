# Node-RED Flow - AquaMonitor

Flow Node-RED untuk menerima data sensor dari ESP8266 via MQTT dan mengirim ke Firebase.

## Prasyarat

- Node.js v18+
- Node-RED terinstall
- Mosquitto MQTT Broker
- Akun Firebase dengan Realtime Database & Firestore

## Instalasi

### 1. Install Node-RED

```bash
npm install -g node-red
```

### 2. Jalankan Node-RED

```bash
node-red
```

Buka [http://localhost:1880](http://localhost:1880)

### 3. Install Node Packages

Buka Node-RED → Menu (☰) → Manage Palette → Install:

| Package                            | Fungsi                       |
| ---------------------------------- | ---------------------------- |
| `node-red-contrib-firebase-rtdb`   | Write ke Realtime Database   |
| `node-red-contrib-cloud-firestore` | Write ke Firestore           |
| `node-red-contrib-mqtt`            | Koneksi MQTT broker          |
| `node-red-node-serialport`         | Komunikasi Serial ke Arduino |

Atau via command line:

```bash
cd ~/.node-red
npm install node-red-contrib-firebase-rtdb node-red-contrib-cloud-firestore node-red-contrib-mqtt node-red-node-serialport
```

### 4. Install & Jalankan Mosquitto Broker

```bash
# Windows (via chocolatey)
choco install mosquitto

# Atau download dari https://mosquitto.org/download/

# Jalankan
mosquitto -v
```

## Import Flow

1. Buka Node-RED [http://localhost:1880](http://localhost:1880)
2. Menu (☰) → Import → Clipboard
3. Copy isi file `flow.json` di folder ini
4. Paste → Import → OK

## Konfigurasi

### MQTT Broker

Double-click node **"Mosquitto Local"** → Edit:

| Field      | Value                    |
| ---------- | ------------------------ |
| Name       | Mosquitto Local          |
| Connection | localhost atau IP broker |
| Port       | 1883                     |
| Client ID  | nodered-aquaponics       |

### Firebase Realtime Database

Double-click node **"RTDB: Set Sensor"**:

| Field            | Value                |
| ---------------- | -------------------- |
| Firebase Project | [Pilih project Anda] |
| Operation        | set                  |
| Path Type        | msg                  |
| Path Field       | rtdbPath             |

### Firebase Firestore

Double-click node **"Firestore: sensor_readings"**:

| Field            | Value                |
| ---------------- | -------------------- |
| Firebase Project | [Pilih project Anda] |
| Collection       | sensor_readings      |
| Operation        | add                  |

### Serial Port (Arduino)

Double-click node **"Serial → Arduino"**:

| Field       | Value                                                                |
| ----------- | -------------------------------------------------------------------- |
| Serial Port | COM3 (sesuaikan dengan port Arduino kamu, misalnya COM3, COM4, COM5) |
| Baud Rate   | 9600                                                                 |
| Newline     | \n                                                                   |

Cek port Arduino: Windows → Device Manager → Ports (COM & LPT)

## Struktur Flow

### Tab: Sensor → Firebase

```
┌────────────────────────────────────────────────────────────────────┐
│  MQTT Input              Function              Firebase RTDB        │
│  ┌──────────┐           ┌───────────┐         ┌─────────────┐      │
│  │ temp     │──────────→│ Validate  │────────→│ Set Sensor  │      │
│  └──────────┘           │ Temp      │         └─────────────┘      │
│  ┌──────────┐           └───────────┘         ┌─────────────┐      │
│  │ pH       │──────────→│ Validate  │────────→│              │      │
│  └──────────┘           │ pH        │         └─────────────┘      │
│  ┌──────────┐           └───────────┘                                 │
│  │ TDS      │──────────→│ Validate  │────────┐                       │
│  └──────────┘           │ TDS       │        │                       │
│  ┌──────────┐           └───────────┘        │                       │
│  │ DO       │──────────→│ Validate  │────────┼──────┐                │
│  └──────────┘           │ DO        │        │      │                │
│  ┌──────────┐           └───────────┘        │      │  Firestore    │
│  │ Turbidity│──────────→│ Validate  │────────┘      │  Add Doc      │
│  └──────────┘           │ Turbidity │                │               │
│                         └───────────┘        ┌───────▼────────┐     │
│                                              │ Prep Firestore  │     │
│                                              │ Doc             │     │
│                                              └─────────────────┘     │
└────────────────────────────────────────────────────────────────────┘
```

### Tab: Aktuator Control

```
┌────────────────────────────────────────────────────────────────────┐
│  MQTT Input              Function              Serial Out          │
│  ┌──────────────┐       ┌───────────┐         ┌──────────────┐    │
│  │ waterPump cmd │──────→│ Relay     │────────→│ Serial →     │    │
│  └──────────────┘       │ waterPump │         │ Arduino      │    │
│  ┌──────────────┐       └───────────┘         └──────────────┘    │
│  │ aerator cmd   │──────→│ Relay     │────────→                  │
│  └──────────────┘       │ aerator   │                             │
│  ┌──────────────┐       └───────────┘                             │
│  │ heater cmd    │──────→│ Relay     │────────→                   │
│  └──────────────┘       │ heater    │                             │
└────────────────────────────────────────────────────────────────────┘
```

## Topic MQTT

### Sensor (ESP → Node-RED)

| Topic                            | Payload           |
| -------------------------------- | ----------------- |
| `aquaponics/sensors/temperature` | `{"value": 26.5}` |
| `aquaponics/sensors/ph`          | `{"value": 7.2}`  |
| `aquaponics/sensors/tds`         | `{"value": 150}`  |
| `aquaponics/sensors/do`          | `{"value": 5.5}`  |
| `aquaponics/sensors/turbidity`   | `{"value": 12}`   |

### Aktuator (Web → ESP via Serial)

| Topic                                        | Payload             |
| -------------------------------------------- | ------------------- |
| `aquaponics/actuators/waterPump/command`     | `{"action": "ON"}`  |
| `aquaponics/actuators/aerator/command`       | `{"action": "OFF"}` |
| `aquaponics/actuators/heater/command`        | `{"action": "ON"}`  |
| `aquaponics/actuators/phPumpDown/command`    | `{"action": "ON"}`  |
| `aquaponics/actuators/phPumpUp/command`      | `{"action": "ON"}`  |
| `aquaponics/actuators/nutritionPump/command` | `{"action": "ON"}`  |

## Payload Format

### Input (dari ESP)

```json
{ "value": 26.5 }
```

### Output ke Firebase RTDB

```json
{
  "value": 26.5,
  "unit": "°C",
  "status": "normal",
  "updatedAt": 1704067200000
}
```

### Output ke Firestore

```json
{
  "sensor": "temperature",
  "value": 26.5,
  "unit": "°C",
  "status": "normal",
  "timestamp": "2024-01-01T00:00:00.000Z"
}
```

## Status Validation

| Sensor      | Normal Range  |
| ----------- | ------------- |
| temperature | 20°C - 30°C   |
| pH          | 5.5 - 8.0     |
| TDS         | 400 - 800 ppm |
| DO          | 4 - 10 mg/L   |
| turbidity   | 0.5 - 5.0 NTU |

## Troubleshooting

### MQTT tidak konek

```bash
# Cek Mosquitto running
netstat -an | grep 1883

# Test Mosquitto
mosquitto_pub -t test -m "hello"
```

### Firebase node error

1. Pastikan package terinstall: `npm list node-red-contrib-firebase-rtdb`
2. Restart Node-RED: `node-red restart`
3. Cek credentials Firebase valid

### Serial port error

1. Cek Arduino connected
2. Cek port benar (Device Manager → COM ports)
3. Pastikan baud rate match (9600)

### Flow tidak deploy

1. Klik "Deploy" button
2. Cek semua node ter-konfigurasi
3. Cek console error (☰ → View → Runtime Debug)

## Contoh Sketch ESP8266

```cpp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* mqtt_server = "192.168.1.100";  // IP Node-RED

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP_Aquaponics")) {
      // Subscribe topics if needed
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Baca sensor (sesuaikan dengan hardware)
  float temp = readTemperature();
  float ph = readPH();

  // Publish ke MQTT
  char msg[50];
  snprintf(msg, 50, "{\"value\": %.2f}", temp);
  client.publish("aquaponics/sensors/temperature", msg);

  delay(5000);
}
```

## File

| File        | Deskripsi                      |
| ----------- | ------------------------------ |
| `flow.json` | Node-RED flow yang siap import |
| `README.md` | Dokumentasi ini                |
