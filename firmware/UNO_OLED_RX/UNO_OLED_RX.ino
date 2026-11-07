#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int LDR_PIN = A2;
const int THRESHOLD = 90;
const unsigned long BIT_US = 40000UL;
const unsigned long SYNC_MS = 1000;
const unsigned long TIMEOUT_MS = 2000;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
char message[81];
byte messageLength = 0;
bool synchronized = false, receiving = false;
bool previousLight = false, timingHigh = false;
unsigned long highSince = 0, lastByteAt = 0;

bool readLight() { return analogRead(LDR_PIN) > THRESHOLD; }
void waitUntil(unsigned long target) {
  while ((long)(micros() - target) < 0) {}
}
bool receiveByte(unsigned long startTime, byte &value) {
  waitUntil(startTime + BIT_US / 2);
  if (readLight()) return false;
  value = 0;
  for (int i = 0; i < 8; ++i) {
    waitUntil(startTime + BIT_US + BIT_US / 2 + (unsigned long)i * BIT_US);
    if (readLight()) value |= (1 << i);
  }
  waitUntil(startTime + 9 * BIT_US + BIT_US / 2);
  return readLight();
}
bool respondsAt(byte address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
void showScreen(const __FlashStringHelper *title, const char *text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.setCursor(0, 16);
  display.print(text);
  display.display();
}
void resetReception() {
  synchronized = receiving = timingHigh = false;
  messageLength = 0;
  previousLight = readLight();
}
void receptionFailed() {
  Serial.println(F("[Reception error]"));
  showScreen(F("Reception error"), "Send again from phone");
  resetReception();
}
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  byte address = 0x3C;
  if (!respondsAt(address)) {
    address = 0x3D;
    if (!respondsAt(address)) {
      Serial.println(F("OLED not detected"));
      while (true) {}
    }
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    Serial.println(F("OLED initialization failed"));
    while (true) {}
  }
  display.setTextWrap(true);
  showScreen(F("Optical Receiver"), "Waiting for message");
  resetReception();
}
void loop() {
  bool light = readLight();
  digitalWrite(LED_BUILTIN, light);
  if (!synchronized) {
    if (light) {
      if (!timingHigh) { highSince = millis(); timingHigh = true; }
      if (millis() - highSince >= SYNC_MS) {
        synchronized = true;
        previousLight = true;
      }
    } else timingHigh = false;
    return;
  }
  if (receiving && millis() - lastByteAt > TIMEOUT_MS) {
    receptionFailed();
    return;
  }
  if (previousLight && !light) {
    unsigned long startTime = micros();
    byte received = 0;
    if (!receiveByte(startTime, received)) { receptionFailed(); return; }
    receiving = true;
    lastByteAt = millis();
    if (received == '\n') {
      message[messageLength] = '\0';
      showScreen(F("Received:"), message);
      Serial.println(message);
      resetReception();
      return;
    }
    if (received != '\r') {
      if (received < 32 || received > 126 || messageLength >= sizeof(message)-1) {
        receptionFailed();
        return;
      }
      message[messageLength++] = (char)received;
    }
    previousLight = true;
    return;
  }
  previousLight = light;
}
