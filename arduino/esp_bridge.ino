/*
 * ESP8266 AquaMonitor bridge
 * Arduino -> ESP: <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
 * Arduino -> ESP: @waterPump,aerator,heater,phPumpDown,phPumpUp,nutritionPump,buzzer
 * ESP -> MQTT:   aquaponics/sensors/<sensor>
 * MQTT -> ESP:   aquaponics/actuators/<device>/command
 * ESP -> Arduino JSON line: {"device":"heater","action":"on"}
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// Isi sesuai hotspot/router yang dipakai ESP dan laptop.
// Jangan gunakan localhost: pada ESP, localhost berarti ESP itu sendiri.
const char* WIFI_SSID = "pon pon";
const char* WIFI_PASSWORD = "11223344";
const char* MQTT_BROKER = "172.20.10.3"; // IPv4 laptop saat hotspot "pon pon" digunakan
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const char* MQTT_CLIENT_ID = "espvonda-aquamonitor";

const char* COMMAND_TOPICS[] = {
  "aquaponics/actuators/waterPump/command",
  "aquaponics/actuators/aerator/command",
  "aquaponics/actuators/heater/command",
  "aquaponics/actuators/phPumpDown/command",
  "aquaponics/actuators/phPumpUp/command",
  "aquaponics/actuators/nutritionPump/command",
  "aquaponics/actuators/buzzer/command"
};
const size_t COMMAND_TOPIC_COUNT = sizeof(COMMAND_TOPICS) / sizeof(COMMAND_TOPICS[0]);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
// RX, TX di sisi ESP: GPIO14 (label D5) menerima Arduino D3;
// GPIO12 (label D6) mengirim ke Arduino D4. Generic ESP8266 tidak
// mendefinisikan alias D5/D6, sehingga nomor GPIO dipakai langsung.
SoftwareSerial bridgeSerial(14, 12);
String uartLine;
unsigned long lastMqttAttemptAt = 0;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000UL;

void connectWiFi();
void connectMqtt();
void onMqttMessage(char* topic, byte* payload, unsigned int length);
void processArduinoLine(String line);
void publishSensor(const char* sensor, float value, const char* unit);
void processCsvFrame(String line);
void processActuatorStateFrame(String line);
String actuatorFromTopic(const String& topic);
String actionFromPayload(byte* payload, unsigned int length);

void setup() {
  // Serial USB untuk debug/Serial Monitor.
  Serial.begin(9600);
  // Jalur fisik komunikasi data Arduino <-> ESP pada D5/D6.
  bridgeSerial.begin(9600);
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(384);
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMqtt();
  mqttClient.loop();

  while (bridgeSerial.available()) {
    const char c = static_cast<char>(bridgeSerial.read());
    if (c == '\n') {
      processArduinoLine(uartLine);
      uartLine = "";
    } else if (uartLine.length() < 240) {
      uartLine += c;
    } else {
      uartLine = ""; // drop malformed/oversized line
    }
  }

}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.println(F("[ESP] Connecting Wi-Fi..."));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.print(F("[ESP] Wi-Fi connected. IP: "));
  Serial.println(WiFi.localIP());
}

void connectMqtt() {
  if (mqttClient.connected()) return;

  const unsigned long now = millis();
  if (lastMqttAttemptAt && now - lastMqttAttemptAt < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttAttemptAt = now;

  const bool connected = strlen(MQTT_USER)
    ? mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)
    : mqttClient.connect(MQTT_CLIENT_ID);
  if (connected) {
    Serial.println(F("[ESP] MQTT connected."));
    for (size_t i = 0; i < COMMAND_TOPIC_COUNT; i++) {
      mqttClient.subscribe(COMMAND_TOPICS[i], 1);
      Serial.print(F("[ESP] Subscribed command topic: "));
      Serial.println(COMMAND_TOPICS[i]);
    }
  } else {
    Serial.print(F("[ESP] MQTT failed, state="));
    Serial.println(mqttClient.state());
  }
}

void processArduinoLine(String line) {
  line.trim();
  if (line.startsWith("<") && line.endsWith(">")) {
    processCsvFrame(line.substring(1, line.length() - 1));
    return;
  }

  if (line.startsWith("@")) {
    processActuatorStateFrame(line.substring(1));
    return;
  }

  // Ignore all non-CSV diagnostic text from Arduino.
}

void processActuatorStateFrame(String csv) {
  static const char* devices[] = {
    "waterPump", "aerator", "heater", "phPumpDown", "phPumpUp", "nutritionPump", "buzzer"
  };
  int start = 0;
  for (size_t i = 0; i < 7; i++) {
    const int comma = csv.indexOf(',', start);
    const String field = comma >= 0 ? csv.substring(start, comma) : csv.substring(start);
    if (!field.length() || (i < 6 && comma < 0)) return;
    if (!mqttClient.connected()) return;

    StaticJsonDocument<64> state;
    state["running"] = field.toInt() != 0;
    char payload[64];
    const size_t length = serializeJson(state, payload, sizeof(payload));
    char topic[80];
    snprintf(topic, sizeof(topic), "aquaponics/actuators/%s/state", devices[i]);
    mqttClient.publish(topic, reinterpret_cast<const uint8_t*>(payload), length, false);
    start = comma + 1;
  }
}

void processCsvFrame(String csv) {
  float values[9];
  int start = 0;
  for (int i = 0; i < 9; i++) {
    const int comma = csv.indexOf(',', start);
    const String field = comma >= 0 ? csv.substring(start, comma) : csv.substring(start);
    if (!field.length()) return;
    values[i] = field.toFloat();
    if (i < 8 && comma < 0) return;
    start = comma + 1;
  }

  if (!mqttClient.connected()) {
    Serial.println(F("[ESP] Sensor frame received; MQTT is offline."));
    return;
  }

  publishSensor("temperature", values[0], "C");
  publishSensor("ph", values[1], "pH");
  publishSensor("tds", values[2], "ppm");
  publishSensor("turbidity", values[3], "NTU");
  publishSensor("do", values[4], "mg/L");
  Serial.println(F("[ESP] Sensor frame published to MQTT."));
}

void publishSensor(const char* sensor, float value, const char* unit) {
  StaticJsonDocument<128> message;
  message["value"] = value;
  message["unit"] = unit;
  message["status"] = "normal";
  char payload[128];
  const size_t length = serializeJson(message, payload, sizeof(payload));
  char topic[64];
  snprintf(topic, sizeof(topic), "aquaponics/sensors/%s", sensor);
  // Overload PubSubClient pada versi yang terpasang menerima byte buffer.
  mqttClient.publish(topic, reinterpret_cast<const uint8_t*>(payload), length, false);
}

String actuatorFromTopic(const String& topic) {
  const int first = topic.indexOf("actuators/");
  if (first < 0) return "";
  const int start = first + 10;
  const int end = topic.indexOf('/', start);
  return end < 0 ? "" : topic.substring(start, end);
}

String actionFromPayload(byte* payload, unsigned int length) {
  StaticJsonDocument<128> incoming;
  if (!deserializeJson(incoming, payload, length)) {
    const char* action = incoming["action"] | "";
    if (strlen(action)) return String(action);
  }

  String raw;
  for (unsigned int i = 0; i < length; i++) raw += static_cast<char>(payload[i]);
  raw.trim();
  raw.replace("\"", "");
  raw.toLowerCase();
  if (raw == "on" || raw == "enable") return "on";
  if (raw == "off" || raw == "disable") return "off";
  return "";
}

void onMqttMessage(char* rawTopic, byte* rawPayload, unsigned int length) {
  const String device = actuatorFromTopic(String(rawTopic));
  const String action = actionFromPayload(rawPayload, length);
  if (!device.length() || !action.length()) {
    Serial.print(F("[ESP] Invalid actuator command on topic: "));
    Serial.println(rawTopic);
    return;
  }

  StaticJsonDocument<128> command;
  command["device"] = device;
  command["action"] = action;
  serializeJson(command, bridgeSerial);
  bridgeSerial.println();

  Serial.print(F("[ESP] RPC forwarded to Arduino: "));
  Serial.print(device);
  Serial.print(F(" -> "));
  Serial.println(action);
}
