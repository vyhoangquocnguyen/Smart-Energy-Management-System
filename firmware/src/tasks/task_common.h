#pragma once
#include <Arduino.h>

// Queue that SensorTask will send WindowResult objects to for publishing
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../drivers/power_sensor.h" 


extern QueueHandle_t xMeasurementsQueue;

