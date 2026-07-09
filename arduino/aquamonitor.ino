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

// Mode uji fungsional DO + aerator.
// true  = hanya sensor suhu + DO yang dibaca, hanya aerator yang boleh bekerja,
//         frame tetap dikirim ke ESP agar dashboard/RTDB masih menerima data.
// false = mode normal AquaMonitor.
const bool DO_AERATOR_TEST_MODE = false;
const bool TEMP_HEATER_TEST_MODE = false;
const bool PH_PUMP_TEST_MODE = false;
const bool TDS_NUTRITION_TEST_MODE = false;
const bool WATER_PUMP_TEST_MODE = false;
const bool BUZZER_TEST_MODE = false;
const bool DOSING_PUMP_IDENTIFY_TEST_MODE = false;
const float DO_AERATOR_ON_MG_L  = 5.0; // aerator diizinkan cycle jika DO <= nilai ini
const float DO_AERATOR_OFF_MG_L = 7.0; // aerator dimatikan jika DO >= nilai ini
// Khusus bypass test. Jika nutrition pump fisik tetap ON saat Serial bilang OFF,
// ubah LOW <-> HIGH untuk memastikan polaritas relay channel nutrisi.
const uint8_t TDS_PUMP_TEST_OFF_LEVEL = LOW;
const unsigned long IDENTIFY_PUMP_ON_MS = 1000UL;
const unsigned long IDENTIFY_PUMP_OFF_MS = 4000UL;
// Pilih satu pompa saja untuk identifikasi:
// 1 = pH DOWN / ASAM  (pin D8)
// 2 = pH UP / BASA    (pin D7)
// 3 = NUTRISI / TDS   (pin D13)
const uint8_t DOSING_IDENTIFY_TARGET = 1;

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
#define DO_SAMPLES  15

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
const uint8_t PH_ASAM_ON_LEVEL = HIGH;
const uint8_t PH_BASA_ON_LEVEL = HIGH;
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
bool doAeratorTestWebEnabled = true;
bool tempHeaterTestWebEnabled = false;
bool phPumpDownTestWebEnabled = false;
bool phPumpUpTestWebEnabled = false;
bool nutritionTestWebEnabled = false;
bool buzzerTestWebEnabled = false;
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
  int samples[DO_SAMPLES];

  for (int i = 0; i < DO_SAMPLES; i++) {
    samples[i] = analogRead(DO_PIN);
    delay(10);
  }

  for (int i = 0; i < DO_SAMPLES - 1; i++) {
    for (int j = i + 1; j < DO_SAMPLES; j++) {
      if (samples[i] > samples[j]) {
        int temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }

  int medianADC = samples[DO_SAMPLES / 2];
  return medianADC * (VREF / 1023.0);
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
  if (DO_AERATOR_TEST_MODE) {
    runDoAeratorTestLoop();
    return;
  }
  if (TEMP_HEATER_TEST_MODE) {
    runTempHeaterTestLoop();
    return;
  }
  if (PH_PUMP_TEST_MODE) {
    runPhPumpTestLoop();
    return;
  }
  if (TDS_NUTRITION_TEST_MODE) {
    runTdsNutritionTestLoop();
    return;
  }
  if (WATER_PUMP_TEST_MODE) {
    runWaterPumpTestLoop();
    return;
  }
  if (BUZZER_TEST_MODE) {
    runBuzzerTestLoop();
    return;
  }
  if (DOSING_PUMP_IDENTIFY_TEST_MODE) {
    runDosingPumpIdentifyTestLoop();
    return;
  }

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
  float nilaiDO = voltageToDO(voltDO, (uint8_t)suhu) / 1000.0;
  float threshold_bawah = 3.0;
  float hasiDO = nilaiDO + threshold_bawah;
  if (hasiDO < 0) hasiDO = 0;
  if (debugThisLoop) Serial.println(F("[DBG] after DO"));

  applyAutomaticControl(suhu, phact, tdsValue, turbADC);
  if (debugThisLoop) Serial.println(F("[DBG] after control"));

  // Mode final:
  // - Sensor dibaca oleh Arduino dan dikirim ke ESP.
  // - Threshold user diproses oleh Node-RED/RTDB.
  // - Command dari Node-RED masuk lewat ESP.
  // - Arduino mengeksekusi aktuator dengan pulse/cycle aman.
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
  Serial.print(F("DO       : ")); Serial.print(hasiDO, 2);         Serial.println(F(" mg/L"));
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
      espSerial.print(hasiDO, 2);     espSerial.print(",");
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

float readWaterTemperature() {
  sensors.requestTemperatures();
  float tempTerbaca = sensors.getTempCByIndex(0);
  if (tempTerbaca != DEVICE_DISCONNECTED_C && tempTerbaca > -50.0 && tempTerbaca < 100.0) {
    return tempTerbaca - 0.9f;
  }
  return 25.0;
}

float readCalibratedDO(float suhu, float& voltDO) {
  voltDO = readDOVoltage();
  float nilaiDO = voltageToDO(voltDO, (uint8_t)suhu) / 1000.0;
  float threshold_bawah = 3.0;
  float hasiDO = nilaiDO + threshold_bawah;
  if (hasiDO < 0) hasiDO = 0;
  return hasiDO;
}

void runDoAeratorTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain aerator benar-benar OFF selama test.
  pumpEnabled = false;
  pumpON = false;
  heaterON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  tdsPumpON = false;
  buzzerON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingTdsDose = false;
  pendingHeaterPulse = false;
  pendingBuzzerPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
  digitalWrite(TDS_PUMP, TDS_PUMP_TEST_OFF_LEVEL);

  float suhu = readWaterTemperature();
  float voltDO = 0;
  float doValue = readCalibratedDO(suhu, voltDO);

  if (!doAeratorTestWebEnabled) {
    aeratorEnabled = false;
    aeratorON = false;
    setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  } else if (doValue <= DO_AERATOR_ON_MG_L) {
    aeratorEnabled = true;
  } else if (doValue >= DO_AERATOR_OFF_MG_L) {
    aeratorEnabled = false;
    aeratorON = false;
    setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  }

  updateAeratorCycle();
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, aeratorON);

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] DO + AERATOR ONLY"));
  Serial.print(F("Suhu     : ")); Serial.print(suhu, 1); Serial.println(F(" C"));
  Serial.print(F("DO       : ")); Serial.print(doValue, 2); Serial.println(F(" mg/L"));
  Serial.print(F("Teg. DO  : ")); Serial.print(voltDO, 4); Serial.println(F(" V"));
  Serial.print(F("Rule     : aerator ON jika DO <= ")); Serial.print(DO_AERATOR_ON_MG_L, 1);
  Serial.print(F(" | OFF jika DO >= ")); Serial.println(DO_AERATOR_OFF_MG_L, 1);
  Serial.print(F("Web Ctrl : aerator ")); Serial.println(doAeratorTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.println(F("Pump         : OFF (bypass test)"));
  Serial.print(F("Aerator      : "));
  if (!aeratorEnabled) Serial.println(F("OFF (DO cukup / disabled)"));
  else Serial.println(aeratorON ? F("ON (5 sec cycle)") : F("OFF (20 sec lockout)"));
  Serial.println(F("Heater       : OFF (bypass test)"));
  Serial.println(F("pH Asam Pump : OFF (bypass test)"));
  Serial.println(F("pH Basa Pump : OFF (bypass test)"));
  Serial.println(F("TDS Pump     : OFF (bypass test)"));
  Serial.println(F("Buzzer       : OFF (bypass test)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(suhu, 1);       espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(doValue, 2);    espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);
      espSerial.println(">");
    }
  }

  delay(1000);
}

void runTempHeaterTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain heater benar-benar OFF selama test.
  pumpEnabled = false;
  pumpON = false;
  aeratorEnabled = false;
  aeratorON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  tdsPumpON = false;
  buzzerON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingTdsDose = false;
  pendingBuzzerPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
  digitalWrite(TDS_PUMP, TDS_PUMP_TEST_OFF_LEVEL);

  float suhu = readWaterTemperature();

  updatePulseTimers();

  if (!tempHeaterTestWebEnabled) {
    pendingHeaterPulse = false;
    stopPulse(heaterON, HEATER_PIN, HEATER_ON_LEVEL);
  } else if (suhu < TEMP_HEATER_ON_C) {
    pendingHeaterPulse = true;
  } else if (suhu >= TEMP_HEATER_OFF_C) {
    pendingHeaterPulse = false;
    stopPulse(heaterON, HEATER_PIN, HEATER_ON_LEVEL);
  }

  if (pendingHeaterPulse && !hasActiveRelay()) {
    pendingHeaterPulse = false;
    startPulse("heater", heaterON, heaterPulseStartedAt, heaterLastPulseAt, HEATER_LOCKOUT_MS, HEATER_PIN, HEATER_ON_LEVEL);
  }
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, heaterON);

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] TEMPERATURE + HEATER ONLY"));
  Serial.print(F("Suhu     : ")); Serial.print(suhu, 1); Serial.println(F(" C"));
  Serial.print(F("Rule     : heater pulse jika suhu < ")); Serial.print(TEMP_HEATER_ON_C, 1);
  Serial.print(F(" | OFF jika suhu >= ")); Serial.println(TEMP_HEATER_OFF_C, 1);
  Serial.print(F("Web Ctrl : heater ")); Serial.println(tempHeaterTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.println(F("Pump         : OFF (bypass test)"));
  Serial.println(F("Aerator      : OFF (bypass test)"));
  Serial.print(F("Heater       : "));
  if (!tempHeaterTestWebEnabled) Serial.println(F("OFF (web disabled)"));
  else Serial.println(heaterON ? F("ON (5 sec pulse)") : F("OFF (lockout / suhu cukup)"));
  Serial.println(F("pH Asam Pump : OFF (bypass test)"));
  Serial.println(F("pH Basa Pump : OFF (bypass test)"));
  Serial.println(F("TDS Pump     : OFF (bypass test)"));
  Serial.println(F("Buzzer       : OFF (bypass test)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(suhu, 1);       espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(heaterON);      espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);
      espSerial.println(">");
    }
  }

  delay(1000);
}

