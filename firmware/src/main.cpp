#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "tasks/task_common.h"

void setup(){
    Serial.begin(115200);
    delay(1000);

    Serial.println("Smart Engery Management System - Firmware booting...")

    //Creat FreeRTOS tasks
    xTaskCreatPinnedToCore(WifiTask, "Wifi Task", WIFI_TASK_STACK, NULL, WIFI_TASK_PRIORITY, NULL , 0);
    xTaskCreatePinnedToCore(SensorTask, "SensorTask", SENSOR_TASK_STACK, NULL, SENSOR_TASK_PRIORITY, NULL, 1)
}

void loop(){
    
}