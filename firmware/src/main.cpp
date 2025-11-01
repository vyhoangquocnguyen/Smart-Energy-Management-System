#include <Arduino.h>
#include "config.h"
#include "tasks/task_common.h"
#include "tasks/wifi_task.h"
#include "tasks/sensor_task.h"
#include "tasks/mqtt_task.h"
#include "drivers/power_sensor.h"
#include "utils/logger.h"
#include "ota.h"

QueueHandle_t xMeasurementsQueue;

void setup() {
  Serial.begin(115200);
  delay(100);
  log_info("Booting %s %s", DEVICE_ID, FIRMWARE_VERSION);

  // init drivers
  power_sensor_init();

  // init OTA
  ota_init();

  // Create the measurement queue (shared)
  xMeasurementsQueue = xQueueCreate(8, sizeof(WindowResult));
  if (xMeasurementsQueue == NULL) {
    log_error("Failed to create measurements queue");
    while (1) { delay(1000); }
  }

  // Create FreeRTOS tasks - pin to cores for deterministic behavior
  BaseType_t ok;

  ok = xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", WIFI_TASK_STACK, NULL, WIFI_TASK_PRIORITY, NULL, 0);
  if (ok != pdPASS) {
    log_error("Failed to create WiFiTask");
    while (1) { delay(1000); }
  }
  ok = xTaskCreatePinnedToCore(SensorTask, "SensorTask", SENSOR_TASK_STACK, NULL, SENSOR_TASK_PRIORITY, NULL, 1);
  if (ok != pdPASS) {
    log_error("Failed to create SensorTask");
    while (1) { delay(1000); }
  }
  ok = xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", MQTT_TASK_STACK, NULL, MQTT_TASK_PRIORITY, NULL, 1);
  if (ok != pdPASS) {
    log_error("Failed to create MQTTTask");
    while (1) { delay(1000); }
  }
  log_info("Tasks created");
}

void loop() {
  // Keep loop light — OTA handled here to allow ArduinoOTA internals to run
  ota_loop();
  delay(100);
}