float readStablePH() {
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

  float ph = (m * voltFinalPH) + b;
  if (ph < 0.0) ph = 0.0;
  if (ph > 14.0) ph = 14.0;
  return ph;
}

void runPhPumpTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain pH pump benar-benar OFF selama test.
  pumpEnabled = false;
  pumpON = false;
  aeratorEnabled = false;
  aeratorON = false;
  heaterON = false;
  tdsPumpON = false;
  buzzerON = false;
  pendingTdsDose = false;
  pendingHeaterPulse = false;
  pendingBuzzerPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  digitalWrite(TDS_PUMP, TDS_PUMP_TEST_OFF_LEVEL);

  updateDoseTimers();

  float suhu = readWaterTemperature();
  float ph = readStablePH();

  if (!phPumpDownTestWebEnabled) {
    pendingPhAsamDose = false;
    stopDose(phAsamPumpON, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL);
  } else if (ph > PH_HIGH_LIMIT) {
    pendingPhAsamDose = true;
  }

  if (!phPumpUpTestWebEnabled) {
    pendingPhBasaDose = false;
    stopDose(phBasaPumpON, PH_BASA_PUMP, PH_BASA_ON_LEVEL);
  } else if (ph < PH_LOW_LIMIT) {
    pendingPhBasaDose = true;
  }

  if (pendingPhAsamDose && !hasActiveRelay()) {
    pendingPhAsamDose = false;
    startDose("phPumpDown", phAsamPumpON, phAsamDoseStartedAt, phAsamLastDoseAt, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL);
  } else if (pendingPhBasaDose && !hasActiveRelay()) {
    pendingPhBasaDose = false;
    startDose("phPumpUp", phBasaPumpON, phBasaDoseStartedAt, phBasaLastDoseAt, PH_BASA_PUMP, PH_BASA_ON_LEVEL);
  }

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] PH + PH PUMPS ONLY"));
  Serial.print(F("Suhu     : ")); Serial.print(suhu, 1); Serial.println(F(" C"));
  Serial.print(F("pH       : ")); Serial.print(ph, 2);
  Serial.print(F(" (Teg: ")); Serial.print(voltFinalPH, 3); Serial.println(F(" V)"));
  Serial.print(F("Rule     : phPumpDown ON jika pH > ")); Serial.print(PH_HIGH_LIMIT, 1);
  Serial.print(F(" | phPumpUp ON jika pH < ")); Serial.println(PH_LOW_LIMIT, 1);
  Serial.print(F("Web Ctrl : phPumpDown ")); Serial.println(phPumpDownTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.print(F("Web Ctrl : phPumpUp   ")); Serial.println(phPumpUpTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.println(F("Pump         : OFF (bypass test)"));
  Serial.println(F("Aerator      : OFF (bypass test)"));
  Serial.println(F("Heater       : OFF (bypass test)"));
  Serial.print(F("pH Asam Pump : ")); printDoseStatus(phAsamPumpON, ph > PH_HIGH_LIMIT, phAsamLastDoseAt);
  Serial.print(F("pH Basa Pump : ")); printDoseStatus(phBasaPumpON, ph < PH_LOW_LIMIT, phBasaLastDoseAt);
  Serial.println(F("TDS Pump     : OFF (bypass test)"));
  Serial.println(F("Buzzer       : OFF (bypass test)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(suhu, 1);       espSerial.print(",");
      espSerial.print(ph, 2);         espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(phAsamPumpON);  espSerial.print(",");
      espSerial.print(phBasaPumpON);  espSerial.print(",");
      espSerial.print(0);
      espSerial.println(">");
    }
  }

  delay(1000);
}

float readStableTDSValue(float suhu) {
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

  int medianValue = analogBufferTemp[SCOUNT / 2];
  averageVoltage = medianValue * VREF / 1023.0;
  float ppmKasar = averageVoltage * TDS_CAL_FACTOR * K_CELL;
  float kompensasi = 1.0 + 0.02 * (suhu - 25.0);
  float tds = ppmKasar / kompensasi;
  if (tds < 0) tds = 0;
  return getStableTDS(tds);
}

void runTdsNutritionTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain nutrition pump benar-benar OFF selama test.
  pumpEnabled = false;
  pumpON = false;
  aeratorEnabled = false;
  aeratorON = false;
  heaterON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  buzzerON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingHeaterPulse = false;
  pendingBuzzerPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);

  updateDoseTimers();

  float suhu = readWaterTemperature();
  float tds = readStableTDSValue(suhu);

  if (!nutritionTestWebEnabled) {
    pendingTdsDose = false;
    stopDose(tdsPumpON, TDS_PUMP, TDS_ON_LEVEL);
  } else if (tds < TDS_LOW_LIMIT) {
    pendingTdsDose = true;
  }

  if (pendingTdsDose && !hasActiveRelay()) {
    pendingTdsDose = false;
    startDose("nutritionPump", tdsPumpON, tdsDoseStartedAt, tdsLastDoseAt, TDS_PUMP, TDS_ON_LEVEL);
  }

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] TDS + NUTRITION PUMP ONLY"));
  Serial.print(F("Suhu     : ")); Serial.print(suhu, 1); Serial.println(F(" C"));
  Serial.print(F("TDS      : ")); Serial.print(tds, 1); Serial.println(F(" ppm"));
  Serial.print(F("Teg. TDS : ")); Serial.print(averageVoltage, 4); Serial.println(F(" V"));
  Serial.print(F("Rule     : nutritionPump ON jika TDS < ")); Serial.println(TDS_LOW_LIMIT, 1);
  Serial.print(F("Web Ctrl : nutritionPump ")); Serial.println(nutritionTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.println(F("Pump         : OFF (bypass test)"));
  Serial.println(F("Aerator      : OFF (bypass test)"));
  Serial.println(F("Heater       : OFF (bypass test)"));
  Serial.println(F("pH Asam Pump : OFF (bypass test)"));
  Serial.println(F("pH Basa Pump : OFF (bypass test)"));
  Serial.print(F("TDS Pump     : ")); printDoseStatus(tdsPumpON, tds < TDS_LOW_LIMIT, tdsLastDoseAt);
  Serial.println(F("Buzzer       : OFF (bypass test)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(suhu, 1);       espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(tds, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(tdsPumpON);
      espSerial.println(">");
    }
  }

  delay(1000);
}

void runWaterPumpTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain water pump benar-benar OFF selama test.
  aeratorEnabled = false;
  aeratorON = false;
  heaterON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  tdsPumpON = false;
  buzzerON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingTdsDose = false;
  pendingHeaterPulse = false;
  pendingBuzzerPulse = false;

  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
  setRelay(TDS_PUMP, TDS_ON_LEVEL, false);

  updateWaterPumpCycle();
  setRelay(PUMP_PIN, PUMP_ON_LEVEL, pumpON);

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] WATER PUMP ONLY"));
  Serial.print(F("Rule     : waterPump cycle ON ")); Serial.print(PUMP_ON_DURATION_MS / 1000);
  Serial.print(F(" sec | OFF ")); Serial.print(PUMP_LOCKOUT_MS / 1000); Serial.println(F(" sec"));
  Serial.print(F("Web Ctrl : waterPump ")); Serial.println(pumpEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.print(F("Pump         : "));
  if (!pumpEnabled) Serial.println(F("OFF (web disabled)"));
  else Serial.println(pumpON ? F("ON (2 sec cycle)") : F("OFF (20 sec lockout)"));
  Serial.println(F("Aerator      : OFF (bypass test)"));
  Serial.println(F("Heater       : OFF (bypass test)"));
  Serial.println(F("pH Asam Pump : OFF (bypass test)"));
  Serial.println(F("pH Basa Pump : OFF (bypass test)"));
  Serial.println(F("TDS Pump     : OFF (bypass test)"));
  Serial.println(F("Buzzer       : OFF (bypass test)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);
      espSerial.println(">");
    }
  }

  delay(1000);
}

