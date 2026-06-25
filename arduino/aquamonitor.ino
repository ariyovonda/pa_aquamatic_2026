// Final Arduino firmware: sensor acquisition, relay safety, and ESP commands.
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

SoftwareSerial espSerial(4, 3);

#define PHSENSORPIN   A0
#define ONE_WIRE_BUS  2
#define TdsSensorPin  A1
#define TURBIDITY_PIN A3
#define DO_PIN        A5

#define PUMP_PIN      12
#define AERATOR_PIN   11
#define HEATER_PIN    9
#define PH_ASAM_PUMP  8
#define PH_BASA_PUMP  7
#define TDS_PUMP      13
#define BUZZER_PIN    10

const float TDS_CAL_FACTOR = 196.11f;
const float K_CELL         = 1.00;
#define VREF            5.0
#define SCOUNT          30
#define DOSE_DURATION_MS 1000UL
#define DOSE_LOCKOUT_MS  30000UL

#define JERNIH_MIN   630
#define TURB_SAMPLES 30

// Mode final stabil:
// - Threshold user tetap diproses oleh Node-RED/RTDB dan masuk sebagai command.
// - Arduino bertugas sebagai pengaman relay: semua aktuator dibuat pulse/cycle,
//   tidak ada relay ON permanen.
// - Arduino menunda publish ke ESP saat relay sedang ON untuk mengurangi noise.
const bool ENABLE_REMOTE_ACTUATOR_COMMANDS = true;
const bool ENABLE_LOCAL_FALLBACK_THRESHOLD = false; // true hanya jika Node-RED dimatikan
const bool RELAY_SAFE_TEST_MODE = false;
const bool ESP_SAFE_TEST_MODE = false;
const bool LOOP_CHECKPOINT_DEBUG = false;
const float TEMP_HEATER_ON_C  = 25.0;
const float TEMP_HEATER_OFF_C = 30.0;
const float PH_LOW_LIMIT      = 6.0;
const float PH_HIGH_LIMIT     = 8.0;
const float TDS_LOW_LIMIT     = 200.0;
const float TDS_HIGH_ALARM    = 500.0;

#define TWO_POINT_CALIBRATION 0
#define DO_CAL1_V   1215.82
#define DO_CAL1_T   25
#define DO_CAL2_V   0.0
#define DO_CAL2_T   0.0
#define DO_SAMPLES  10

const uint16_t DO_Table[41] PROGMEM = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080,  9860,  9660,  9460,  9270,
   9080,  8900,  8730,  8570,  8410,  8250,  8110,  7960,  7820,  7690,
   7560,  7430,  7300,  7180,  7070,  6950,  6840,  6730,  6630,  6530,
   6410
};

#define FILTER_SIZE 10
float tdsBuffer[FILTER_SIZE];
int   indexTds = 0;
bool  tdsFilterReady = false;

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage  = 0;
float tdsValue        = 0;

const int   ROLLING_SIZE     = 30;
const float OUTLIER_THRESH_V = 0.150;
const int   MAX_REJECT_COUNT = 15;

float rollingBuf[ROLLING_SIZE];
int   rollingIdx  = 0;
int   rejectCount = 0;
float voltFinalPH = 0;

int bufferarr[10], temp_sort;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const float VPH7     = 2.521;
const float VPH4     = 3.064;
const float VOLT_MIN = 0.5;
const float VOLT_MAX = 4.5;
float phact, m, b;

bool heaterON = false;
bool pumpEnabled = true;
bool pumpON = false;
bool aeratorEnabled = true;
bool aeratorON = false;
bool phAsamPumpON = false;
bool phBasaPumpON = false;
bool tdsPumpON = false;
bool buzzerON = false;

// Polaritas diambil dari sketch asli Anda. Uji relay tanpa cairan sebelum dipakai.
const uint8_t PUMP_ON_LEVEL = HIGH;
const uint8_t AERATOR_ON_LEVEL = HIGH;
const uint8_t HEATER_ON_LEVEL = HIGH;
const uint8_t PH_ASAM_ON_LEVEL = LOW;
const uint8_t PH_BASA_ON_LEVEL = LOW;
const uint8_t TDS_ON_LEVEL = HIGH;
const uint8_t BUZZER_ON_LEVEL = HIGH;

