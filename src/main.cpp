#include <Adafruit_NeoPixel.h>
#include <Adafruit_PM25AQI.h>
#include <Arduino.h>
#include <RTCZero.h>

#include "cfaqi.h"
#include "stats.h"

#define NEO_PIXEL_M0_EXPRESS_LINE 8

Adafruit_NeoPixel gNeoStar(1, NEO_PIXEL_M0_EXPRESS_LINE);
Adafruit_PM25AQI gAqiSensor = Adafruit_PM25AQI();
RTCZero gOnboardRtc;

const size_t kSamplesLength = 20;
uint16_t gPm25Env[kSamplesLength] = {};

uint32_t gAqiColors[kAqiLevelsCount] = {0x0000FF00, 0x00FFFF00, 0x00FF8800,
                                        0x00FF0000, 0x00FF00FF, 0x00880000};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  gOnboardRtc.begin();

  // Wait for serial monitor to open
  Serial.begin(115200);
  uint8_t count = 9;
  while (count > 0 && !Serial) {
    count--;
    digitalWrite(LED_BUILTIN, HIGH);
    delay(800);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }

  Serial.println("Adafruit PMSA003I Air Quality Sensor");
  Serial1.begin(9600);

  delay(1000);

  if (!gAqiSensor.begin_UART(&Serial1)) {  // connect to the sensor over hardware serial
    Serial.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
  }

  Serial.println("PM25 found!");

  gNeoStar.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  gNeoStar.clear();  // Set all pixel colors to 'off'

  // Start at 00h00m00s
  gOnboardRtc.setSeconds(0);
  gOnboardRtc.setMinutes(0);
  gOnboardRtc.setHours(0);
}

void loop() {
  static uint32_t last_ms = millis();
  static uint16_t last_crc = 0;
  static size_t index = 0;
  static bool filled = false;

  uint32_t start = millis();
  digitalWrite(13, LED_BUILTIN);  // turn the LED on (HIGH is the voltage level)

  PM25_AQI_Data data;

  uint8_t count = 20;
  while (count > 0 && !gAqiSensor.read(&data)) {
    count--;
    delay(500);  // try again in a bit!
  }
  if (count > 0) {
    if (last_crc != data.checksum) {
      gPm25Env[index++] = data.pm25_env;
      if (index >= kSamplesLength) {
        index = 0;
        filled = true;
      }
      last_crc = data.checksum;
    }
  }

  delay(50);  // Just to let the LED blink long enough...

  uint32_t now = millis();
  if (filled) {
    if (now - last_ms > 6000) {
      uint16_t mae = 0;
      float nmae = 0.0f;
      uint16_t pm25EnvMean = mean_error(kSamplesLength, gPm25Env, mae, nmae);
      int16_t aqiValue(0);
      AqiLevel aqiLevel;
      pm25_to_aqi(static_cast<float>(pm25EnvMean), aqiValue, aqiLevel);
      Serial.print("PM 2.5 (Env): mean = ");
      Serial.print(pm25EnvMean);
      Serial.print(" / MAE = ");
      Serial.print(mae);
      Serial.print(" ==> aqi = ");
      Serial.println(aqiValue);
      gNeoStar.setPixelColor(0, gAqiColors[static_cast<int>(aqiLevel)]);
      gNeoStar.show();
      last_ms = now;
    }
  } else {
    Serial.print("averaging for ");
    Serial.print(kSamplesLength + 1 - index);
    Serial.println("s...");
  }

  digitalWrite(LED_BUILTIN, LOW);

  uint32_t elapsed = millis() - start;
  if (elapsed > 999) {
    Serial.println("Could not read data in 1s!");
  } else {
    delay(1000 - elapsed);
  }
}