void runBuzzerTestLoop() {
  const unsigned long now = millis();

  if (ENABLE_REMOTE_ACTUATOR_COMMANDS && millis() > COMMAND_STARTUP_DELAY_MS) {
    receiveEspCommands();
  }

  // Pastikan semua aktuator selain buzzer benar-benar OFF selama test.
  pumpEnabled = false;
  pumpON = false;
  aeratorEnabled = false;
  aeratorON = false;
  heaterON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  tdsPumpON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingTdsDose = false;
  pendingHeaterPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
  setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
  setRelay(TDS_PUMP, TDS_ON_LEVEL, false);

  updatePulseTimers();

  if (!buzzerTestWebEnabled) {
    pendingBuzzerPulse = false;
    stopPulse(buzzerON, BUZZER_PIN, BUZZER_ON_LEVEL);
  } else {
    pendingBuzzerPulse = true;
  }

  if (pendingBuzzerPulse && !hasActiveRelay()) {
    pendingBuzzerPulse = false;
    startPulse("buzzer", buzzerON, buzzerPulseStartedAt, buzzerLastPulseAt, BUZZER_LOCKOUT_MS, BUZZER_PIN, BUZZER_ON_LEVEL);
  }
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, buzzerON);

  Serial.println(F("----------------------------------"));
  Serial.println(F("[TEST MODE] BUZZER ONLY"));
  Serial.print(F("Rule     : buzzer pulse ON ")); Serial.print(BUZZER_PULSE_DURATION_MS / 1000);
  Serial.print(F(" sec | OFF ")); Serial.print(BUZZER_LOCKOUT_MS / 1000); Serial.println(F(" sec"));
  Serial.print(F("Web Ctrl : buzzer ")); Serial.println(buzzerTestWebEnabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(F("===== STATUS AKTUATOR ====="));
  Serial.println(F("Pump         : OFF (bypass test)"));
  Serial.println(F("Aerator      : OFF (bypass test)"));
  Serial.println(F("Heater       : OFF (bypass test)"));
  Serial.println(F("pH Asam Pump : OFF (bypass test)"));
  Serial.println(F("pH Basa Pump : OFF (bypass test)"));
  Serial.println(F("TDS Pump     : OFF (bypass test)"));
  Serial.print(F("Buzzer       : ")); Serial.println(buzzerON ? F("ON (1 sec pulse)") : F("OFF (lockout / web disabled)"));
  Serial.println(F("==========================="));

  if (pendingActuatorStatePublish && !hasActiveRelay()) {
    publishActuatorState();
  }

  if (now - lastPublishTime >= PUBLISH_INTERVAL && !hasActiveRelay()) {
    lastPublishTime = now;
    if (!ESP_SAFE_TEST_MODE) {
      // Format tetap sama agar esp_bridge.ino dan dashboard tidak perlu diubah:
      // <temperature,ph,tds,turbidity,do,heater,phAcid,phBase,tdsPump>
      espSerial.print("<");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 1);        espSerial.print(",");
      espSerial.print(0.0, 2);        espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);             espSerial.print(",");
      espSerial.print(0);
      espSerial.println(">");
    }
  }

  delay(1000);
}

