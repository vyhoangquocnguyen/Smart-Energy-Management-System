#include "wifi_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include <WiFi.h>

static volatile bool wifi_connected = false;

void connectWiFiBlocking(){
    log_info("Wifi: connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while(WiFi.status() != WL_CONNECTED && (millis() -start) < 30000){
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED){
        log_info("WiFi connected IP=%s", WiFi.localIP().toString().c_str());
        wifi_connected = true;
    } else{
        wifi_connected = false;
        log_warn("WiFi connection failed");
    }
}

void WiFiTask(void *pvParameters){
    //try connect immediately
    connectWiFiBlocking();
    while(true){
        if (WiFi.status() != WL_CONNECTED){
            log_warn("Wifi disconnected, retrying...")
            connectWiFiBlocking();
        }
        vTaskDelay(5000/ portTICK_PERIOD_MS);
    }
}

bool WifiisConnected(){
    return(WiFi.status() == WL_CONNECTED);
}