#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder and button pins
const int ENCODER_PIN = 9;       // Encoder signal pin
const int RESET_BUTTON_PIN = 11; // Reset button

// Measurement variables
volatile unsigned long pulseCount = 0; // Counts encoder pulses
float cmPerPulse = 0.1;                // Adjust based on your encoder wheel circumference
float measurement = 0.0;

// Debounce for reset
unsigned long lastResetPress = 0;
const unsigned long debounceDelay = 200;

void IRAM_ATTR encoderISR() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderISR, RISING);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(6, 7); // SDA, SCL for ESP32-C3 Supermini
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  showSplashScreen();
}

void loop() {
  // Calculate measurement in cm
  measurement = pulseCount * cmPerPulse;

  // Handle reset button
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (millis() - lastResetPress > debounceDelay) {
      pulseCount = 0;
      measurement = 0.0;
      lastResetPress = millis();
      Serial.println("Measurement reset");
    }
  }

  // Display measurement
  displayMeasurement();

  delay(100);
}

// Splash screen animation
void showSplashScreen() {
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);

  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(4);

  const char* text = "UD TAPE";
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

// Display measurement and header
void displayMeasurement() {
  display.clearDisplay();

  // Header
  display.fillRect(0, 0, SCREEN_WIDTH, 20, SSD1306_WHITE);
  drawBatteryIcon(2, 6, 75); // 75% battery
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 0);
  display.println("UD TAPE");

  // Measurement display
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(0, 40);
  display.print(measurement, 2);
  display.print(" cm");

  display.display();
}

// Battery icon
void drawBatteryIcon(int x, int y, int fillPercent) {
  int w = 14;
  int h = 8;
  display.drawRect(x, y, w, h, SSD1306_BLACK);
  display.fillRect(x + w, y + 2, 2, h - 4, SSD1306_BLACK);

  int fillWidth = (w - 4) * fillPercent / 100;
  fillWidth = constrain(fillWidth, 0, w - 4);
  display.fillRect(x + 2, y + 2, fillWidth, h - 4, SSD1306_BLACK);
}