void forceAllRelaysOffForDosingIdentify() {
  pumpEnabled = false;
  pumpON = false;
  aeratorEnabled = false;
  aeratorON = false;
  heaterON = false;
  phAsamPumpON = false;
  phBasaPumpON = false;
  tdsPumpON = false;
  buzzerON = false;
  pendingPhAsamDose = false;
  pendingPhBasaDose = false;
  pendingTdsDose = false;
  pendingHeaterPulse = false;
  pendingBuzzerPulse = false;

  setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
  setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
  setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
  setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
  // Mode identifikasi dosing dibuat direct-write agar tidak tertukar oleh
  // konfigurasi polaritas lama. Untuk uji ini diasumsikan relay dosing aktif HIGH:
  // LOW = OFF, HIGH = ON. Jika fisik masih ON saat LOW, cek terminal NC/NO.
  digitalWrite(PH_ASAM_PUMP, LOW);
  digitalWrite(PH_BASA_PUMP, LOW);
  digitalWrite(TDS_PUMP, LOW);
}

void pulseIdentifyPump(const __FlashStringHelper* label, uint8_t pin) {
  forceAllRelaysOffForDosingIdentify();
  delay(IDENTIFY_PUMP_OFF_MS);

  Serial.println(F("----------------------------------"));
  Serial.print(F("[IDENTIFY] SEKARANG TEST: "));
  Serial.println(label);
  Serial.println(F("Pompa ini ON 1 detik. Lihat fisik pompa mana yang menyala."));
  Serial.print(F("Pin relay: D"));
  Serial.println(pin);

  digitalWrite(pin, HIGH);
  delay(IDENTIFY_PUMP_ON_MS);

  digitalWrite(pin, LOW);

  Serial.print(F("[IDENTIFY] SELESAI: "));
  Serial.println(label);
  Serial.println(F("Catat hasilnya: pompa fisik mana yang tadi menyala?"));
}

