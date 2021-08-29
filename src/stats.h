#ifndef PAQIM_STATS_H
#define PAQIM_STATS_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/** Compute the arithmetic mean of the provided array, together with the Mean
 * Absolute Error (MAE) and the Normalized Mean Absolute Error. (NMAE)*/
template <class T>
T mean_error(size_t size, const T data[], T &mae, float &nmae) {
  T sum = 0;
  for (size_t i = 0; i < size; i++) {
    sum += data[i];
  }
  T mean = sum / static_cast<T>(size);
  float error = 0.0;
  for (size_t i = 0; i < size; i++) {
    error += fabs(data[i] - mean);
  }
  mae = static_cast<T>(error / (float)(size));
  nmae = error / (float)(sum);
  return mean;
}

#endif
