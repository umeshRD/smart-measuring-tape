#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================ Pins ==================
#define ENCODER_CLK 4
#define ENCODER_DT  5
#define RESET_BUTTON_PIN 9
#define UNIT_BUTTON_PIN 10
#define BATTERY_ADC_PIN 3

// ========== Battery divider ============
const float R1 = 100000.0;
const float R2 = 100000.0;

// ============ Variables ================
volatile int encoderPos = 0;
float measurement = 0.0;
int lastClkState;

enum Unit { CM, METER, FOOT };
Unit currentUnit = CM;

// Buttons
unsigned long lastResetPress = 0;
unsigned long lastUnitPress  = 0;
const unsigned long debounceDelay = 200;

// Calibration
bool calibrationMode = false;
unsigned long resetButtonHoldStart = 0;
const unsigned long holdThreshold = 3000; // 3 s hold

// Distance settings
Preferences prefs;
float wheelCircumference = 7.5;     // fixed wheel size (cm)
float DIST_PER_PULSE = 0.01;        // will be loaded or calibrated

// ================== Setup ==================
void setup() {
  Serial.begin(115200);

  // Encoder pins
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  lastClkState = digitalRead(ENCODER_CLK);

  // Buttons
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UNIT_BUTTON_PIN, INPUT_PULLUP);

  // ADC
  analogReadResolution(12);

  // Load saved calibration
  prefs.begin("tape", false);
  DIST_PER_PULSE = prefs.getFloat("dpp", 0.01);

  // OLED
  Wire.begin(6, 7);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  showSplashScreen();
}

// ================= Loop ==================
void loop() {
  readEncoder();
  handleButtons();
  displayMeasurement();
  delay(30);
}

// ---------- Splash screen ----------
void showSplashScreen() {
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(4);

  const char* text = "SMART MEASURING TAPE";
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, 30, &x1, &y1, &w, &h);
  int targetX = (SCREEN_WIDTH - w) / 2;
  int yPos = (SCREEN_HEIGHT - h) / 2;

  for (int x = SCREEN_WIDTH; x >= targetX; x -= 4) {
    display.clearDisplay();
    display.fillScreen(SSD1306_WHITE);
    display.setCursor(x, yPos);
    display.print(text);
    display.display();
    delay(30);
  }
  delay(1000);
}

// ---------- Encoder ----------
void readEncoder() {
  int clkState = digitalRead(ENCODER_CLK);
  if (clkState != lastClkState && clkState == LOW) {
    if (digitalRead(ENCODER_DT) != clkState) encoderPos++;
    else encoderPos--;
  }
  lastClkState = clkState;

  if (!calibrationMode) measurement = encoderPos * DIST_PER_PULSE;
  else measurement = abs(encoderPos); // show pulses
}

// ---------- Save calibration ----------
void saveDistPerPulse(float value) {
  DIST_PER_PULSE = value;
  prefs.putFloat("dpp", DIST_PER_PULSE);
  Serial.print("Saved DIST_PER_PULSE = ");
  Serial.println(DIST_PER_PULSE, 6);
}

// ---------- Buttons / calibration ----------
void handleButtons() {
  unsigned long now = millis();

  // Long press reset → calibration
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (resetButtonHoldStart == 0) resetButtonHoldStart = now;
    else if (!calibrationMode && now - resetButtonHoldStart > holdThreshold) {
      calibrationMode = true;
      encoderPos = 0;
      Serial.println("Calibration: Roll exactly 10 cm then release RESET.");
      resetButtonHoldStart = now;
    }
  } else {
    if (resetButtonHoldStart != 0) {
      if (calibrationMode) {
        if (encoderPos > 0) {
          float distanceRolled = 10.0; // cm distance used for calibration
          float newDPP = distanceRolled / (float)abs(encoderPos);
          saveDistPerPulse(newDPP);
        }
        calibrationMode = false;
        encoderPos = 0;
      } else {
        if (now - lastResetPress > debounceDelay) {
          encoderPos = 0;
          measurement = 0.0;
          Serial.println("Measurement reset");
          lastResetPress = now;
        }
      }
      resetButtonHoldStart = 0;
    }
  }

  // Unit button
  if (digitalRead(UNIT_BUTTON_PIN) == LOW) {
    if (now - lastUnitPress > debounceDelay) {
      if (currentUnit == FOOT) currentUnit = CM;
      else currentUnit = (Unit)((int)currentUnit + 1);
      Serial.print("Unit changed to: ");
      Serial.println(getUnitName());
      lastUnitPress = now;
    }
  }
}

// ---------- Battery ----------
float readBatteryVoltage() {
  int raw = analogRead(BATTERY_ADC_PIN);
  float vPin = (raw / 4095.0) * 3.3;
  return vPin * ((R1 + R2) / R2);
}
int voltageToPercent(float v) {
  if (v >= 4.2) return 100;
  if (v <= 3.0) return 0;
  return (int)((v - 3.0) * 100 / (4.2 - 3.0));
}

// ---------- Display ----------
void displayMeasurement() {
  display.clearDisplay();

  // Header
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(1, 2);
  display.print("# MEASURING TAPE #");

  float battV = readBatteryVoltage();
  drawBatteryIcon(SCREEN_WIDTH - 18, 2, voltageToPercent(battV));

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(0, 21);

  if (calibrationMode) {
    display.print("Calib: ");
    display.print((int)measurement);
    display.print(" pulses");
  } else {
    float val = measurement;
    const char* unitStr = "cm";
    switch (currentUnit) {
      case CM: val = measurement; break;
      case METER: val = measurement / 100.0; unitStr = "m"; break;
      case FOOT: val = measurement * 0.0328084; unitStr = "ft"; break;
    }
    display.print(val, 2);
    display.print(" ");
    display.print(unitStr);
  }

  drawButton(0, SCREEN_HEIGHT - 10, 40, 10, "RESET");
  drawButton(SCREEN_WIDTH - 40, SCREEN_HEIGHT - 10, 40, 10, "UNIT");
  display.display();
}

void drawBatteryIcon(int x, int y, int pct) {
  int w = 14, h = 8;
  display.drawRect(x, y, w, h, SSD1306_BLACK);
  display.fillRect(x + w, y + 2, 2, h - 4, SSD1306_BLACK);
  int fillW = (w - 4) * pct / 100;
  fillW = constrain(fillW, 0, w - 4);
  display.fillRect(x + 2, y + 2, fillW, h - 4, SSD1306_BLACK);
}

void drawButton(int x, int y, int w, int h, const char* label) {
  display.fillRect(x, y, w, h, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);
  int tx = x + (w - tw) / 2;
  int ty = y + (h - th) / 2;
  display.setCursor(tx, ty);
  display.print(label);
}

const char* getUnitName() {
  switch (currentUnit) {
    case CM: return "cm";
    case METER: return "m";
    case FOOT: return "ft";
  }
  return "" 

