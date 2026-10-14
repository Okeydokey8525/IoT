#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================================
// HE THONG GIAM SAT & CANH BAO QUA TAI DIEN NANG - NHOM 16 (De tai 21)
// PHAN MO RONG TUAN 8: FSM (Da nghiem thu Tuan 7) + WiFi/MQTT non-blocking
// MQTT Topics: telemetry | alarm | control | status (LWT)
// ============================================================================

// ============================================================================
// 1. CAU HINH PHAN CUNG & CHAN GPIO (KHOIP 100% voi diagram.json Wokwi)
// ============================================================================
const int PIN_POT    = 34; // Bien tro mo phong cong suat (ADC)
const int PIN_LED    = 25; // Den do canh bao
const int PIN_BUZZER = 26; // Coi Buzzer (LEDC PWM)
const int PIN_RELAY  = 33; // Relay 5V (Active-HIGH tren Wokwi: HIGH = DONG, LOW = NGAT)

const int BUZZER_CHANNEL = 0;
const int BUZZER_RES     = 8; // 8-bit (0 - 255)

// ============================================================================
// 2. THAM SO DIEN NANG & NGUONG BAO VE FSM
// ============================================================================
const float VOLTAGE_AC         = 220.0f;  // Dien ap danh dinh lu dan dung (V)
const float MAX_POWER_SCALE    = 3000.0f; // Thang do cong suat cuc dai (W)
const float WARN_THRESHOLD_W   = 1200.0f; // Nguong bat dau canh bao (W)
const float CLEAR_THRESHOLD_W  = 1100.0f; // Nguong ha tai phuc hoi - Hysteresis (W)
const unsigned long TR_DELAY_MS = 3000;   // Thoi gian tre WARNING -> CRITICAL (ms)

// May trang thai huu han (FSM)
enum SysState : uint8_t { STATE_NORMAL, STATE_WARNING, STATE_CRITICAL };
SysState currentState = STATE_NORMAL;
unsigned long warnStartTime = 0;
bool relayState = true;       // true = DONG (cap dien), false = NGAT (bao ve)
bool manualRelayMode = false; // true khi nhan lenh relay_on/relay_off tu xa (downlink)
bool alarmSent = false;       // Co edge-trigger: chi ban 1 tin ALARM khi vao CRITICAL

// Cac dai luong dien nang phuc vu giam sat
float powerW = 0.0f;
float currentA = 0.0f;
double totalEnergyKWh = 0.0;
unsigned long lastEnergyCalcMs = 0;

// Bo dem buzzer tranh spam PWM lien tuc
bool cachedBuzzerState = false;
int  cachedBuzzerFreq  = 0;

// ============================================================================
// 3. CAU HINH MANG WI-FI & MQTT BROKER
// ============================================================================
const char* WIFI_SSID = "Wokwi-GUEST"; // Wi-Fi ao cua Wokwi, khong mat khau
const char* WIFI_PASS = "";

const char* MQTT_BROKER    = "broker.hivemq.com"; // Broker cong cong mien phi
const uint16_t MQTT_PORT   = 1883;
const char* MQTT_CLIENT_ID = "Nhom16_IoT_ESP32_P21";

// Ban do Topic chuan hoa 3 ken (+1 kenh trang thai/LWT)
const char* TOPIC_TELEMETRY = "nhom16/bai21/telemetry"; // Uplink dinh ky (2s)
const char* TOPIC_ALARM     = "nhom16/bai21/alarm";     // Uplink su kien khan
const char* TOPIC_CONTROL   = "nhom16/bai21/control";   // Downlink dieu khien tu xa
const char* TOPIC_STATUS    = "nhom16/bai21/status";    // LWT: online/offline

WiFiClient espClient;
PubSubClient mqtt(espClient);