void runDosingPumpIdentifyTestLoop() {
  static bool printedIntro = false;
  if (!printedIntro) {
    printedIntro = true;
    Serial.println(F("=================================="));
    Serial.println(F("[TEST MODE] IDENTIFIKASI SATU DOSING PUMP"));
    Serial.println(F("ESP, sensor, web, dan threshold DIABAIKAN sementara."));
    Serial.println(F("Hanya satu pompa yang dites sesuai DOSING_IDENTIFY_TARGET."));
    Serial.print(F("Target saat ini: "));
    Serial.println(DOSING_IDENTIFY_TARGET);
    Serial.println(F("=================================="));
  }

  if (DOSING_IDENTIFY_TARGET == 1) {
    pulseIdentifyPump(F("pH DOWN / ASAM"), PH_ASAM_PUMP);
  } else if (DOSING_IDENTIFY_TARGET == 2) {
    pulseIdentifyPump(F("pH UP / BASA"), PH_BASA_PUMP);
  } else if (DOSING_IDENTIFY_TARGET == 3) {
    pulseIdentifyPump(F("NUTRISI / TDS PUMP"), TDS_PUMP);
  } else {
    forceAllRelaysOffForDosingIdentify();
    Serial.println(F("[IDENTIFY] Target tidak valid. Gunakan 1, 2, atau 3."));
    delay(2000);
  }
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

  if (DO_AERATOR_TEST_MODE || TEMP_HEATER_TEST_MODE || PH_PUMP_TEST_MODE || TDS_NUTRITION_TEST_MODE || WATER_PUMP_TEST_MODE || BUZZER_TEST_MODE) {
    if (DO_AERATOR_TEST_MODE && !strcmp(device, "aerator")) {
      doAeratorTestWebEnabled = on;
      aeratorEnabled = on;
      if (on) {
        aeratorStateChangedAt = millis() - AERATOR_LOCKOUT_MS;
      } else {
        aeratorON = false;
        setRelay(AERATOR_PIN, AERATOR_ON_LEVEL, false);
      }
      requestActuatorStatePublish();
    } else if (TEMP_HEATER_TEST_MODE && !strcmp(device, "heater")) {
      tempHeaterTestWebEnabled = on;
      if (on) {
        pendingHeaterPulse = true;
      } else {
        pendingHeaterPulse = false;
        stopPulse(heaterON, HEATER_PIN, HEATER_ON_LEVEL);
      }
      requestActuatorStatePublish();
    } else if (PH_PUMP_TEST_MODE && !strcmp(device, "phPumpDown")) {
      phPumpDownTestWebEnabled = on;
      if (!on) {
        pendingPhAsamDose = false;
        stopDose(phAsamPumpON, PH_ASAM_PUMP, PH_ASAM_ON_LEVEL);
      }
      requestActuatorStatePublish();
    } else if (PH_PUMP_TEST_MODE && !strcmp(device, "phPumpUp")) {
      phPumpUpTestWebEnabled = on;
      if (!on) {
        pendingPhBasaDose = false;
        stopDose(phBasaPumpON, PH_BASA_PUMP, PH_BASA_ON_LEVEL);
      }
      requestActuatorStatePublish();
    } else if (TDS_NUTRITION_TEST_MODE && !strcmp(device, "nutritionPump")) {
      nutritionTestWebEnabled = on;
      if (!on) {
        pendingTdsDose = false;
        stopDose(tdsPumpON, TDS_PUMP, TDS_ON_LEVEL);
      }
      requestActuatorStatePublish();
    } else if (WATER_PUMP_TEST_MODE && !strcmp(device, "waterPump")) {
      if (on) {
        pumpEnabled = true;
        pumpStateChangedAt = millis() - PUMP_LOCKOUT_MS;
      } else {
        pumpEnabled = false;
        pumpON = false;
        pumpStateChangedAt = millis();
        setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
      }
      requestActuatorStatePublish();
    } else if (BUZZER_TEST_MODE && !strcmp(device, "buzzer")) {
      buzzerTestWebEnabled = on;
      if (on) {
        pendingBuzzerPulse = true;
      } else {
        pendingBuzzerPulse = false;
        stopPulse(buzzerON, BUZZER_PIN, BUZZER_ON_LEVEL);
      }
      requestActuatorStatePublish();
    } else {
      pendingPhAsamDose = false;
      pendingPhBasaDose = false;
      pendingTdsDose = false;
      pendingHeaterPulse = false;
      pendingBuzzerPulse = false;
      pumpEnabled = false;
      pumpON = false;
      heaterON = false;
      phAsamPumpON = false;
      phBasaPumpON = false;
      tdsPumpON = false;
      buzzerON = false;
      setRelay(PUMP_PIN, PUMP_ON_LEVEL, false);
      setRelay(HEATER_PIN, HEATER_ON_LEVEL, false);
      setRelay(BUZZER_PIN, BUZZER_ON_LEVEL, false);
      setRelay(PH_ASAM_PUMP, PH_ASAM_ON_LEVEL, false);
      setRelay(PH_BASA_PUMP, PH_BASA_ON_LEVEL, false);
      digitalWrite(TDS_PUMP, TDS_PUMP_TEST_OFF_LEVEL);
      Serial.println(F(">> TEST MODE: command selain aktuator uji diabaikan"));
    }
    return;
  }

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
