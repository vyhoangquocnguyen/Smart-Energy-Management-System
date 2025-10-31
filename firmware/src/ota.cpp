#include "ota.h"
#include <ArduinoOTA.h>
#include "config.h"

void ota_init() {
  ArduinoOTA.setHostname(DEVICE_ID);
  // Optionally set password: ArduinoOTA.setPassword("password");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
  });
  ArduinoOTA.begin();
  Serial.println("OTA initialized");
}

void ota_loop() {
  ArduinoOTA.handle();
}
