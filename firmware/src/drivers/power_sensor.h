#pragma once
#include <Arduino.h>

struct SamplePair {
  int32_t voltage_raw;
  int32_t current_raw;
  uint32_t ts_us;
};

struct WindowResult {
  float Vrms;
  float Irms;
  float Power;        // W
  float EnergyWhDelta;
  uint32_t window_start_ts;
};

//initialize ADV pins and settings
void power_sensor_init(void);

//blocking capture of SAMPLE_WINDOW_SEC worth of samples into provided buffer
void power_sensor_capture_window(SamplePair *buffer, size_t count);

//compute WindowResult from the buffer (RMS, active power, energy delta)
void power_sensor_compute(SamplePair *buffer, size_t count, WindowResult &wr);