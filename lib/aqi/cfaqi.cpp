#include "cfaqi.h"

#include <math.h>

const char *AqiNames[] = {
    "Good", "Moderate", "UnhealthySG", "Unhealthy", "VeryUnheal.", "Hazardous"};

const char *AqiColors[] = {"Green", "Yellow", "Orange",
                                          "Red",   "Purple", "Marron"};

// From page 11 of:
// https://www.airnow.gov/sites/default/files/2018-05/aqi-technical-assistance-document-may2016.pdf

const float ConcentrationBreakpoints[kAqiLevelsCount + 2] = {
    -0.1f, 12.0f, 35.4f, 55.4f, 150.4f, 250.4f, 350.4f, 500.4f};

const int16_t AqiBreakpoints[kAqiLevelsCount + 2] = {-1,  50,  100, 150,
                                                     200, 300, 400, 500};

bool pm25_to_aqi(float pm, int16_t &aqiValue, AqiLevel &aqiLevel) {
  bool valid = false;
  aqiValue = -1;
  aqiLevel = AqiLevel::OutOfRange;
  int bracket = 0;
  if (pm >= 0.0) {
    for (size_t i = 1; i < kAqiLevelsCount + 2; i++) {
      if (pm <= ConcentrationBreakpoints[i]) {
        valid = true;
        bracket = i;
        break;
      }
    }
  }
  if (pm > ConcentrationBreakpoints[kAqiLevelsCount + 1]) {
    // case completely out of range -> we saturate the value
    aqiValue = 500;
  }
  if (valid) {
    aqiLevel = static_cast<AqiLevel>(bracket - 1);
    int lowI = AqiBreakpoints[bracket - 1] + 1;
    int highI = AqiBreakpoints[bracket];
    float lowC = ConcentrationBreakpoints[bracket - 1] + 0.1f;
    float highC = ConcentrationBreakpoints[bracket];
    aqiValue = roundf(lowI + (highI - lowI) / (highC - lowC) * (pm - lowC));
  }

  return valid;
}

int16_t pm25_to_aqi_value(float pm) {
  AqiLevel level;
  int16_t value;
  pm25_to_aqi(pm, value, level);
  return value;
}
