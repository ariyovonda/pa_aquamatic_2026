/*
 * ============================================================
 *  AquaMonitor – ESP8266/ESP32 WiFi + MQTT Bridge
 * ============================================================
 *  Menerima data dari ATmega via Serial, publish ke MQTT broker
 *  Menerima perintah dari MQTT, teruskan ke ATmega via Serial
 * ============================================================
 *  Library yang dibutuhkan:
 *  - PubSubClient  (https://github.com/knolleary/pubsubclient)
 *  - ArduinoJson v6
 * ============================================================
 */

#include <ESP8266WiFi.h>    // Ganti dengan <WiFi.h> untuk ESP32
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ── WiFi Config ───────────────────────────────────────────────
const char* WIFI_SSID     = "NAMA_WIFI_ANDA";
const char* WIFI_PASSWORD = "PASSWORD_WIFI_ANDA";

// ── MQTT Config ───────────────────────────────────────────────
const char* MQTT_BROKER   = "192.168.1.100";  // IP komputer Node-RED
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "";               // kosongkan jika tanpa auth
const char* MQTT_PASS     = "";
const char* MQTT_CLIENT   = "esp-aquaponics";

// Subscribe untuk perintah aktuator
const char* TOPICS_SUB[]  = {
  "aquaponics/actuators/waterPump/command",
  "aquaponics/actuators/aerator/command",
  "aquaponics/actuators/heater/command"
};

// ── Objects ───────────────────────────────────────────────────
WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);
String        serialBuffer = "";

void setup() {
  Serial.begin(9600);
  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  // Read from ATmega
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processSerialLine(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

// ── WiFi ──────────────────────────────────────────────────────
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    tries++;
  }
}

// ── MQTT Reconnect ────────────────────────────────────────────
void reconnectMQTT() {
  int tries = 0;
  while (!mqttClient.connected() && tries < 5) {
    bool ok = strlen(MQTT_USER) > 0
      ? mqttClient.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS)
      : mqttClient.connect(MQTT_CLIENT);
    if (ok) {
      for (int i = 0; i < 3; i++) {
        mqttClient.subscribe(TOPICS_SUB[i], 1);
      }
    }
    tries++;
    delay(2000);
  }
}

// ── Serial from ATmega → MQTT publish ────────────────────────
void processSerialLine(String line) {
  line.trim();
  if (!line.startsWith("MQTT_PUB:")) return;
  String rest = line.substring(9);  // remove "MQTT_PUB:"
  int sep = rest.indexOf(':');
  if (sep < 0) return;
  String topic   = rest.substring(0, sep);
  String payload = rest.substring(sep + 1);
  mqttClient.publish(topic.c_str(), payload.c_str(), false);
}

// ── MQTT → Serial to ATmega ───────────────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  // Forward raw JSON to ATmega
  Serial.println(msg);
}