unsigned long phAsamDoseStartedAt = 0;
unsigned long phBasaDoseStartedAt = 0;
unsigned long tdsDoseStartedAt = 0;
unsigned long phAsamLastDoseAt = 0;
unsigned long phBasaLastDoseAt = 0;
unsigned long tdsLastDoseAt = 0;
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 5000UL;
unsigned long pumpStateChangedAt = 0;
const unsigned long PUMP_ON_DURATION_MS = 2000UL;
const unsigned long PUMP_LOCKOUT_MS = 20000UL;
unsigned long aeratorStateChangedAt = 0;
const unsigned long AERATOR_ON_DURATION_MS = 5000UL;
const unsigned long AERATOR_LOCKOUT_MS = 20000UL;
unsigned long heaterPulseStartedAt = 0;
unsigned long heaterLastPulseAt = 0;
const unsigned long HEATER_PULSE_DURATION_MS = 5000UL;
const unsigned long HEATER_LOCKOUT_MS = 20000UL;
unsigned long buzzerPulseStartedAt = 0;
unsigned long buzzerLastPulseAt = 0;
const unsigned long BUZZER_PULSE_DURATION_MS = 1000UL;
const unsigned long BUZZER_LOCKOUT_MS = 5000UL;
const unsigned long COMMAND_STARTUP_DELAY_MS = 8000UL;
bool pendingPhAsamDose = false;
bool pendingPhBasaDose = false;
bool pendingTdsDose = false;
bool pendingHeaterPulse = false;
bool pendingBuzzerPulse = false;
bool pendingActuatorStatePublish = false;
String espCommandBuffer;
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 1000UL;
uint8_t debugLoopCount = 0;

float readFilteredVoltage() {
  for (int i = 0; i < 10; i++) {
    bufferarr[i] = analogRead(PHSENSORPIN);
    delay(20);
  }
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (bufferarr[i] > bufferarr[j]) {
        temp_sort    = bufferarr[i];
        bufferarr[i] = bufferarr[j];
        bufferarr[j] = temp_sort;
      }
    }
  }
  unsigned long avgval_ph = 0;
  for (int i = 2; i < 8; i++) avgval_ph += bufferarr[i];
  return (float)avgval_ph * 5.0 / 1023.0 / 6.0;
}

float getRollingAverage() {
  float sum = 0;
  for (int i = 0; i < ROLLING_SIZE; i++) sum += rollingBuf[i];
  return sum / ROLLING_SIZE;
}

void autoResetRollingBuffer(float currentVolt) {
  for (int i = 0; i < ROLLING_SIZE; i++) rollingBuf[i] = currentVolt;
  rollingIdx  = 0;
  rejectCount = 0;
  Serial.println(F(">> AUTO-RESET: Rolling buffer diperbarui."));
}

float getStableTDS(float newValue) {
  // Jangan rata-ratakan pembacaan pertama dengan array nol. Nilai nol pada
  // boot dapat membuat Node-RED salah menganggap TDS rendah dan menyalakan
  // pompa nutrisi sebelum sensor benar-benar terbaca.
  if (!tdsFilterReady) {
    for (int i = 0; i < FILTER_SIZE; i++) tdsBuffer[i] = newValue;
    indexTds = 0;
    tdsFilterReady = true;
    return newValue;
  }
  tdsBuffer[indexTds] = newValue;
  indexTds = (indexTds + 1) % FILTER_SIZE;
  float sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) sum += tdsBuffer[i];
  return sum / FILTER_SIZE;
}

int readTurbADC() {
  long sum = 0;
  for (int i = 0; i < TURB_SAMPLES; i++) {
    sum += analogRead(TURBIDITY_PIN);
    delay(5);
  }
  return constrain(sum / TURB_SAMPLES, 0, 1023);
}

float voltageToNTU(float voltage) {
  if (voltage > 4.2) return 0;
  if (voltage < 2.5) return 3000;
  float ntu = -1120.4 * pow(voltage, 2) + 5742.3 * voltage - 4353.8;
  if (ntu < 0) ntu = 0;
  return ntu;
}

