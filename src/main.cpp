#include <Adafruit_NeoPixel.h>
#include <Adafruit_PM25AQI.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <RTCZero.h>

#include "cfaqi.h"
#include "stats.h"

#define NEO_PIXEL_M0_EXPRESS_LINE 8

Adafruit_NeoPixel gNeoStar(1, NEO_PIXEL_M0_EXPRESS_LINE);
Adafruit_PM25AQI gAqiSensor = Adafruit_PM25AQI();
Adafruit_SH1107 gDisplay = Adafruit_SH1107(64, 128, &Wire);
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5

RTCZero gOnboardRtc;

uint32_t gAqiColors[kAqiLevelsCount] = {0x0000FF00, 0x00FFFF00, 0x00FF8800,
                                        0x00FF0000, 0x00FF00FF, 0x00880000};

// In "active" mode, the PMS5003 outputs data every 2.3s when changes are slow, and
// anywhere from 200ms to 800ms when changes are more rapid (at least from the specs!)

// Witch a real time update every 5s, we should guaranty to have at least 2 samples,
// at the very max 25.

// The SAMD21 has plenty of RAM (32K), so at the highest sampling frequency (5Hz), 1 minute (60s)
// would be 300 samples, hence 600 bytes than we can afford

const size_t kNumberOfCachedSamples = 5 * 60 + 20;
uint16_t gPm25CachedSamples[kNumberOfCachedSamples] = {};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  gOnboardRtc.begin();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  gDisplay.begin();
  gDisplay.display();
  gDisplay.setRotation(1);

  // Wait for serial monitor to open
  Serial.begin(115200);
  uint8_t count = 6;
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
  static uint32_t timer5 = 0;
  static uint32_t timer60 = 0;
  static size_t index5 = 0;
  static size_t index60 = 0;

  static uint16_t last_crc = 0;

  static uint16_t pm25EnvMean5s = 0;
  static uint16_t pm25EnvMean60s = 0;
  static int16_t aqiValueMean5s = -1;
  static int16_t aqiValueMean60s = -1;
  static AqiLevel aqiLevelMean5s = AqiLevel::OutOfRange;
  static AqiLevel aqiLevelMean60s = AqiLevel::OutOfRange;


  PM25_AQI_Data data;

  uint8_t count = 20;
  while (count > 0 && !gAqiSensor.read(&data)) {
    count--;
    delay(100);  // try again in a bit!
  }

  if (count > 0) {
    bool updateAvailable = false;
    if (last_crc != data.checksum) {
      gPm25CachedSamples[index60++] = data.pm25_env;
      if (index60 >= kNumberOfCachedSamples) {
        index60 = 0;
        Serial.println("cache is too small: this should not have happened!");
      }
      last_crc = data.checksum;

      uint32_t now = millis();
      uint16_t mae = 0;
      float nmae = 0.0f;
      if (now - timer5 > 5 * 1000) {
        uint16_t pm25EnvMean5s =
            mean_error(index60 - index5, gPm25CachedSamples + index5, mae, nmae);
        pm25_to_aqi(static_cast<float>(pm25EnvMean5s), aqiValueMean5s, aqiLevelMean5s);
        index5 = index60;
        timer5 = now;
        updateAvailable = true;
        if (now - timer60 > 60 * 1000) {
          Serial.print("60s elapsed... index60=");
          Serial.println(index60);
          uint16_t pm25EnvMean60s = mean_error(index60, gPm25CachedSamples, mae, nmae);
          pm25_to_aqi(static_cast<float>(pm25EnvMean60s), aqiValueMean60s, aqiLevelMean60s);
          Serial.println("out of it...");
          timer60 = now;
          index60 = 0;
          index5 = 0;
          updateAvailable = true;
        }
      }

      if (updateAvailable) {
        // Display
        char buffer[32];
        gDisplay.clearDisplay();
        gDisplay.setTextSize(1);
        gDisplay.setTextColor(SH110X_WHITE);
        gDisplay.setCursor(0, 0);
        sprintf(buffer, "pm: 1=%d 2.5=%d 10=%d", data.pm10_env, data.pm25_env, data.pm100_env);
        gDisplay.println(buffer);
        sprintf(buffer, "#: %d 2.5=%d %d",
                data.particles_03um + data.particles_05um + data.particles_10um,
                data.particles_25um, data.particles_50um + data.particles_100um);
        gDisplay.println(buffer);
        gDisplay.setTextSize(2);
        sprintf(buffer, "AQI:%d|%d", aqiValueMean5s, aqiValueMean60s);
        gDisplay.setCursor(0, 24);
        gDisplay.println(buffer);
        sprintf(buffer, "%s", AqiNames[static_cast<int>(aqiLevelMean5s)]);
        gDisplay.setCursor(0, 46);
        gDisplay.println(buffer);
        gDisplay.display();

        gNeoStar.setPixelColor(0, gAqiColors[static_cast<int>(aqiLevelMean60s)]);
        gNeoStar.show();

        Serial.print("PM 2.5 (Env) 5s Mean = ");
        Serial.print(pm25EnvMean5s);
        Serial.print(" ==> AQI(5s) = ");
        Serial.print(aqiValueMean5s);
        Serial.print(" | PM 2.5 (Env) 60s Mean = ");
        Serial.print(pm25EnvMean60s);
        Serial.print(" ==> AQI(60s) = ");
        Serial.print(aqiValueMean60s);
        Serial.print(" % MAE = ");
        Serial.println(mae);
      }
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

}
