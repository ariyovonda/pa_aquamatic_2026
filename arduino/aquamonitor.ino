/*
 * ============================================================
 *  AquaMonitor – Arduino ATmega + ESP8266/ESP32
 *  Sistem Monitoring Akuaponik (Selada & Ikan Nila)
 * ============================================================
 *  Sensor  : DS18B20 (Suhu), pH Sensor (analog), TDS Sensor,
 *            DO Sensor (analog), Turbidity Sensor (analog)
 *  Aktuator: Pompa Air (pin 7), Aerator (pin 8), Heater (pin 9)
 *  Protokol: MQTT via ESP8266/ESP32 (Serial bridge)
 * ============================================================
 *  Wiring Summary:
 *  - DS18B20 DATA  -> pin 2 (OneWire)
 *  - pH Sensor     -> A0
 *  - TDS Sensor    -> A1
 *  - DO Sensor     -> A2
 *  - Turbidity     -> A3
 *  - Water Pump    -> pin 7 (relay)
 *  - Aerator       -> pin 8 (relay)
 *  - Heater        -> pin 9 (relay)
 *  - ESP TX        -> pin 11 (SoftwareSerial RX on ATmega)
 *  - ESP RX        -> pin 10 (SoftwareSerial TX on ATmega)
 * ============================================================
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>  // v6

// ── Pins ──────────────────────────────────────────────────────
#define PIN_DS18B20       2
#define PIN_PH            A0
#define PIN_TDS           A1
#define PIN_DO            A2
#define PIN_TURBIDITY     A3
#define PIN_PUMP_WATER    7
#define PIN_AERATOR       8
#define PIN_HEATER        9
#define PIN_ESP_TX        10   // ATmega TX → ESP RX
#define PIN_ESP_RX        11   // ATmega RX ← ESP TX

// ── Timing ────────────────────────────────────────────────────
#define SEND_INTERVAL     5000    // ms between sensor readings

// ── Objects ───────────────────────────────────────────────────
OneWire           oneWire(PIN_DS18B20);
DallasTemperature tempSensor(&oneWire);
SoftwareSerial    espSerial(PIN_ESP_RX, PIN_ESP_TX);

unsigned long     lastSend = 0;

// ── Calibration constants ─────────────────────────────────────
// Adjust these for your specific sensors!
const float PH_OFFSET     = 0.0;    // pH calibration offset
const float TDS_FACTOR    = 0.5;    // TDS voltage to ppm factor
const float DO_FACTOR     = 5.0;    // DO conversion factor (depends on sensor)
const float TURB_FACTOR   = 1.0;    // Turbidity conversion

// ── Actuator state ────────────────────────────────────────────
bool pumpWaterOn  = true;
bool aeratorOn    = true;
bool heaterOn     = false;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  tempSensor.begin();

  pinMode(PIN_PUMP_WATER, OUTPUT);
  pinMode(PIN_AERATOR,    OUTPUT);
  pinMode(PIN_HEATER,     OUTPUT);

  // Default states
  digitalWrite(PIN_PUMP_WATER, HIGH);  // relay active LOW → HIGH = OFF if using active-low relay
  digitalWrite(PIN_AERATOR,    HIGH);
  digitalWrite(PIN_HEATER,     LOW);

  Serial.println("[BOOT] AquaMonitor ready");
}

void loop() {
  unsigned long now = millis();

  // ── 1. Read & publish sensors every SEND_INTERVAL ─────────
  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
    readAndPublishSensors();
  }

  // ── 2. Listen for actuator commands from ESP (JSON) ────────
  if (espSerial.available()) {
    String line = espSerial.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) handleCommand(line);
  }
}

// ─────────────────────────────────────────────────────────────
void readAndPublishSensors() {
  float temperature = readTemperature();
  float ph          = readPH();
  float tds         = readTDS();
  float doVal       = readDO();
  float turbidity   = readTurbidity();

  publishSensor("temperature", temperature);
  publishSensor("ph",          ph);
  publishSensor("tds",         tds);
  publishSensor("do",          doVal);
  publishSensor("turbidity",   turbidity);

  // Auto-control heater based on temperature
  if (temperature < 22.0 && !heaterOn) {
    setActuator("heater", PIN_HEATER, true);
  } else if (temperature > 28.0 && heaterOn) {
    setActuator("heater", PIN_HEATER, false);
  }

  Serial.print("[DATA] T="); Serial.print(temperature);
  Serial.print(" pH=");       Serial.print(ph);
  Serial.print(" TDS=");      Serial.print(tds);
  Serial.print(" DO=");       Serial.print(doVal);
  Serial.print(" Turb=");     Serial.println(turbidity);
}

// ── Sensor Readers ────────────────────────────────────────────
float readTemperature() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  return (t == -127.0) ? 0.0 : t;
}

float readPH() {
  // Average 10 readings for stability
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(PIN_PH); delay(10); }
  float voltage = (sum / 10.0) * (5.0 / 1023.0);
  // Typical pH sensor: pH = 3.5 * V + calibration_offset
  return 3.5 * voltage + PH_OFFSET;
}

float readTDS() {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(PIN_TDS); delay(5); }
  float voltage = (sum / 10.0) * (5.0 / 1023.0);
  // TDS ppm = voltage * factor * 1000
  float tds = (133.42 * voltage * voltage * voltage
             - 255.86 * voltage * voltage
             + 857.39 * voltage) * TDS_FACTOR;
  return tds;
}

float readDO() {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(PIN_DO); delay(5); }
  float voltage = (sum / 10.0) * (5.0 / 1023.0);
  // Adjust formula based on your DO sensor datasheet
  return voltage * DO_FACTOR;
}

float readTurbidity() {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(PIN_TURBIDITY); delay(5); }
  float voltage = (sum / 10.0) * (5.0 / 1023.0);
  // NTU = -1120.4*V^2 + 5742.3*V - 4352.9 (example curve)
  float ntu = -1120.4 * voltage * voltage + 5742.3 * voltage - 4352.9;
  return max(0.0f, ntu);
}

// ── MQTT Publish via ESP ──────────────────────────────────────
void publishSensor(const char* sensor, float value) {
  StaticJsonDocument<128> doc;
  doc["sensor"]    = sensor;
  doc["value"]     = round(value * 100.0) / 100.0;
  doc["timestamp"] = millis();
  String out;
  serializeJson(doc, out);
  // Format: MQTT_PUB:<topic>:<payload>\n
  espSerial.print("MQTT_PUB:aquaponics/sensors/");
  espSerial.print(sensor);
  espSerial.print(":");
  espSerial.println(out);
}

// ── Handle incoming commands ──────────────────────────────────
void handleCommand(String json) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) { Serial.println("[CMD] Parse error"); return; }

  const char* device = doc["device"];
  const char* action = doc["action"];
  bool on = (strcmp(action, "on") == 0 || strcmp(action, "enable") == 0);

  if (strcmp(device, "waterPump") == 0) setActuator("waterPump", PIN_PUMP_WATER, on);
  else if (strcmp(device, "aerator") == 0) setActuator("aerator", PIN_AERATOR, on);
  else if (strcmp(device, "heater") == 0)  setActuator("heater",  PIN_HEATER, on);
}

void setActuator(const char* name, int pin, bool on) {
  // Active LOW relay: LOW = ON, HIGH = OFF
  digitalWrite(pin, on ? LOW : HIGH);
  Serial.print("[ACT] "); Serial.print(name);
  Serial.println(on ? " → ON" : " → OFF");
}
