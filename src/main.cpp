#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h" // Thong tin broker rieng (khong commit) - mau: secrets.h.example

// ============================================================================
// HE THONG GIAM SAT & CANH BAO QUA TAI DIEN NANG - NHOM 16 (De tai 21)
// FSM bao ve + WiFi/MQTT
// MQTT Topics: telemetry | alarm | control | status (LWT)
// BAN DUY NHAT: build bang PlatformIO (core 2.x) va dan nguyen file len
// Wokwi web (core 3.x) deu chay duoc - xem setupBuzzer()/setBuzzer().
//
// KIEN TRUC 2 LUONG (sua sau kiem thu T5 ngay 20/11/2026):
//  - protectTask (FreeRTOS, core 1): doc ADC moi 10ms, chay FSM, dieu khien
//    LED/coi/relay. KHONG goi bat ky ham mang nao -> khong bao gio bi chan.
//  - loop() (luong Arduino): Wi-Fi, MQTT, gui telemetry/alarm, in Serial.
//    mqtt.connect() co the chan vai giay khi broker loi, nhung chi lam
//    cham viec gui du lieu, khong lam tre viec ngat tai.
//  - Lenh tu xa va su kien canh bao di qua cac bien co (pendingCmd,
//    alarmPending) thay vi goi truc tiep giua 2 luong.
// ============================================================================

// ============================================================================
// 1. CAU HINH PHAN CUNG & CHAN GPIO (KHOP 100% voi diagram.json Wokwi)
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

// May trang thai huu han (FSM) - chi protectTask duoc ghi
enum SysState : uint8_t { STATE_NORMAL, STATE_WARNING, STATE_CRITICAL };
volatile SysState currentState = STATE_NORMAL;
unsigned long warnStartTime = 0;
volatile bool relayState = true; // true = DONG (cap dien), false = NGAT (bao ve)
bool manualRelayMode = false;    // true khi nhan lenh relay_on/relay_off tu xa
bool alarmSent = false;          // Co edge-trigger: 1 su kien ALARM moi lan vao CRITICAL

// Cac dai luong dien nang (protectTask ghi, loop doc)
volatile float powerW = 0.0f;
volatile float currentA = 0.0f;
volatile double totalEnergyKWh = 0.0;
unsigned long lastEnergyCalcMs = 0;

// Bo dem buzzer tranh spam PWM lien tuc
bool cachedBuzzerState = false;
int  cachedBuzzerFreq  = 0;

// Giao tiep giua 2 luong
enum RemoteCmd : uint8_t { CMD_NONE, CMD_RELAY_ON, CMD_RELAY_OFF, CMD_RESET };
volatile RemoteCmd pendingCmd = CMD_NONE; // loop ghi, protectTask thuc hien
volatile bool  alarmPending = false;       // protectTask bat, loop gui len broker
volatile float alarmPowerW  = 0.0f;

// Nhat ky chuyen trang thai (protectTask ghi, loop in ra Serial)
struct FsmEvent { uint8_t from; uint8_t to; unsigned long atMs; float p; };
const uint8_t EVT_N = 8;
FsmEvent evtBuf[EVT_N];
volatile uint8_t evtHead = 0; // so su kien da ghi
uint8_t evtPrinted = 0;       // so su kien da in

portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// 3. CAU HINH MANG WI-FI & MQTT BROKER
// ============================================================================
const char* WIFI_SSID = "Wokwi-GUEST"; // Wi-Fi ao cua Wokwi, khong mat khau
const char* WIFI_PASS = "";

// BAO MAT TANG 2 (24/11/2026): broker rieng EMQX Cloud Serverless, bat buoc tai khoan
// + ma hoa TLS cong 8883. Thong tin dang nhap nam trong secrets.h.
const char* MQTT_BROKER    = SECRET_MQTT_HOST;
const uint16_t MQTT_PORT   = SECRET_MQTT_PORT;
const char* MQTT_USER      = SECRET_MQTT_USER;
const char* MQTT_PASS      = SECRET_MQTT_PASS;
const char* MQTT_CLIENT_ID = "Nhom16_IoT_ESP32_P21";