float readDOVoltage() {
  long total = 0;
  for (int i = 0; i < DO_SAMPLES; i++) {
    total += analogRead(DO_PIN);
    delay(5);
  }
  float avgADC = total / (float)DO_SAMPLES;
  return avgADC * (VREF / 1023.0);
}

float voltageToDO(float voltageVolt, uint8_t temperature) {
  temperature = constrain(temperature, 0, 40);
  float voltage_mV = voltageVolt * 1000.0;
  uint16_t V_saturation;
#if TWO_POINT_CALIBRATION == 0
  V_saturation = (uint32_t)DO_CAL1_V + (uint32_t)35 * temperature - (uint32_t)DO_CAL1_T * 35;
#else
  V_saturation = ((int16_t)temperature - DO_CAL2_T) * ((int16_t)DO_CAL1_V - DO_CAL2_V) / ((int16_t)DO_CAL1_T - DO_CAL2_T) + DO_CAL2_V;
#endif
  return (voltage_mV * pgm_read_word(&DO_Table[temperature])) / V_saturation;
}

void setup() {
  Serial.begin(9600);
  if (!ESP_SAFE_TEST_MODE) espSerial.begin(9600);
  sensors.begin();

  pinMode(PUMP_PIN,     OUTPUT);
  pinMode(AERATOR_PIN,  OUTPUT);
  pinMode(HEATER_PIN,   OUTPUT);
  pinMode(BUZZER_PIN,   OUTPUT);
  pinMode(PH_ASAM_PUMP, OUTPUT);
  pinMode(PH_BASA_PUMP, OUTPUT);
  pinMode(TDS_PUMP,     OUTPUT);

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
  setRelay(TDS_PUMP, TDS_ON_LEVEL, false);

  // Prime buffer ADC TDS sebelum pembacaan pertama. Tanpa ini, sebagian besar
  // buffer berisi 0 dan nilai median awal dapat menjadi 0 ppm.
  for (int i = 0; i < SCOUNT; i++) {
    analogBuffer[i] = analogRead(TdsSensorPin);
    delay(10);
  }
  analogBufferIndex = 0;

  pumpStateChangedAt = millis();
  aeratorStateChangedAt = millis();
  if (RELAY_SAFE_TEST_MODE) {
    pumpEnabled = false;
    pumpON = false;
    aeratorEnabled = false;
    aeratorON = false;
  }

  m = (7.0 - 4.01) / (VPH7 - VPH4);
  b = 7.0 - (m * VPH7);

  Serial.println(F("===== SISTEM MONITORING AIR ====="));
  Serial.print(F("Kalibrasi pH -> m: ")); Serial.print(m, 4);
  Serial.print(F(" | b: ")); Serial.println(b, 4);
  Serial.println(F("=================================="));

  if (m >= 0) {
    Serial.println(F("ERROR: Slope positif! Periksa nilai VPH7 dan VPH4."));
    while (true);
  }

  for (int i = 0; i < ROLLING_SIZE; i++) {
    rollingBuf[i] = analogRead(PHSENSORPIN) * 5.0 / 1023.0;
    delay(50);
  }
  voltFinalPH = getRollingAverage();
  phact = (m * voltFinalPH) + b;

  Serial.println(F("[SUKSES] Sistem siap. Memulai pengukuran..."));
}