// Bo dem thoi gian non-blocking
unsigned long lastSampleMs    = 0;
unsigned long lastPublishMs   = 0;
unsigned long lastWifiRetryMs = 0;
unsigned long lastMqttRetryMs = 0;

const unsigned long SAMPLE_INTERVAL_MS  = 10;   // Lay mau ADC moi 10ms
const unsigned long PUBLISH_INTERVAL_MS = 2000; // Day telemetry moi 2s
const unsigned long RETRY_INTERVAL_MS   = 5000; // Thu ket noi lai moi 5s

// ============================================================================
// 4. BO LOC TRUNG BINH TRUOT (MOVING AVERAGE - 8 MAU)
// ============================================================================
const int FILT_N = 8;
float adcBuffer[FILT_N] = {0};
int   adcIndex = 0;
bool  bufferFilled = false;

float updateMovingAverage(int rawADC) {
  adcBuffer[adcIndex++] = (float)rawADC;
  if (adcIndex >= FILT_N) { adcIndex = 0; bufferFilled = true; }
  int count = bufferFilled ? FILT_N : adcIndex;
  float sum = 0;
  for (int i = 0; i < count; i++) sum += adcBuffer[i];
  return sum / count;
}

// ============================================================================
// 5. DIEU KHIEN CO CAU CHAP HANH
// ============================================================================
void setRelay(bool enable) {
  relayState = enable;
  // Module Relay active-HIGH: HIGH = dong tiep diem, LOW = ngat tiep diem
  digitalWrite(PIN_RELAY, enable ? HIGH : LOW);
}

void setBuzzer(bool enable, int freq = 1500) {
  if (enable == cachedBuzzerState && freq == cachedBuzzerFreq) return; // tranh spam LEDC
  cachedBuzzerState = enable;
  cachedBuzzerFreq  = freq;
  ledcWriteTone(BUZZER_CHANNEL, enable ? freq : 0);
}
// ============================================================================
// 6. XU LY NHAN LENH DIEU KHIEN TU XA (DOWNLINK CALLBACK)
// ============================================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_CONTROL) != 0) return;

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.printf("[MQTT] Parse JSON lenh that bai: %s\r\n", error.c_str());
    return;
  }

  const char* cmd = doc["cmd"];
  if (!cmd) return;
  Serial.printf("[MQTT Downlink] Nhan lenh: %s\r\n", cmd);

  if (strcmp(cmd, "relay_on") == 0) {
    manualRelayMode = true;   // Bat co dieu khien thu cong, FSM khong tu dong doi lai
    setRelay(true);
    Serial.println("[DIEU KHIEN] Da bat Relay tu xa.\r\n");
  } else if (strcmp(cmd, "relay_off") == 0) {
    manualRelayMode = true;   // Chan FSM tu dong dong lai Relay trong trang thai NORMAL
    setRelay(false);
    Serial.println("[DIEU KHIEN] Da tat Relay tu xa.\r\n");
  } else if (strcmp(cmd, "reset") == 0) {
    manualRelayMode = false;  // Trao toan quyen kiem soat lai cho FSM
    currentState = STATE_NORMAL;
    alarmSent = false;
    setBuzzer(false);
    setRelay(true);
    Serial.println("[DIEU KHIEN] Da Reset he thong ve NORMAL.\r\n");
  }
}