// Chung chi goc DigiCert Global Root CA (het han 10/11/2031) de ESP32
// kiem tra dung la may chu EMQX Cloud, chong gia mao broker.
// SHA-256: 43:48:A0:E9:44:4C:78:CB:26:5E:05:8D:5E:89:44:B4:D8:4F:96:62:BD:26:DB:25:7F:89:34:A4:43:C7:01:61
const char* ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

// Ban do Topic chuan hoa 3 kenh (+1 kenh trang thai/LWT)
const char* TOPIC_TELEMETRY = "nhom16/bai21/telemetry"; // Uplink dinh ky (2s)
const char* TOPIC_ALARM     = "nhom16/bai21/alarm";     // Uplink su kien khan
const char* TOPIC_CONTROL   = "nhom16/bai21/control";   // Downlink dieu khien tu xa
const char* TOPIC_STATUS    = "nhom16/bai21/status";    // LWT: online/offline

WiFiClientSecure espClient; // Ket noi TLS (ma hoa) toi broker
PubSubClient mqtt(espClient);

// Bo dem thoi gian non-blocking (luong loop)
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
// 5. DIEU KHIEN CO CAU CHAP HANH (chi goi tu protectTask, tru luc setup)
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
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(PIN_BUZZER, enable ? freq : 0);     // core 3.x (Wokwi web): API theo chan
#else
  ledcWriteTone(BUZZER_CHANNEL, enable ? freq : 0); // core 2.x (PlatformIO): API theo kenh
#endif
}

void setupBuzzer() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BUZZER, 2000, BUZZER_RES);
#else
  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RES);
  ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);
#endif
}

const char* stateName(uint8_t s) {
  return (s == STATE_NORMAL) ? "NORMAL" : (s == STATE_WARNING) ? "WARNING" : "CRITICAL";
}

// Ghi 1 su kien chuyen trang thai (goi trong protectTask)
void logTransition(uint8_t from, uint8_t to, unsigned long now) {
  portENTER_CRITICAL(&dataMux);
  FsmEvent& e = evtBuf[evtHead % EVT_N];
  e.from = from; e.to = to; e.atMs = now; e.p = powerW;
  evtHead++;
  portEXIT_CRITICAL(&dataMux);
}

// ============================================================================
// 6. LUONG BAO VE TAI CHO (FreeRTOS task, doc lap voi mang)
// ============================================================================
void applyRemoteCommand() {
  RemoteCmd cmd = pendingCmd;
  if (cmd == CMD_NONE) return;
  pendingCmd = CMD_NONE;

  if (cmd == CMD_RELAY_ON) {
    manualRelayMode = true;   // Bat co dieu khien thu cong, FSM khong tu dong doi lai
    setRelay(true);
  } else if (cmd == CMD_RELAY_OFF) {
    manualRelayMode = true;   // Chan FSM tu dong dong lai Relay trong trang thai NORMAL
    setRelay(false);
  } else if (cmd == CMD_RESET) {
    manualRelayMode = false;  // Trao toan quyen kiem soat lai cho FSM
    if (currentState != STATE_NORMAL) logTransition(currentState, STATE_NORMAL, millis());
    currentState = STATE_NORMAL;
    alarmSent = false;
    setBuzzer(false);
    setRelay(true);
  }
}

