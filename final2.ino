#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder pins (3-pin encoder)
#define ENCODER_CLK 4
#define ENCODER_DT  5

// Push buttons
#define RESET_BUTTON_PIN 9
#define UNIT_BUTTON_PIN 10

// ADC pin for battery voltage measurement
#define BATTERY_ADC_PIN 3

// Resistor values for voltage divider
const float R1 = 100000.0; // resistor connected to battery +
const float R2 = 100000.0; // resistor connected to GND

// Measurement variables
volatile int encoderPos = 0;
float measurement = 0.0; // cm based
int lastClkState;

// Units enum
enum Unit { CM, METER, FOOT };
Unit currentUnit = CM;

// Debounce
unsigned long lastResetPress = 0;
unsigned long lastUnitPress = 0;
const unsigned long debounceDelay = 200;

// Calibration variables
bool calibrationMode = false;
unsigned long resetButtonHoldStart = 0;
const unsigned long holdThreshold = 3000; // 3 seconds hold for calibration start

// Distance per pulse, default guess value (will be calibrated)
float DIST_PER_PULSE = 0.01; // cm per pulse, adjust with calibration

void setup() {
  Serial.begin(115200);

  // Encoder setup
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  lastClkState = digitalRead(ENCODER_CLK);

  // Buttons setup
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UNIT_BUTTON_PIN, INPUT_PULLUP);

  // ADC pin setup
  analogReadResolution(12); // 12-bit ADC for ESP32-C3

  // OLED setup
  Wire.begin(6, 7); // SDA, SCL for ESP32-C3
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  showSplashScreen();
}

void loop() {
  readEncoder();
  handleButtons();
  displayMeasurement();
  delay(30);
}

// Splash screen
void showSplashScreen() {
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);

  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(4);

  const char* text = "UDxxx";
  int16_t x1, y1;
  uint16_t w, h;
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

// Read rotary encoder
void readEncoder() {
  int clkState = digitalRead(ENCODER_CLK);
  if (clkState != lastClkState && clkState == LOW) {
    if (digitalRead(ENCODER_DT) != clkState) {
      encoderPos++;
    } else {
      encoderPos--;
    }
  }
  lastClkState = clkState;

  if (!calibrationMode) {
    measurement = encoderPos * DIST_PER_PULSE;
  } else {
    // In calibration mode, measurement is pulses count for calibration
    measurement = abs(encoderPos); // count pulses absolute (no negative)
  }
}

// Handle push buttons and calibration mode
void handleButtons() {
  unsigned long currentMillis = millis();

  // Detect long press on Reset button to enter calibration
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (resetButtonHoldStart == 0) resetButtonHoldStart = currentMillis;
    else if (!calibrationMode && currentMillis - resetButtonHoldStart > holdThreshold) {
      calibrationMode = true;
      encoderPos = 0; // reset pulses count
      Serial.println("Entered calibration mode! Roll exactly 100cm then press RESET again.");
      resetButtonHoldStart = currentMillis; // prevent repeated triggers
    }
  } else {
    if (resetButtonHoldStart != 0) {
      // Button released
      if (calibrationMode) {
        // Finish calibration: calculate DIST_PER_PULSE
        if (encoderPos > 0) {
          DIST_PER_PULSE = 100.0 / (float)abs(encoderPos); // 100 cm / pulses
          Serial.print("Calibration done! DIST_PER_PULSE = ");
          Serial.println(DIST_PER_PULSE, 6);
        }
        calibrationMode = false;
        encoderPos = 0;
      } else {
        // Normal reset
        if (currentMillis - lastResetPress > debounceDelay) {
          encoderPos = 0;
          measurement = 0.0;
          Serial.println("Measurement reset");
          lastResetPress = currentMillis;
        }
      }
      resetButtonHoldStart = 0;
    }
  }

  // Unit button change
  if (digitalRead(UNIT_BUTTON_PIN) == LOW) {
    if (currentMillis - lastUnitPress > debounceDelay) {
      if (currentUnit == FOOT) currentUnit = CM;
      else currentUnit = (Unit)((int)currentUnit + 1);

      Serial.print("Unit changed to: ");
      Serial.println(getUnitName());
      lastUnitPress = currentMillis;
    }
  }
}

// Read battery voltage through ADC pin and convert to real voltage
float readBatteryVoltage() {
  int adcRaw = analogRead(BATTERY_ADC_PIN);
  float voltageAtPin = (adcRaw / 4095.0) * 3.3;  // 12-bit ADC
  float batteryVoltage = voltageAtPin * ((R1 + R2) / R2);
  return batteryVoltage;
}

// Convert battery voltage to percentage (3.0V to 4.2V LiPo)
int voltageToPercent(float voltage) {
  if (voltage >= 4.2) return 100;
  else if (voltage <= 3.0) return 0;
  else return (int)((voltage - 3.0) * 100 / (4.2 - 3.0));
}

// Display screen
void displayMeasurement() {
  display.clearDisplay();

  // Header background
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);

  // UD TAPE title (moved left)
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(1, 2);
  display.print("# MEASURING TAPE #");

  // Read and show real battery voltage and percentage
  float battVoltage = readBatteryVoltage();
  int battPercent = voltageToPercent(battVoltage);
  drawBatteryIcon(SCREEN_WIDTH - 18, 2, battPercent);

  // Measurement text
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(0, 21);

  float displayValue = measurement;
  const char* unitStr = "cm";

  if (calibrationMode) {
    display.print("Calib: ");
    display.print((int)measurement);
    display.print(" pulses");
  } else {
    switch (currentUnit) {
      case CM:
        unitStr = "cm";
        break;
      case METER:
        displayValue = measurement / 100.0;
        unitStr = "m";
        break;
      case FOOT:
        displayValue = measurement * 0.0328084;
        unitStr = "ft";
        break;
    }
    display.print(displayValue, 2);
    display.print(" ");
    display.print(unitStr);
  }

  // Bottom buttons
  drawButton(0, SCREEN_HEIGHT - 10, 40, 10, "RESET");
  drawButton(SCREEN_WIDTH - 40, SCREEN_HEIGHT - 10, 40, 10, "UNIT");

  display.display();
}

// Draw battery icon with fill percentage
void drawBatteryIcon(int x, int y, int fillPercent) {
  int w = 14;
  int h = 8;
  display.drawRect(x, y, w, h, SSD1306_BLACK);
  display.fillRect(x + w, y + 2, 2, h - 4, SSD1306_BLACK);
  int fillWidth = (w - 4) * fillPercent / 100;
  fillWidth = constrain(fillWidth, 0, w - 4);
  display.fillRect(x + 2, y + 2, fillWidth, h - 4, SSD1306_BLACK);
}

// Draw white box with label
void drawButton(int x, int y, int w, int h, const char* label) {
  display.fillRect(x, y, w, h, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t tw, th;
  display.getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);
  int textX = x + (w - tw) / 2;
  int textY = y + (h - th) / 2;
  display.setCursor(textX, textY);
  display.print(label);
}

// Get unit name
const char* getUnitName() {
  switch (currentUnit) {
    case CM: return "cm";
    case METER: return "m";
    case FOOT: return "ft";
  }
  return "";
}
