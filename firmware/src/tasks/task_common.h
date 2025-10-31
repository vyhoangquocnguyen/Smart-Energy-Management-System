#pragma once
#include <Arduino.h>

// Queue that SensorTask will send WindowResult objects to for publishing
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

extern QueueHandle_t xMeasurementsQueue; // holds pointer to allocated WindowResult items

// Helper to allocate WindowResult on heap (publisher will free)
struct WindowResult; // forward
WindowResult* windowresult_alloc();
void windowresult_free(WindowResult* wr);
