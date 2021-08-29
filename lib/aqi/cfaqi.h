#ifndef AAQIM_CFAQI_H
#define AAQIM_CFAQI_H

#include <stdint.h>
#include <stdlib.h>

enum class AqiLevel : int8_t {
  OutOfRange = -1,
  Good = 0,
  Moderate = 1,
  USG = 2,
  Unhealthy = 3,
  VeryUnhealthy = 4,
  Hazardous = 5
};
constexpr size_t kAqiLevelsCount = 6;

extern const char *AqiNames[];
extern const char *AqiColors[];

bool pm25_to_aqi(float pm, int16_t &aqiValue, AqiLevel &aqiLevel);

int16_t pm25_to_aqi_value(float pm);

#endif