void protectTask(void* param) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    unsigned long now = millis();

    // --- GIAI DOAN 1: Doc cam bien va loc trung binh (moi 10ms) ---
    int rawADC = analogRead(PIN_POT);
    float avgADC = updateMovingAverage(rawADC);
    float p = (avgADC / 4095.0f) * MAX_POWER_SCALE;

    unsigned long dtMs = now - lastEnergyCalcMs;
    lastEnergyCalcMs = now;
    portENTER_CRITICAL(&dataMux);
    powerW = p;
    // Dong dien tinh theo tai AC (chi co dong khi Relay dong)
    currentA = relayState ? (p / VOLTAGE_AC) : 0.0f;
    // Tich luy dien nang tieu thu (chi tich luy khi tai dang co dien)
    if (relayState && p > 0.0f) {
      totalEnergyKWh += ((double)p * (double)dtMs) / (3600000.0 * 1000.0);
    }
    portEXIT_CRITICAL(&dataMux);

    // --- Thuc hien lenh tu xa (neu co) ---
    applyRemoteCommand();

    // --- GIAI DOAN 2A: CHUYEN TRANG THAI FSM ---
    SysState prev = currentState;
    switch (currentState) {
      case STATE_NORMAL:
        if (p > WARN_THRESHOLD_W) {
          currentState = STATE_WARNING;
          warnStartTime = now;
          alarmSent = false;
        }
        break;

      case STATE_WARNING:
        if (p < CLEAR_THRESHOLD_W) {
          currentState = STATE_NORMAL;
        } else if (now - warnStartTime >= TR_DELAY_MS) {
          currentState = STATE_CRITICAL;
        }
        break;

      case STATE_CRITICAL:
        // Chi khi nguoi dung ha cong suat ve duoi 1100W moi tu phuc hoi
        if (p < CLEAR_THRESHOLD_W) {
          currentState = STATE_NORMAL;
          manualRelayMode = false; // Trao lai quyen kiem soat cho FSM
          alarmSent = false;
        }
        break;
    }
    if (currentState != prev) logTransition(prev, currentState, now);

    // --- GIAI DOAN 2B: XUAT DAU RA ACTUATOR THEO TRANG THAI HIEN TAI ---
    switch (currentState) {
      case STATE_NORMAL:
        digitalWrite(PIN_LED, LOW);
        setBuzzer(false);
        // Chi tu dong dong lai Relay khi KHONG dang o che do dieu khien thu cong
        if (!manualRelayMode && !relayState) setRelay(true);
        break;

      case STATE_WARNING:
        digitalWrite(PIN_LED, HIGH); // Bat LED canh bao truoc
        setBuzzer(false);            // Chua hu coi
        if (!manualRelayMode) setRelay(true); // RELAY VAN DONG (chua ngat tai)
        break;

      case STATE_CRITICAL:
        digitalWrite(PIN_LED, HIGH);
        setBuzzer(true, 1500); // Hu coi 1500Hz
        setRelay(false);       // NGAT RELAY BAO VE - khong phu thuoc mang
        if (!alarmSent) {      // Danh dau 1 su kien ALARM; loop se gui khi co mang
          alarmPowerW = p;
          alarmPending = true;
          alarmSent = true;
        }
        break;
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
  }
}

// ============================================================================
// 7. XU LY NHAN LENH DIEU KHIEN TU XA (DOWNLINK CALLBACK - luong loop)
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

  // Chi dat co; protectTask se thuc hien trong vong 10ms
  if (strcmp(cmd, "relay_on") == 0) {
    pendingCmd = CMD_RELAY_ON;
    Serial.println("[DIEU KHIEN] Da bat Relay tu xa.\r\n");
  } else if (strcmp(cmd, "relay_off") == 0) {
    pendingCmd = CMD_RELAY_OFF;
    Serial.println("[DIEU KHIEN] Da tat Relay tu xa.\r\n");
  } else if (strcmp(cmd, "reset") == 0) {
    pendingCmd = CMD_RESET;
    Serial.println("[DIEU KHIEN] Da Reset he thong ve NORMAL.\r\n");
  }
}

