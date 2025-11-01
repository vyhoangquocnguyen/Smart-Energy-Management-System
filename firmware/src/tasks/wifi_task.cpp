#include "wifi_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Must be installed via platformio.ini: tzapu/WiFiManager
#include <WiFiManager.h> 

static WiFiManager wm;

/**
 * @brief Attempts to connect to WiFi using stored credentials, or starts Captive Portal.
 * This runs only once during initial boot.
 */
void connectWiFiBlocking(){
    log_info("WiFi: Starting connection manager. Attempting auto-connect...");
    
    // Set timeout for provisioning (5 minutes)
    wm.setConfigPortalTimeout(300); 

    // autoConnect() attempts connection. If fails, starts AP/Portal named DEVICE_ID.
    if (!wm.autoConnect(DEVICE_ID)) { 
        // This is only reached if the portal times out or the device cannot proceed.
        log_error("WiFi: Provisioning Failed. Rebooting in 10s...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP.restart();
    } else {
        // Successful connection or provisioned successfully.
        log_info("WiFi: Connected/Provisioned. IP=%s", WiFi.localIP().toString().c_str());
    }
}

/**
 * @brief Main WiFi task. Manages connection checks and runs wm.process().
 */
void WiFiTask(void *pvParameters){
    // Initial connection and provisioning happens here
    connectWiFiBlocking();
    
    while(true){
        // Required call to keep the WiFiManager/Captive Portal functionality alive
        wm.process(); 
        
        if (WiFi.status() != WL_CONNECTED){
            log_warn("WiFi disconnected, retrying...");
            // Use standard WiFi.begin() with saved credentials
            WiFi.begin();
        }

        // Wait 5 seconds before checking status again
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

bool WifiisConnected(){
    return(WiFi.status() == WL_CONNECTED);
}