void loop() {
  const bool debugThisLoop = LOOP_CHECKPOINT_DEBUG && debugLoopCount < 3;
  if (debugThisLoop) { Serial.print(F("[DBG] loop ")); Serial.println(debugLoopCount); }

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && !RELAY_SAFE_TEST_MODE && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }
  if (debugThisLoop) Serial.println(F("[DBG] after command"));
  updateDoseTimers();
  updatePulseTimers();
  if (debugThisLoop) Serial.println(F("[DBG] after timers"));
  if (!RELAY_SAFE_TEST_MODE) {
    processPendingPulses();
    updateWaterPumpCycle();
    updateAeratorCycle();
  }
  if (debugThisLoop) Serial.println(F("[DBG] after actuator scheduler"));

  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'R' || cmd == 'r') {
      float v_init = readFilteredVoltage();
      autoResetRollingBuffer(v_init);
      Serial.println(F(">> MANUAL RESET diterima."));
    }
  }

  sensors.requestTemperatures();
  if (debugThisLoop) Serial.println(F("[DBG] after requestTemperatures"));
  float tempTerbaca = sensors.getTempCByIndex(0);
  if (debugThisLoop) Serial.println(F("[DBG] after getTemp"));
  float suhu = 25.0;
  if (tempTerbaca != DEVICE_DISCONNECTED_C && tempTerbaca > -50.0 && tempTerbaca < 100.0) {
    suhu = tempTerbaca - 0.9f;
  }

  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;
    float voltPH = readFilteredVoltage();
    if (voltPH >= VOLT_MIN && voltPH <= VOLT_MAX) {
      float avgPH   = getRollingAverage();
      float deltaPH = abs(voltPH - avgPH);
      if (deltaPH > OUTLIER_THRESH_V) {
        rejectCount++;
        Serial.print(F(">> PH WARNING: Outlier ditolak ["));
        Serial.print(rejectCount); Serial.print(F("/"));
        Serial.print(MAX_REJECT_COUNT); Serial.println("]");
        if (rejectCount >= MAX_REJECT_COUNT) autoResetRollingBuffer(voltPH);
      } else {
        rejectCount = 0;
        rollingBuf[rollingIdx] = voltPH;
        rollingIdx = (rollingIdx + 1) % ROLLING_SIZE;
        voltFinalPH = getRollingAverage();
      }
    } else {
      Serial.println(F(">> PH CRITICAL: Tegangan pH di luar batas fisik sensor!"));
    }
  }
  if (debugThisLoop) Serial.println(F("[DBG] after pH"));

  phact = (m * voltFinalPH) + b;
  if (phact < 0.0)  phact = 0.0;
  if (phact > 14.0) phact = 14.0;

  analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
  analogBufferIndex++;
  if (analogBufferIndex == SCOUNT) analogBufferIndex = 0;

  for (int i = 0; i < SCOUNT; i++) analogBufferTemp[i] = analogBuffer[i];
  for (int i = 0; i < SCOUNT - 1; i++) {
    for (int j = i + 1; j < SCOUNT; j++) {
      if (analogBufferTemp[i] > analogBufferTemp[j]) {
        int tmp = analogBufferTemp[i];
        analogBufferTemp[i] = analogBufferTemp[j];
        analogBufferTemp[j] = tmp;
      }
    }
  }
  int medianValue  = analogBufferTemp[SCOUNT / 2];
  averageVoltage   = medianValue * VREF / 1023.0;
  float ppmKasar   = averageVoltage * TDS_CAL_FACTOR * K_CELL;
  float kompensasi = 1.0 + 0.02 * (suhu - 25.0);
  tdsValue         = ppmKasar / kompensasi;
  if (tdsValue < 0) tdsValue = 0;
  tdsValue = getStableTDS(tdsValue);
  if (debugThisLoop) Serial.println(F("[DBG] after TDS"));

  int   turbADC     = readTurbADC();
  float turbVoltage = (turbADC / 1023.0) * 5.0;
  float ntu         = voltageToNTU(turbVoltage);
  if (debugThisLoop) Serial.println(F("[DBG] after turbidity"));

  float voltDO  = readDOVoltage();
  float nilaiDO = voltageToDO(voltDO, (uint8_t)suhu);
  if (nilaiDO < 0) nilaiDO = 0;
  if (debugThisLoop) Serial.println(F("[DBG] after DO"));

  applyAutomaticControl(suhu, phact, tdsValue, turbADC);
  if (debugThisLoop) Serial.println(F("[DBG] after control"));

  // Arduino memutuskan aktuator secara otomatis dari data sensor lokal.
  // Node-RED/RTDB/web hanya menerima telemetry dan status.
  setRelay(PUMP_PIN, PUMP_ON_LEVEL, pumpON);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, aeratorON);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, heaterON);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, buzzerON);

  Serial.println(F("----------------------------------"));
  Serial.print(F("Suhu     : ")); Serial.print(suhu, 1);           Serial.println(F(" C"));
  Serial.print(F("pH       : ")); Serial.print(phact, 2);
  Serial.print(F(" (Teg: "));     Serial.print(voltFinalPH, 3);    Serial.println(F(" V)"));
  Serial.print(F("TDS      : ")); Serial.print(tdsValue, 1);       Serial.println(F(" ppm"));
  Serial.print(F("Teg. TDS : ")); Serial.print(averageVoltage, 4); Serial.println(F(" V"));
  Serial.print(F("Turb ADC : ")); Serial.print(turbADC);
  Serial.print(F(" | "));         Serial.print(turbVoltage, 2);
  Serial.print(F(" V | NTU: "));  Serial.print(ntu, 1);
  Serial.print(F(" | "));
  Serial.println(turbADC > JERNIH_MIN ? F("AIR JERNIH") : F("AIR KERUH"));
  Serial.print(F("DO       : ")); Serial.print(nilaiDO, 2);        Serial.println(F(" mg/L"));
  Serial.print(F("Teg. DO  : ")); Serial.print(voltDO, 4);         Serial.println(F(" V"));

  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.print(F("Pump         : "));
  if (!pumpEnabled) Serial.println(F("OFF (manual disabled)"));
  else Serial.println(pumpON ? F("ON (2 sec cycle)") : F("OFF (20 sec lockout)"));
  Serial.print(F("Aerator      : "));
  if (!aeratorEnabled) Serial.println(F("OFF (manual disabled)"));
  else Serial.println(aeratorON ? F("ON (5 sec cycle)") : F("OFF (20 sec lockout)"));
  Serial.print(F("Heater       : ")); Serial.println(heaterON     ? F("ON") : F("OFF"));
  Serial.print(F("pH Asam Pump : ")); printDoseStatus(phAsamPumpON, phact > PH_HIGH_LIMIT, phAsamLastDoseAt);
  Serial.print(F("pH Basa Pump : ")); printDoseStatus(phBasaPumpON, phact < PH_LOW_LIMIT, phBasaLastDoseAt);
  Serial.print(F("TDS Pump     : ")); printDoseStatus(tdsPumpON, tdsValue < TDS_LOW_LIMIT, tdsLastDoseAt);
  Serial.print(F("Buzzer       : ")); Serial.println(buzzerON     ? F("ON") : F("OFF"));
  Serial.println(F("==========================="));
  if (debugThisLoop) debugLoopCount++;

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      espSerial.print("<");
      espSerial.print(suhu, 1);       espSerial.print(",");
      espSerial.print(phact, 2);      espSerial.print(",");
      espSerial.print(tdsValue, 1);   espSerial.print(",");
      espSerial.print(ntu, 1);        espSerial.print(",");
      espSerial.print(nilaiDO, 2);    espSerial.print(",");
      espSerial.print(heaterON);      espSerial.print(",");
      espSerial.print(phAsamPumpON);  espSerial.print(",");
      espSerial.print(phBasaPumpON);  espSerial.print(",");
      espSerial.print(tdsPumpON);
      espSerial.println(">");
    }
  }

  delay(1000);
}

