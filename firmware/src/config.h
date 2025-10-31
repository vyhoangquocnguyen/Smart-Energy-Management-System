#pragma once
// ====== config.h ======
// Edit these values for your environment. Do NOT commit secrets in public repos.

#include <Arduino.h>

// Device
#define DEVICE_ID "meter-01"
#define FIRMWARE_VERSION "v1.0.0-rtos"

// WiFi
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"

// MQTT
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 8883  // TLS port
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_TOPIC_FMT "smartenergy/%s/data"
// Sampling
#define SAMPLE_RATE_HZ 2000           // desired samples/sec per channel (software sampling)
#define SAMPLE_WINDOW_SEC 1           // window for RMS/power aggregation
#define SAMPLES_PER_WINDOW (SAMPLE_RATE_HZ * SAMPLE_WINDOW_SEC)

// ADC pins (ESP32 ADC1 recommended)
#define PIN_VOLTAGE 34
#define PIN_CURRENT 35

// ADC & calibration (defaults are placeholders — calibrate)
#define ADC_MAX_COUNT 4095.0
#define ADC_REF_MV 3300.0           // mV full-scale (approx)
#define VOLTAGE_DIVIDER_RATIO 111.0 // example; replace with measured ratio
#define CT_BURDEN_OHMS 62.0
#define CT_TURNS 1.0
#define VOLTAGE_ADC_OFFSET 2048.0
#define CURRENT_ADC_OFFSET 2048.0
#define VOLTAGE_SCALE 1.0
#define CURRENT_SCALE 1.0

// SPIFFS buffering
#define SPIFFS_BUFFER_FILE "/mqtt_buffer.ndjson"
#define SPIFFS_MAX_BYTES (5 * 1024 * 1024)

// Task config
#define WIFI_TASK_STACK 4096
#define SENSOR_TASK_STACK 8192
#define MQTT_TASK_STACK 8192

#define WIFI_TASK_PRIORITY 2
#define SENSOR_TASK_PRIORITY 3
#define MQTT_TASK_PRIORITY 1

// Other
#define PUBLISH_INTERVAL_MS (SAMPLE_WINDOW_SEC * 1000U)
#define MQTT_RECONNECT_DELAY_MS 5000UL
