#pragma once


// #include <Arduino.h>


#define WIFI_SSID "YOUR WIFI NAME"
#define WIFI_PASSWORD "YOUR WIFI PASSWORD"

//Sampling
#define SAMPLE_RATE_HZ 2000
#define SAMPLE_WINDOW_SEC 1
#define SAMPLES_PER_WINDOW (SAMPLE_RATE_HZ * SAMPLE_WINDOW_SEC)


// ADC pins (ESP32 ADC1 recommended)
#define PIN_VOLTAGE 34
#define PIN_CURRENT 35

// ADC & calibration (defaults are placeholders — calibrate)
#define ADC_MAX_COUNT 4095.0
#define ADC_REF_MV 3300.0           // mV full-scale (approx)
#define VOLTAGE_DIVIDER_RATIO 111.0 // replace with measured ratio
#define CT_BURDEN_OHMS 62.0
#define CT_TURNS 1.0
#define VOLTAGE_ADC_OFFSET 2048.0
#define CURRENT_ADC_OFFSET 2048.0
#define VOLTAGE_SCALE 1.0
#define CURRENT_SCALE 1.0

// Task config
#define WIFI_TASK_STACK 4096
#define SENSOR_TASK_STACK 8192

#define WIFI_TASK_PRIORITY 3
#define SENSOR_TASK_PRIORITY 2