#include "power_sensor.h"
#include "config.h"
#include <math.h>

void power_sensor_init() {
  analogReadResolution(12); // 0-4095
  pinMode(PIN_VOLTAGE, INPUT);
  pinMode(PIN_CURRENT, INPUT);
  analogSetPinAttenuation(PIN_VOLTAGE, ADC_11db);
  analogSetPinAttenuation(PIN_CURRENT, ADC_11db);
}

static inline int read_adc_pin(int pin) {
  return analogRead(pin);
}

void power_sensor_capture_window(SamplePair *buffer, size_t count) {
  const uint32_t period_us = 1000000UL / SAMPLE_RATE_HZ;
  uint32_t next = micros();
  for (size_t i = 0; i < count; ++i) {
    while ((int32_t)(micros() - next) < 0) { delayMicroseconds(0); }
    next += period_us;
    buffer[i].voltage_raw = read_adc_pin(PIN_VOLTAGE);
    buffer[i].current_raw = read_adc_pin(PIN_CURRENT);
    buffer[i].ts_us = micros();
  }
}

// convert raw ADC (0..4095) to mV
static inline float raw_to_mV(int raw) {
  return (raw / ADC_MAX_COUNT) * ADC_REF_MV;
}

static inline float adc_to_lineVoltage_mV(float adc_mV) {
  // convert measured mV to actual mains voltage (peak, but we compute RMS later)
  // first to volts then apply divider ratio
  return (adc_mV / 1000.0) * VOLTAGE_DIVIDER_RATIO * 1000.0; // returns mV-of-line (but we use V later)
}

static inline float adc_to_current_A(float adc_mV) {
  // voltage across burden resistor -> current = V / Rburden
  float volts = (adc_mV / 1000.0) * CURRENT_SCALE;
  float amps = volts / CT_BURDEN_OHMS / CT_TURNS;
  return amps;
}

void power_sensor_compute(SamplePair *samples, size_t count, WindowResult &res) {
  // compute DC offset of raw readings
  double v_sum = 0.0, c_sum = 0.0;
  for (size_t i = 0; i < count; ++i) {
    v_sum += (double)samples[i].voltage_raw;
    c_sum += (double)samples[i].current_raw;
  }
  double v_mean = v_sum / (double)count;
  double c_mean = c_sum / (double)count;

  double v_sq_sum = 0.0, c_sq_sum = 0.0, p_sum = 0.0;
  for (size_t i = 0; i < count; ++i) {
    double v_ac_raw = (double)samples[i].voltage_raw - v_mean;
    double c_ac_raw = (double)samples[i].current_raw - c_mean;

    // shift to mid-rail (approx) for converting to mV - using ADC offset macros
    double v_adc = raw_to_mV((int)round(v_ac_raw + VOLTAGE_ADC_OFFSET));
    double c_adc = raw_to_mV((int)round(c_ac_raw + CURRENT_ADC_OFFSET));

    // convert to physical quantities
    double v_line_V = (v_adc / 1000.0) * VOLTAGE_DIVIDER_RATIO * VOLTAGE_SCALE;
    double i_line_A = (c_adc / 1000.0) * CURRENT_SCALE / CT_BURDEN_OHMS / CT_TURNS;

    v_sq_sum += v_line_V * v_line_V;
    c_sq_sum += i_line_A * i_line_A;
    p_sum += v_line_V * i_line_A;
  }

  double Vrms = sqrt(v_sq_sum / (double)count);
  double Irms = sqrt(c_sq_sum / (double)count);
  double P = p_sum / (double)count;
  double E_wh = (P * (double)SAMPLE_WINDOW_SEC) / 3600.0;

  res.Vrms = (float)Vrms;
  res.Irms = (float)Irms;
  res.Power = (float)P;
  res.EnergyWhDelta = (float)E_wh;
  res.window_start_ts = samples[0].ts_us / 1000000UL;
}