void setRelay(uint8_t pin, uint8_t onLevel, bool on) {
  digitalWrite(pin, on ? onLevel : (onLevel == HIGH ? LOW : HIGH));
}

void printDoseStatus(bool active, bool conditionActive, unsigned long lastDoseAt) {
  if (active) {
    Serial.println(F("ON (dosing 1 sec)"));
    return;
  }

  if (!ENABLE_LOCAL_FALLBACK_THRESHOLD) {
    Serial.println(conditionActive ? F("OFF (waiting threshold command)") : F("OFF"));
    return;
  }

  const unsigned long now = millis();
  const bool inLockout = lastDoseAt > 0 && now - lastDoseAt < DOSE_LOCKOUT_MS;
  if (conditionActive && inLockout) {
    Serial.print(F("OFF (mixing lockout "));
    Serial.print((DOSE_LOCKOUT_MS - (now - lastDoseAt)) / 1000);
    Serial.println(F(" sec)"));
    return;
  }

  if (conditionActive) {
    Serial.println(F("OFF (waiting next dose)"));
    return;
  }

  Serial.println(F("OFF"));
}

void applyAutomaticControl(float suhu, float ph, float tds, int turbADC) {
  if (RELAY_SAFE_TEST_MODE) {
    pumpEnabled = false;
    pumpON = false;
    aeratorEnabled = false;
    aeratorON = false;
    heaterON = false;
    phAsamPumpON = false;
    phBasaPumpON = false;
    tdsPumpON = false;
    buzzerON = false;
    return;
  }

  // Fallback lokal sengaja default OFF supaya nilai threshold user di RTDB
  // menjadi sumber keputusan utama. Jika Node-RED tidak dipakai, aktifkan
  // ENABLE_LOCAL_FALLBACK_THRESHOLD.
  if (ENABLE_LOCAL_FALLBACK_THRESHOLD) {
    if (suhu < TEMP_HEATER_ON_C) pendingHeaterPulse = true;
    if (ph < PH_LOW_LIMIT) pendingPhBasaDose = true;
    else if (ph > PH_HIGH_LIMIT) pendingPhAsamDose = true;
    if (tds < TDS_LOW_LIMIT) pendingTdsDose = true;
    if (tds > TDS_HIGH_ALARM || turbADC <= JERNIH_MIN) pendingBuzzerPulse = true;
  }
}

