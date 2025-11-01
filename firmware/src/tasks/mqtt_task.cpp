#include "mqtt_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include "../tasks/task_common.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

extern QueueHandle_t xMeasurementsQueue;
static WiFiClient wifiClient; // Note: Use WiFiClientSecure if using MQTT_PORT 8883
static PubSubClient mqttClient(wifiClient);
static char mqttTopic[128];
static char buf[256]; // Buffer for JSON serialization

// Helper for SPIFFS initialization
static void ensureSPIFFS(){
    if(!SPIFFS.begin(true))
    {
        log_error("Failed to mount SPIFFS");
    }
}

/**
 * @brief Appends a JSON line to the SPIFFS buffer file.
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
 * @brief Attempts to publish buffered measurements and truncates the file if successful.
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

    std::vector<String> lines;
    // Read all lines into vector
    while(f.available()){
        String line = f.readStringUntil('\n');
        if(line.length() > 2) lines.push_back(line); 
    }
    f.close();

    // Reopen file with FILE_WRITE (truncates) to clear the buffer before publishing
    File out = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_WRITE);
    if(!out) return;
    out.close();

    int published_count = 0;
    
    for(auto &ln : lines){
        bool ok = mqttClient.publish(mqttTopic, ln.c_str());
        if(!ok){
            log_warn("Failed to publish buffered data. Rewriting to buffer.");
            // Rewrite failed line back to the buffer
            File fw = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_APPEND);
            if(fw){
                fw.println(ln);
                fw.close();
            }
            return; // Stop and return to let the next loop iteration handle remaining items
        }
        published_count++;
        vTaskDelay(pdMS_TO_TICKS(5)); // Small yield
    }
    
    if (published_count > 0) {
        log_info("Buffer flushed: %d item(s) published", published_count);
    }
}

/**
 * @brief Blocking function to ensure MQTT connection is established.
 */
static void mqtt_connect_blocking(){
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    while(!mqttClient.connected()){
        if(WiFi.status()!= WL_CONNECTED){
            log_warn("MQTT: WiFi not connected, waiting...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        
        log_info("MQTT: connecting to %s:%d (non-SSL)", MQTT_BROKER, MQTT_PORT);
        
        if(mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)){
            log_info("MQTT: connected successfully");
            snprintf(mqttTopic, sizeof(mqttTopic), MQTT_TOPIC_FMT, DEVICE_ID);
            
            // Flush buffer immediately upon successful connection
            buffer_flush();
        }else{
            log_warn("MQTT: connection failed, rc=%d. Retrying...", mqttClient.state());
            vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_DELAY_MS));
        }
    }
}

bool mqtt_publish_json(const char* json, size_t len){
    if(!mqttClient.connected()) return false;
    bool ok = mqttClient.publish(mqttTopic, json, len);
    return ok;
}

void MQTTTask(void *pvParameters){
    ensureSPIFFS();
    mqtt_connect_blocking();
    
    while(true){
        if(!mqttClient.connected()){
            mqtt_connect_blocking();
        }
        
        mqttClient.loop();

        WindowResult res; // Struct copied from queue
    
        if(xMeasurementsQueue != NULL){
            // Receive the struct by value with a timeout
            if(xQueueReceive(xMeasurementsQueue, &res, pdMS_TO_TICKS(500)) == pdPASS){

                // Serialize the received data
                StaticJsonDocument<256> doc;
                doc["deviceId"] = DEVICE_ID;
                doc["fw"] = FIRMWARE_VERSION;
                doc["voltage"] = res.Vrms;
                doc["current"] = res.Irms;
                doc["power"] = res.Power;
                doc["energy_delta_wh"] = res.EnergyWhDelta;
                doc["timestamp"] = res.window_start_ts;    
                
                size_t len = serializeJson(doc, buf); 
                
                if(mqtt_publish_json(buf, len)){ 
                    log_info("MQTT: published V=%.1f P=%.2f", res.Vrms, res.Power);
                } else {
                    log_warn("MQTT: publish failed -> buffer");
                    buffer_append(String(buf));
                }
            }
        }
        
        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}