// ============================================================================
// 8. QUAN LY KET NOI MANG (luong loop)
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
    return;
  }

  // 2. Duy tri ket noi MQTT Broker
  if (!mqtt.connected()) {
    if (now - lastMqttRetryMs >= RETRY_INTERVAL_MS) {
      lastMqttRetryMs = now;
      Serial.printf("[MQTT] Dang ket noi toi %s...\r\n", MQTT_BROKER);

      // Cau hinh LWT: neu ESP32 mat nguon dot ngot, broker tu publish "offline"
      // Luu y: ham nay co the chan vai giay khi broker loi - chi anh huong luong mang
      // Dang nhap bang tai khoan rieng; broker tu choi neu sai mat khau (ma loi 4/5)
      if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, TOPIC_STATUS, 1, true, "offline")) {
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
// 9. DONG GOI & GUI DU LIEU MQTT JSON (UPLINK)
// ============================================================================
void readSnapshot(float& p, float& i, double& e) {
  portENTER_CRITICAL(&dataMux);
  p = powerW; i = currentA; e = totalEnergyKWh;
  portEXIT_CRITICAL(&dataMux);
}

void publishTelemetry() {
  if (!mqtt.connected()) return;

  float p, i; double e;
  readSnapshot(p, i, e);

  StaticJsonDocument<256> doc;
  doc["U"]     = VOLTAGE_AC;
  doc["I"]     = serialized(String(i, 2));
  doc["P"]     = serialized(String(p, 1));
  doc["E"]     = serialized(String(e, 5));
  doc["relay"] = relayState ? "ON" : "OFF";
  doc["state"] = stateName(currentState);

  char buffer[256];
  size_t bytes = serializeJson(doc, buffer);
  mqtt.publish(TOPIC_TELEMETRY, buffer, bytes);
}

void publishAlarmEvent() {
  StaticJsonDocument<192> doc;
  doc["event"]     = "OVERLOAD_CRITICAL";
  doc["P_trip"]    = serialized(String(alarmPowerW, 1));
  doc["threshold"] = WARN_THRESHOLD_W;
  doc["relay"]     = "OFF";
  doc["msg"]       = "Qua tai keo dai qua 3 giay. Da ngat Relay!";

  char buffer[192];
  size_t bytes = serializeJson(doc, buffer);
  mqtt.publish(TOPIC_ALARM, buffer, bytes);
  Serial.println("[MQTT UPLINK] Da ban su kien ALARM len Broker.\r\n");
}

// In cac lan chuyen trang thai kem thoi diem xay ra (ms ke tu khi khoi dong)
void printFsmEvents() {
  while (evtPrinted != evtHead) {
    if ((uint8_t)(evtHead - evtPrinted) > EVT_N) evtPrinted = evtHead - EVT_N; // bo qua neu bi ghi de
    FsmEvent e;
    portENTER_CRITICAL(&dataMux);
    e = evtBuf[evtPrinted % EVT_N];
    portEXIT_CRITICAL(&dataMux);
    Serial.printf("[FSM] t=%lu ms: %s -> %s (P = %.1f W)\r\n",
                  e.atMs, stateName(e.from), stateName(e.to), e.p);
    evtPrinted++;
  }
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
  setupBuzzer();

  // Khoi tao trang thai ban dau an toan
  digitalWrite(PIN_LED, LOW);
  setBuzzer(false);
  setRelay(true);

  // Cau hinh mang ban dau
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  espClient.setCACert(ROOT_CA); // Chi tin may chu co chung chi do DigiCert Global Root CA ky
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512); // Nang buffer tranh truan goi tin JSON

  lastEnergyCalcMs = millis();

  // Luong bao ve: uu tien cao hon loop (1), chay tren core 1 cung loop
  xTaskCreatePinnedToCore(protectTask, "protect", 4096, NULL, 3, NULL, 1);

  Serial.println("\r\n========================================================");
  Serial.println(" [IoT Nhom 16] - FIRMWARE FSM + MQTT (luong bao ve rieng)");
  Serial.println(" Topics: telemetry | alarm | control | status");
  Serial.println("========================================================\r\n");
}

// ============================================================================
// LOOP (luong mang: khong anh huong toi viec ngat tai)
// ============================================================================
void loop() {
  unsigned long now = millis();

  // GIAI DOAN 3: Xu ly mang & MQTT
  handleNetwork(now);

  // In cac lan chuyen trang thai FSM (truoc khi gui ALARM de log dung thu tu)
  printFsmEvents();

  // Gui su kien ALARM dang cho (dung 1 lan moi lan vao CRITICAL)
  if (alarmPending && mqtt.connected()) {
    alarmPending = false;
    publishAlarmEvent();
  }

  // GIAI DOAN 4: Xuat ban tin dinh ky (moi 2 giay)
  if (now - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;

    publishTelemetry();

    float p, i; double e;
    readSnapshot(p, i, e);
    uint8_t st = currentState;
    const char* stStr = (st == STATE_NORMAL) ? "NORMAL" :
                        (st == STATE_WARNING) ? "WARN" : "CRIT";

    // In Log chuan hoa ra Serial Monitor (co \r\n chong lech dong)
    Serial.printf("[FSM: %-6s] P: %6.1f W | I: %4.2f A | E: %8.5f kWh | Relay: %-3s | WiFi: %s | MQTT: %s\r\n",
                  stStr, p, i, e,
                  relayState ? "ON" : "OFF",
                  (WiFi.status() == WL_CONNECTED) ? "OK" : "NO",
                  mqtt.connected() ? "OK" : "NO");
  }
}
