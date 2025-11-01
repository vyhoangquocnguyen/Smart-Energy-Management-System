#include "mqtt_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include "../tasks/task_common.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector> // Required for std::vector in buffer_flush

// Global variable definitions
extern QueueHandle_t xMeasurementsQueue;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char mqttTopic[128];

/**
 * @brief Ensures SPIFFS is initialized and mounted.
 */
static void ensureSPIFFS(){
    if(!SPIFFS.begin(true))
    {
        log_error("Failed to mount SPIFFS");
    }
}

/**
 * @brief Appends a JSON message to the SPIFFS buffer file if MQTT is down.
 * @param jsonLine The JSON string to save.
 */
static void buffer_append(const String &jsonLine){
    ensureSPIFFS();
    File f = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_APPEND);
    if(!f){
        log_error("Failed to open SPIFFS buffer for append");
        return;
    }
    f.println(jsonLine);
    f.close();
}

/**
 * @brief Attempts to publish messages stored in the SPIFFS buffer.
 * If publishing fails for any message, it stops and re-appends the remaining.
 */
static void buffer_flush(){
    ensureSPIFFS();
    if(!SPIFFS.exists(SPIFFS_BUFFER_FILE)){
        return;
    }
    
    File f = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_READ);
    if(!f){
        log_warn("Failed to open SPIFFS buffer for read");
        return;
    }
    
    // Read all lines into a vector
    std::vector<String> lines;
    while(f.available()){
        String line = f.readStringUntil('\n');
        if(line.length() > 2) lines.push_back(line); // Ignore empty or too short lines
    }
    f.close();
    
    // Truncate file immediately by opening with write access (resets file size to zero)
    File out = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_WRITE);
    if(!out) return;
    out.close();

    // Iterate through lines and publish
    for(auto &ln : lines){
        bool ok = mqttClient.publish(mqttTopic, ln.c_str());
        if(!ok){
            log_warn("Failed to publish from buffer: %s. Re-buffering remaining items.", ln.c_str());
            // Failed to publish, re-append this line and return, leaving the rest for next attempt
            File fw = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_APPEND);
            if(fw){
                fw.println(ln);
                fw.close();
            }
            return;
        }
        // Use vTaskDelay instead of blocking delay()
        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
    log_info("Buffer flushed successfully.");
}

/**
 * @brief Connects to the MQTT broker, blocking until successful.
 */
static void mqtt_connect_blocking(){
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    
    // Keep trying until connected (but yield for other tasks)
    while(!mqttClient.connected()){
        if(WiFi.status()!= WL_CONNECTED){
            log_warn("MQTT: WiFi not connected, waiting...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        log_info("MQTT: connecting to %s:%d", MQTT_BROKER, MQTT_PORT);
        
        if(mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)){
            log_info("MQTT: connected");
            snprintf(mqttTopic, sizeof(mqttTopic), MQTT_TOPIC_FMT, DEVICE_ID);
            buffer_flush(); // Attempt to send buffered messages on successful connection
        }else{
            log_warn("MQTT: connection failed, rc=%d", mqttClient.state());
            vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_DELAY_MS));
        }
    }
}

/**
 * @brief Attempts to publish a JSON string to MQTT.
 * @param json The JSON string.
 * @param len The length of the JSON string.
 * @return true if published, false otherwise.
 */
bool mqtt_publish_json(const char* json, size_t len){
    if(!mqttClient.connected()) return false;
    bool ok = mqttClient.publish(mqttTopic, json, len);
    return ok;
}

/**
 * @brief Main MQTT task. Handles connection, publishing, and buffering.
 */
void MQTTTask(void *pvParameters){
    // Initial setup
    ensureSPIFFS();
    mqtt_connect_blocking();
    
    while(true){
        // Ensure connection is maintained
        if(!mqttClient.connected()){
            mqtt_connect_blocking();
        }
        // Process incoming and outgoing MQTT traffic
        mqttClient.loop();

        // 1. Prepare to receive data (stack allocation for memory safety)
        WindowResult res; 
        
        // 2. Check and receive measurement data from the queue
        if(xMeasurementsQueue != NULL){
            // xQueueReceive copies the data from the queue into the stack struct 'res'
            if(xQueueReceive(xMeasurementsQueue, &res, 500/portTICK_PERIOD_MS) == pdPASS){

                // 3. Serialize the data
                StaticJsonDocument<256> doc;
                char buf[256]; // <-- FIXED: Added missing buffer declaration

                doc["deviceId"] = DEVICE_ID;
                doc["fw"] = FIRMWARE_VERSION;
                doc["voltage"] = res.Vrms;
                doc["current"] = res.Irms;
                doc["power"] = res.Power;
                doc["energy_delta_wh"] = res.EnergyWhDelta;
                doc["timestamp"] = res.window_start_ts; 
                
                size_t len = serializeJson(doc, buf);
                
                // 4. Publish or buffer
                if(mqtt_publish_json(buf, len)){
                    log_info("MQTT: published V=%.1f P=%.2f", res.Vrms, res.Power);
                } else {
                    log_warn("MQTT: publish failed (MQTT disconnected or queue too fast) -> buffering");
                    buffer_append(String(buf));
                }
                
                // No free() needed as 'res' is on the stack and the queue item was a copy.
            }
        }
        
        // Yield to other tasks
        vTaskDelay(1/ portTICK_PERIOD_MS);
    }
}
