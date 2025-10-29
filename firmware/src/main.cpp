#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "tasks/task_common.h"

void setup(){
    Serial.begin(115200);
    delay(1000);

    Serial.println("Smart Engery Management System - Firmware booting...")

    //Creat FreeRTOS tasks
    xTaskCreatPinnedToCore(WifiTask, "Wifi Task", 4096, NULL, 3, &wifiTaskHandle, 0);
}

void loop(){
    
}