bool anyDoseActive() {
  return phAsamPumpON || phBasaPumpON || tdsPumpON;
}

bool hasActiveRelay() {
  return pumpON || aeratorON || heaterON || buzzerON || anyDoseActive();
}

void requestActuatorStatePublish() {
  pendingActuatorStatePublish = true;
}

void startDose(const char* device, bool& active, unsigned long& startedAt, unsigned long& lastDoseAt, uint8_t pin, uint8_t onLevel) {
  const unsigned long now = millis();
  if (active) return;
  if (hasActiveRelay()) return;
  if (lastDoseAt > 0 && now - lastDoseAt < DOSE_LOCKOUT_MS) return;
  active = true;
  startedAt = now;
  lastDoseAt = now;
  setRelay(pin, onLevel, true);
  Serial.print(F("[DOSE] ")); Serial.print(device); Serial.println(F(" ON for 1 second"));
  requestActuatorStatePublish();
}

void stopDose(bool& active, uint8_t pin, uint8_t onLevel) {
  active = false;
  setRelay(pin, onLevel, false);
}

void startPulse(const char* device, bool& active, unsigned long& startedAt, unsigned long& lastPulseAt, unsigned long lockoutMs, uint8_t pin, uint8_t onLevel) {
  const unsigned long now = millis();
  if (active || hasActiveRelay()) return;
  if (lastPulseAt > 0 && now - lastPulseAt < lockoutMs) return;
  active = true;
  startedAt = now;
  lastPulseAt = now;
  setRelay(pin, onLevel, true);
  Serial.print(F("[PULSE] ")); Serial.print(device); Serial.println(F(" ON"));
  requestActuatorStatePublish();
}

void stopPulse(bool& active, uint8_t pin, uint8_t onLevel) {
  active = false;
  setRelay(pin, onLevel, false);
}

void updateDoseTimers() {
  const unsigned long now = millis();
  if (phAsamPumpON && now - phAsamDoseStartedAt >= DOSE_DURATION_MS) { stopDose(phAsamPumpON, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL); requestActuatorStatePublish(); }
  if (phBasaPumpON && now - phBasaDoseStartedAt >= DOSE_DURATION_MS) { stopDose(phBasaPumpON, PH_BASA_PUMP, PH_BASA_ON_LEVEL); requestActuatorStatePublish(); }
  if (tdsPumpON && now - tdsDoseStartedAt >= DOSE_DURATION_MS) { stopDose(tdsPumpON, TDS_PUMP, TDS_ON_LEVEL); requestActuatorStatePublish(); }
}

void updatePulseTimers() {
  const unsigned long now = millis();
  if (heaterON && now - heaterPulseStartedAt >= HEATER_PULSE_DURATION_MS) { stopPulse(heaterON, HEATER_PIN, HEATER_ON_LEVEL); requestActuatorStatePublish(); }
  if (buzzerON && now - buzzerPulseStartedAt >= BUZZER_PULSE_DURATION_MS) { stopPulse(buzzerON, BUZZER_PIN, BUZZER_ON_LEVEL); requestActuatorStatePublish(); }
}