// ============================================================================
// 7. QUAN LY KET NOI MANG NON-BLOCKING (Tuyet doi khong dung delay)
// ============================================================================
void handleNetwork(unsigned long now) {
  // 1. Duy tri ket noi Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWifiRetryMs >= RETRY_INTERVAL_MS) {
      lastWifiRetryMs = now;
      Serial.println("[WiFi] Dang thu ket noi lai...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    return; // Dang mat mang Wi-Fi: thoat ngay de FSM van chay va cat tai tai cho
  }

  // 2. Duy tri ket noi MQTT Broker
  if (!mqtt.connected()) {
    if (now - lastMqttRetryMs >= RETRY_INTERVAL_MS) {
      lastMqttRetryMs = now;
      Serial.println("[MQTT] Dang ket noi toi Broker HiveMQ...");

      // Cau hinh LWT: neu ESP32 mat nguon dot ngot, broker tu publish "offline"
      if (mqtt.connect(MQTT_CLIENT_ID, TOPIC_STATUS, 1, true, "offline")) {
        Serial.println("[MQTT] Ket noi thanh cong!");
        mqtt.publish(TOPIC_STATUS, "online", true);
        mqtt.subscribe(TOPIC_CONTROL); // Lang nghe lenh dieu khien
      } else {
        Serial.printf("[MQTT] Ket noi that bai, ma loi: %d\r\n", mqtt.state());
      }
    }
    return;
  }

  // 3. Giu nhip Keep-alive va nhan ban tin den
  mqtt.loop();
}

// ============================================================================
// 8. DONG GOI & GUI DU LIEU MQTT JSON (UPLINK)
// ============================================================================
void publishTelemetry() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<256> doc;
  doc["U"]     = VOLTAGE_AC;
  doc["I"]     = serialized(String(currentA, 2));
  doc["P"]     = serialized(String(powerW, 1));
  doc["E"]     = serialized(String(totalEnergyKWh, 5));
  doc["relay"] = relayState ? "ON" : "OFF";

  switch (currentState) {
    case STATE_NORMAL:   doc["state"] = "NORMAL";   break;
    case STATE_WARNING:  doc["state"] = "WARNING";  break;
    case STATE_CRITICAL: doc["state"] = "CRITICAL"; break;
  }

  char buffer[256];
  size_t bytes = serializeJson(doc, buffer);
  mqtt.publish(TOPIC_TELEMETRY, buffer, bytes);
}

