#include "power_sensor.h"
#include "../config.h"

#include <math.h>

/**
 * @brief Initializes ADC and pin configuration.
 */
void power_sensor_init() {
    analogReadResolution(12); // Configure ADC for 0-4095 range (12-bit)
    pinMode(PIN_VOLTAGE, INPUT);
    pinMode(PIN_CURRENT, INPUT);
    // Use 11db attenuation for measuring higher voltages (up to ~3.6V)
    analogSetPinAttenuation(PIN_VOLTAGE, ADC_11db);
    analogSetPinAttenuation(PIN_CURRENT, ADC_11db);
}

/**
 * @brief Reads the raw ADC value from a specified pin.
 * @param pin The analog pin to read.
 * @return The raw 12-bit ADC value (0-4095).
 */
static inline int read_adc_pin(int pin) {
    return analogRead(pin);
}

/**
 * @brief Captures a window of voltage and current samples at a precise frequency.
 * * Uses microsecond timing and taskYIELD() to ensure precise, non-blocking timing
 * for all samples within the window.
 * * @param buffer Pointer to the array where SamplePair data will be stored.
 * @param count The total number of samples to capture (SAMPLES_PER_WINDOW).
 */
void power_sensor_capture_window(SamplePair *buffer, size_t count) {
    // Calculate the required period between samples in microseconds
    const uint32_t period_us = 1000000UL / SAMPLE_RATE_HZ;
    uint32_t next = micros();
    
    for (size_t i = 0; i < count; ++i) {
        // Wait for the next sampling moment, yielding the task if necessary
        while ((int32_t)(micros() - next) < 0) { 
            taskYIELD(); 
        }
        
        next += period_us; // Calculate time for the next sample
        
        // Read samples
        buffer[i].voltage_raw = read_adc_pin(PIN_VOLTAGE);
        buffer[i].current_raw = read_adc_pin(PIN_CURRENT);
        buffer[i].ts_us = micros();
    }
}

/**
 * @brief Computes Vrms, Irms, Active Power, and Energy Delta from a sample window.
 * * @param samples Array of captured SamplePair structs.
 * @param count The number of samples in the array.
 * @param res Reference to the output struct to store results.
 */
void power_sensor_compute(SamplePair *samples, size_t count, WindowResult &res) {
    
    // --- 1. Compute DC Offset (Mean) ---
    // This is the CRITICAL first step to center the AC waveform around zero.
    double v_raw_sum = 0.0;
    double c_raw_sum = 0.0;

    for (size_t i = 0; i < count; ++i) {
        v_raw_sum += (double)samples[i].voltage_raw;
        c_raw_sum += (double)samples[i].current_raw;
    }
    
    double v_mean = v_raw_sum / (double)count;
    double c_mean = c_raw_sum / (double)count;


    // --- 2. Compute Squared Sums and Instantaneous Power Sum ---
    double v_sq_sum = 0.0, c_sq_sum = 0.0, p_sum = 0.0;

    // Pre-calculate conversion factors for efficiency (mV to V/A)
    // V_CONV_FACTOR: Raw count -> AC Voltage (V)
    const double V_CONV_FACTOR = (ADC_REF_MV / ADC_MAX_COUNT) * VOLTAGE_DIVIDER_RATIO * VOLTAGE_SCALE / 1000.0;
    // I_CONV_FACTOR: Raw count -> AC Current (A)
    const double I_CONV_FACTOR = (ADC_REF_MV / ADC_MAX_COUNT) * CURRENT_SCALE / CT_BURDEN_OHMS / CT_TURNS / 1000.0;

    for (size_t i = 0; i < count; ++i) {
        // Get zero-centered (AC only) raw ADC value
        double v_ac_raw = (double)samples[i].voltage_raw - v_mean;
        double c_ac_raw = (double)samples[i].current_raw - c_mean;

        // Convert zero-centered raw reading directly to physical V/A
        double v_line_V = v_ac_raw * V_CONV_FACTOR;
        double i_line_A = c_ac_raw * I_CONV_FACTOR;

        // Accumulate squared values for RMS calculation (V^2 and A^2)
        v_sq_sum += v_line_V * v_line_V;
        c_sq_sum += i_line_A * i_line_A;
        
        // Accumulate instantaneous power (V * I) for Active Power calculation
        p_sum += v_line_V * i_line_A; 
    }

    // --- 3. Final RMS and Power Calculations ---
    
    // Root Mean Square (RMS)
    double Vrms = sqrt(v_sq_sum / (double)count);
    double Irms = sqrt(c_sq_sum / (double)count);
    
    // Active Power (Average instantaneous power)
    double P = p_sum / (double)count; 
    
    // Energy Delta in Watt-hours (Wh) for this window
    double E_wh = (P * (double)SAMPLE_WINDOW_SEC) / 3600.0; // P(W) * T(sec) / 3600

    // --- 4. Store Results ---
    res.Vrms = (float)Vrms;
    res.Irms = (float)Irms;
    res.Power = (float)P;
    res.EnergyWhDelta = (float)E_wh;
    // Store timestamp in seconds
    res.window_start_ts = samples[0].ts_us / 1000000UL; 
}