void processPendingPulses() {
  if (hasActiveRelay()) return;

  if (pendingPhAsamDose) {
    pendingPhAsamDose = false;
    startDose("phPumpDown", phAsamPumpON, phAsamDoseStartedAt, phAsamLastDoseAt, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL);
    return;
  }
  if (pendingPhBasaDose) {
    pendingPhBasaDose = false;
    startDose("phPumpUp", phBasaPumpON, phBasaDoseStartedAt, phBasaLastDoseAt, PH_BASA_PUMP, PH_BASA_ON_LEVEL);
    return;
  }
  if (pendingTdsDose) {
    pendingTdsDose = false;
    startDose("nutritionPump", tdsPumpON, tdsDoseStartedAt, tdsLastDoseAt, TDS_PUMP, TDS_ON_LEVEL);
    return;
  }
  if (pendingHeaterPulse) {
    pendingHeaterPulse = false;
    startPulse("heater", heaterON, heaterPulseStartedAt, heaterLastPulseAt, HEATER_LOCKOUT_MS, HEATER_PIN, HEATER_ON_LEVEL);
    return;
  }
  if (pendingBuzzerPulse) {
    pendingBuzzerPulse = false;
    startPulse("buzzer", buzzerON, buzzerPulseStartedAt, buzzerLastPulseAt, BUZZER_LOCKOUT_MS, BUZZER_PIN, BUZZER_ON_LEVEL);
    return;
  }
}

// Water pump: ON 2 detik, lalu OFF/lockout 20 detik, berulang selama
// pumpEnabled bernilai true. Perintah OFF membatalkan siklus secara langsung.
void updateWaterPumpCycle() {
  const unsigned long now = millis();

  if (!pumpEnabled) {
    if (pumpON) {
      pumpON = false;
      setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
      requestActuatorStatePublish();
    }
    return;
  }

  if (pumpON) {
    if (now - pumpStateChangedAt < PUMP_ON_DURATION_MS) return;
    pumpON = false;
    pumpStateChangedAt = now;
    setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
    requestActuatorStatePublish();
    return;
  }

  if (now - pumpStateChangedAt < PUMP_LOCKOUT_MS) return;
  if (hasActiveRelay()) return;
  pumpON = true;
  pumpStateChangedAt = now;
  setRelay(PUMP_PIN, PUMP_ON_LEVEL, true);
  requestActuatorStatePublish();
}

void updateAeratorCycle() {
  const unsigned long now = millis();

  if (!aeratorEnabled) {
    if (aeratorON) {
      aeratorON = false;
      setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
      requestActuatorStatePublish();
    }
    return;
  }

  if (aeratorON) {
    if (now - aeratorStateChangedAt < AERATOR_ON_DURATION_MS) return;
    aeratorON = false;
    aeratorStateChangedAt = now;
    setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
    requestActuatorStatePublish();
    return;
  }

  if (now - aeratorStateChangedAt < AERATOR_LOCKOUT_MS) return;
  if (hasActiveRelay()) return;
  aeratorON = true;
  aeratorStateChangedAt = now;
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, true);
  requestActuatorStatePublish();
}

// Status relay dikirim terpisah agar web menampilkan keadaan fisik Arduino,
// termasuk fase ON 2 detik / lockout 20 detik pada water pump.
void publishActuatorState() {
  if (ESP_SAFE_TEST_MODE) return;
  if (hasActiveRelay()) {
    pendingActuatorStatePublish = true;
    return;
  }
  espSerial.print("@");
  espSerial.print(pumpON);       espSerial.print(",");
  espSerial.print(aeratorON);    espSerial.print(",");
  espSerial.print(heaterON);     espSerial.print(",");
  espSerial.print(phAsamPumpON); espSerial.print(",");
  espSerial.print(phBasaPumpON); espSerial.print(",");
  espSerial.print(tdsPumpON);    espSerial.print(",");
  espSerial.println(buzzerON);
  pendingActuatorStatePublish = false;
}