void publishAlarmEvent() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<192> doc;
  doc["event"]     = "OVERLOAD_CRITICAL";
  doc["P_trip"]    = serialized(String(powerW, 1));
  doc["threshold"] = WARN_THRESHOLD_W;
  doc["relay"]     = "OFF";
  doc["msg"]       = "Qua tai keo dai qua 3 giay. Da ngat Relay!";

  char buffer[192];
  size_t bytes = serializeJson(doc, buffer);
  // Gui dung 1 lan moi lan vao CRITICAL (edge-triggered qua co alarmSent)
  mqtt.publish(TOPIC_ALARM, buffer, bytes);
  Serial.println("[MQTT UPLINK] Da ban su kien ALARM len Broker.\r\n");
}
// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_POT, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);

  // Cau hinh PWM cho coi buzzer
  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RES);
  ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);

  // Khoi tao trang thai ban dau an toan
  digitalWrite(PIN_LED, LOW);
  setBuzzer(false);
  setRelay(true);

  // Cau hinh mang ban dau
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512); // Nang buffer tranh truan goi tin JSON

  lastEnergyCalcMs = millis();

  Serial.println("\r\n========================================================");
  Serial.println(" [IoT Nhom 16] - KHOI DONG FIRMWARE TUAN 8 (FSM + MQTT)");
  Serial.println(" Topics: telemetry | alarm | control | status");
  Serial.println("========================================================\r\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();

  // ----------------------------------------------------
  // GIAI DOAN 1: Doc cam bien va loc trung binh (moi 10ms)
  // ----------------------------------------------------
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    int rawADC = analogRead(PIN_POT);
    float avgADC = updateMovingAverage(rawADC);
    powerW = (avgADC / 4095.0f) * MAX_POWER_SCALE;

    // Dong dien tinh theo tai AC (chi co dong khi Relay dong)
    currentA = relayState ? (powerW / VOLTAGE_AC) : 0.0f;

    // Tich luy dien nang tieu thu (chi tich luy khi tai dang co dien)
    unsigned long dtMs = now - lastEnergyCalcMs;
    lastEnergyCalcMs = now;
    if (relayState && powerW > 0.0f) {
      totalEnergyKWh += ((double)powerW * (double)dtMs) / (3600000.0 * 1000.0);
    }
  }

  // ----------------------------------------------------
  // GIAI DOAN 2A: CHUYEN TRANG THAI FSM (chi tinh toan dieu kien)
  // ----------------------------------------------------
  switch (currentState) {
    case STATE_NORMAL:
      if (powerW > WARN_THRESHOLD_W) {
        currentState = STATE_WARNING;
        warnStartTime = now;
        alarmSent = false;
      }
      break;

    case STATE_WARNING:
      if (powerW < CLEAR_THRESHOLD_W) {
        currentState = STATE_NORMAL;
      } else if (now - warnStartTime >= TR_DELAY_MS) {
        currentState = STATE_CRITICAL;
      }
      break;

    case STATE_CRITICAL:
      // Chi khi nguoi dung ha cong suat ve duoi 1100W moi tu phuc hoi
      if (powerW < CLEAR_THRESHOLD_W) {
        currentState = STATE_NORMAL;
        manualRelayMode = false; // Trao lai quyen kiem soat cho FSM
        alarmSent = false;
      }
      break;
  }

  // ----------------------------------------------------
  // GIAI DOAN 2B: XUAT DAU RA ACTUATOR THEO TRANG THAI HIEN TAI
  // (Ap dung TRONG CUNG vong lap voi phep chuyen trang thai ->
  //  log Serial luon nhat quan, khong xay hien tuong
  //  "CRIT nhung Relay van ON" o chinh dau chuyen trang thai)
  // ----------------------------------------------------
  switch (currentState) {
    case STATE_NORMAL:
      digitalWrite(PIN_LED, LOW);
      setBuzzer(false);
      // Chi tu dong dong lai Relay khi KHONG dang o che do dieu khien thu cong
      // (Tranh de lenh relay_off tu XA bi huy bo ngay lap luc)
      if (!manualRelayMode && !relayState) {
        setRelay(true);
      }
      break;

    case STATE_WARNING:
      digitalWrite(PIN_LED, HIGH); // Bat LED canh bao truoc
      setBuzzer(false);            // Chua hu coi
      if (!manualRelayMode) {
        setRelay(true);            // RELAY VAN DONG (chua ngat tai)
      }
      break;

    case STATE_CRITICAL:
      digitalWrite(PIN_LED, HIGH);
      setBuzzer(true, 1500); // Hu coi 1500Hz
      setRelay(false);       // NGAT RELAY BAO VE

      // Ban ban tin ALARM len MQTT dung 1 lan (Edge-Triggered)
      if (!alarmSent && mqtt.connected()) {
        publishAlarmEvent();
      }
      alarmSent = true;
      break;
  }

  // ----------------------------------------------------
  // GIAI DOAN 3: Xu ly mang & MQTT Non-blocking
  // ----------------------------------------------------
  handleNetwork(now);

  // ----------------------------------------------------
  // GIAI DOAN 4: Xuat ban tin dinh ky (moi 2 giay)
  // ----------------------------------------------------
  if (now - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;

    publishTelemetry();

    // In Log chuan hoa ra Serial Monitor (co \r\n chong lech dong)
    const char* stStr = (currentState == STATE_NORMAL) ? "NORMAL" :
                        (currentState == STATE_WARNING) ? "WARN" : "CRIT";

    Serial.printf("[FSM: %-6s] P: %6.1f W | I: %4.2f A | E: %8.5f kWh | Relay: %-3s | WiFi: %s | MQTT: %s\r\n",
                  stStr,
                  powerW,
                  currentA,
                  totalEnergyKWh,
                  relayState ? "ON" : "OFF",
                  (WiFi.status() == WL_CONNECTED) ? "OK" : "NO",
                  mqtt.connected() ? "OK" : "NO");
  }
}


