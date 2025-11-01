#include "wifi_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Attempts to connect to WiFi, yielding the task during the process.
 * IMPORTANT: Uses vTaskDelay() instead of Arduino's delay() to remain non-blocking.
 */
void connectWiFiBlocking(){
    log_info("WiFi: connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Use vTaskDelay() to yield control while waiting for connection
    uint32_t attempts = 0;
    const uint32_t max_attempts = 60; // Max 30 seconds (60 * 500ms)
    
    while(WiFi.status() != WL_CONNECTED && attempts < max_attempts){
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED){
        log_info("WiFi connected IP=%s", WiFi.localIP().toString().c_str());
    } else{
        log_warn("WiFi connection failed after %u attempts.", attempts);
        WiFi.disconnect(); // Explicitly disconnect if failure to ensure clean retry later
    }
}

/**
 * @brief Main WiFi task. Manages initial connection and reconnections.
 * This task is pinned to Core 0.
 * @param pvParameters Standard FreeRTOS task parameter (unused).
 */
void WiFiTask(void *pvParameters){
    // Try to connect immediately
    connectWiFiBlocking();
    
    while(true){
        // Check status and attempt reconnection if needed
        if (WiFi.status() != WL_CONNECTED){
            log_warn("WiFi disconnected, retrying...");
            connectWiFiBlocking();
        }
        
        // Wait 5 seconds before checking status again
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Public function to check the current WiFi connection status.
 * @return true if connected, false otherwise.
 */
bool WifiisConnected(){
    return(WiFi.status() == WL_CONNECTED);
}
