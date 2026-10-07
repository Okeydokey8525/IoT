#include <Arduino.h>

#define PIN_POT    34
#define PIN_LED    25
#define PIN_BUZZER 26

// PWM Channel cho Buzzer (tranh loi LEDC cua ham tone/noTone)
const int BUZZER_PWM_CH = 0;

// Cac thong so mo phong dien nang
const float VOLTAGE = 220.0f;                   // Dien ap 220V
const float MAX_POWER_SCALE = 2000.0f;          // Cong suat toi da 2000W
const float POWER_OVERLOAD_THRESHOLD = 1200.0f; // Nguong qua tai 1200W
const unsigned long OVERTIME_LIMIT_SEC = 25;    // Chay lien tuc > 25s

// Bien trang thai
unsigned long lastTick = 0;
unsigned long runningSeconds = 0;
unsigned long highPowerDuration = 0;
double totalEnergyKWh = 0.0;
int sampleCount = 0;
bool buzzerPlaying = false;

void setBuzzer(bool on, int freq = 1000) {
  if (on) {
    ledcWriteTone(BUZZER_PWM_CH, freq);
    buzzerPlaying = true;
  } else if (buzzerPlaying) {
    ledcWriteTone(BUZZER_PWM_CH, 0);
    buzzerPlaying = false;
  }
}

void setup() {
  // Khoi tao UART0 chuan
  Serial.begin(115200, SERIAL_8N1, 3, 1);
  delay(1000);

  // Khoi tao LED
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Khoi tao LEDC PWM cho Buzzer
  ledcSetup(BUZZER_PWM_CH, 1000, 8);
  ledcAttachPin(PIN_BUZZER, BUZZER_PWM_CH);
  ledcWriteTone(BUZZER_PWM_CH, 0);

  // In Banner khoi dong dep mat (dung \r\n de tranh loi lech dong)
  Serial.print("\r\n");
  Serial.println("+=============================================================+");
  Serial.println("|      HE THONG GIAM SAT & CANH BAO BAT THUONG DIEN NANG      |");
  Serial.println("|              TRUONG DAI HOC CONG THUONG TP.HCM              |");
  Serial.println("|           Nhom 16: Le Duc Luong - Tran Le Duc Tri           |");
  Serial.println("+=============================================================+\r\n");
  Serial.flush();
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick < 1000) {
    return;
  }
  float dtSec = (now - lastTick) / 1000.0f;
  lastTick = now;
  runningSeconds++;
  sampleCount++;

  // 1. Doc bien tro mo phong tai dien
  int rawADC = analogRead(PIN_POT);
  float power = (rawADC / 4095.0f) * MAX_POWER_SCALE;
  float current = (power > 0.0f) ? (power / VOLTAGE) : 0.0f;

  // 2. Tinh dien nang tich luy (kWh)
  totalEnergyKWh += (power * (dtSec / 3600.0f)) / 1000.0f;

  // 3. Kiem tra bat thuong
  bool isOverload = (power > POWER_OVERLOAD_THRESHOLD);

  if (power > 200.0f) {
    highPowerDuration++;
  } else {
    highPowerDuration = 0;
  }
  bool isOverTime = (highPowerDuration >= OVERTIME_LIMIT_SEC);

  // 4. Xu ly canh bao phan cung
  const char* statusTag;
  const char* alertMessage = "";

  if (isOverload) {
    statusTag = "[!] QUA TAI (OVERLOAD)";
    alertMessage = "P > 1200W! Nguy co chap chay hoac hong thiet bi!";
    digitalWrite(PIN_LED, HIGH);
    setBuzzer(true, 1500);
  } else if (isOverTime) {
    statusTag = "[*] CANH BAO: CHAY QUA LAU";
    alertMessage = "Thiet bi chay > 25s! Kiem tra quen tat thiet bi.";
    if (sampleCount % 2 == 0) {
      digitalWrite(PIN_LED, HIGH);
      setBuzzer(true, 800);
    } else {
      digitalWrite(PIN_LED, LOW);
      setBuzzer(false);
    }
  } else {
    statusTag = "[OK] BINH THUONG";
    digitalWrite(PIN_LED, LOW);
    setBuzzer(false);
  }

  // 5. Dinh dang gio:phut:giay
  int hrs = runningSeconds / 3600;
  int mins = (runningSeconds % 3600) / 60;
  int secs = runningSeconds % 60;

  // 6. In bang giam sat chuan dep (tat ca deu co \r\n de khong bi lech dong)
  Serial.println("+----------------------- BAN TIN GIAM SAT --------------------+");
  Serial.printf("| Thoi gian chay: %02d:%02d:%02d  |  Mau do: #%-5d | ADC: %-4d / 4095 |\r\n", 
                hrs, mins, secs, sampleCount, rawADC);
  Serial.println("+-------------------------------------------------------------+");
  Serial.printf("|  Dien ap nguon (U)     : %6.1f V                             |\r\n", VOLTAGE);
  Serial.printf("|  Dong dien tieu thu (I): %6.2f A                             |\r\n", current);
  Serial.printf("|  Cong suat hien tai (P): %6.1f W  (Max: %.0f W)              |\r\n", power, MAX_POWER_SCALE);
  Serial.printf("|  Dien nang tich luy    : %9.5f kWh                        |\r\n", totalEnergyKWh);
  Serial.println("+-------------------------------------------------------------+");
  Serial.printf("|  TRANG THAI            : %-35s|\r\n", statusTag);
  if (isOverload || isOverTime) {
    Serial.printf("|  CHI TIET CANH BAO     : %-35s|\r\n", alertMessage);
    Serial.println("|  BAO DONG              : COI BUZZER & DEN LED DANG KICH HOAT|");
  }
  Serial.println("+=============================================================+");
  Serial.flush();
}