void receiveEspCommands() {
  uint8_t bytesRead = 0;
  while (espSerial.available() && bytesRead < 32) {
    bytesRead++;
    const char c = static_cast<char>(espSerial.read());
    if (c == '\n') {
      handleEspCommand(espCommandBuffer);
      espCommandBuffer = "";
    } else if (espCommandBuffer.length() < 127) {
      espCommandBuffer += c;
    } else {
      espCommandBuffer = "";
    }
  }
}

String getJsonField(const String& line, const char* key) {
  String pattern = "\"";
  pattern += key;
  pattern += "\"";
  int keyPos = line.indexOf(pattern);
  if (keyPos < 0) return "";
  int colon = line.indexOf(':', keyPos + pattern.length());
  if (colon < 0) return "";
  int firstQuote = line.indexOf('"', colon + 1);
  if (firstQuote < 0) return "";
  int secondQuote = line.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return "";
  String value = line.substring(firstQuote + 1, secondQuote);
  value.trim();
  return value;
}

void handleEspCommand(const String& line) {
  String cleanLine = line;
  cleanLine.trim();
  if (!cleanLine.length()) return;

  String deviceStr = getJsonField(cleanLine, "device");
  String actionStr = getJsonField(cleanLine, "action");
  actionStr.toLowerCase();
  if (!deviceStr.length() || !actionStr.length()) {
    Serial.print(F(">> ESP COMMAND INVALID: "));
    Serial.println(cleanLine);
    return;
  }
  const char* device = deviceStr.c_str();
  const char* action = actionStr.c_str();
  const bool on = !strcmp(action, "on") || !strcmp(action, "enable");
  const bool off = !strcmp(action, "off") || !strcmp(action, "disable");
  if (!strlen(device) || (!on && !off)) return;

  // Tahap threshold v2: command eksternal boleh masuk untuk semua aktuator.
  // Khusus dosing pump, fungsi startDose() tetap membatasi ON 1 detik dan
  // lockout 30 detik agar aman saat nanti sudah memakai cairan.

  Serial.print(F(">> ESP COMMAND: "));
  Serial.print(device);
  Serial.print(F(" -> "));
  Serial.println(action);

  if (!strcmp(device, "phPumpDown") && on) pendingPhAsamDose = true;
  else if (!strcmp(device, "phPumpUp") && on) pendingPhBasaDose = true;
  else if (!strcmp(device, "nutritionPump") && on) pendingTdsDose = true;
  else if (!strcmp(device, "phPumpDown") && off) { pendingPhAsamDose = false; stopDose(phAsamPumpON, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL); }
  else if (!strcmp(device, "phPumpUp") && off) { pendingPhBasaDose = false; stopDose(phBasaPumpON, PH_BASA_PUMP, PH_BASA_ON_LEVEL); }
  else if (!strcmp(device, "nutritionPump") && off) { pendingTdsDose = false; stopDose(tdsPumpON, TDS_PUMP, TDS_ON_LEVEL); }
  else if (!strcmp(device, "waterPump")) {
    // Water pump tidak boleh ON permanen. Command ON hanya mengaktifkan
    // siklus lokal 2 detik ON / 20 detik OFF. Command OFF mematikan siklus.
    if (on) {
      pumpEnabled = true;
      pumpStateChangedAt = millis() - PUMP_LOCKOUT_MS;
    } else {
      pumpEnabled = false;
      pumpON = false;
      pumpStateChangedAt = millis();
      setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
    }
  }
  else if (!strcmp(device, "aerator")) {
    aeratorEnabled = on;
    if (on) aeratorStateChangedAt = millis() - AERATOR_LOCKOUT_MS;
    if (off) { aeratorON = false; setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false); }
  }
  else if (!strcmp(device, "heater")) {
    if (on) pendingHeaterPulse = true;
    else { pendingHeaterPulse = false; stopPulse(heaterON, HEATER_PIN, HEATER_ON_LEVEL); }
  }
  else if (!strcmp(device, "buzzer")) {
    if (on) pendingBuzzerPulse = true;
    else { pendingBuzzerPulse = false; stopPulse(buzzerON, BUZZER_PIN, BUZZER_ON_LEVEL); }
  }
  requestActuatorStatePublish();
}
