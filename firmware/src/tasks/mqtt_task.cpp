#include "mqtt_task.h"
#include "../config.h"
#include "../utils/logger.h"
#include "../tasks/task_common.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

extern QueueHandle_t xMeasurementsQueue;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char mqttTopic[128];

//Helper for SPIFFS buffer
static void ensureSPIFFS(){
    if(!SPIFFS.begin(true))
    {
        log_error("Failed to mount SPIFFS");
    }
}

statid void buffer_append(const String &jsonLine){
static void buffer_append(const String &jsonLine){
    ensureSPIFFS();
    File f = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_APPEND);
    if(!f){
        log_error("Failed to open SPIFFS buffer");
        return;
    }
    f.println(jsonLine);
    f.close();
}static void buffer_flush(){
    ensureSPIFFS();
    if(!SPIFFS.exists(SPIFFS_BUFFER_FILE)){
        return;
    }
    File f = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_READ);
    if(!f){
        log_warn("Failed to open SPIFFS buffer");
        return;
    }
    std::vector<String> lines;
    while(f.available()){
        String line = f.readStringUntil('\n');
        if(line.length() > 2) lines.push_back(line);
    }
    f.close();
    //Truncate file (open with write) and then rewrite and remainning if fail
    File out = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_WRITE);
    if(!out) return;
    out.close();

    for(auto &ln : lines){
        bool ok = mqttClient.publish(mqttTopic, ln.c_str());
        if(!ok){
            log_warn("Failed to publish: %s", ln.c_str());
            File fw = SPIFFS.open(SPIFFS_BUFFER_FILE, FILE_APPEND);
            if(fw){
                fw.println(ln);
                fw.close();
            }
            return;
        }
        delay(5);
    }
    log_info("Buffer flushed");
}

static void mqtt_connect_blocking(){
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
static void mqtt_connect_blocking(){
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    //keep trying until connect (but yeild for other tasks)
    while(!mqttClient.connected()){
        if(WiFi.status()!= WL_CONNECTED){
            log_warn("MQTT: WiFi not connected, watiting...");
            vTaskDelay(2000/ portTICK_PERIOD_MS);
            continue;
        }
        log_info("MQTT: connecting to %s:%d", MQTT_BROKER, MQTT_PORT);
        if(mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)){
            log_info("MQTT: connected");
            snprintf(mqttTopic, sizeof(mqttTopic), MQTT_TOPIC_FMT, DEVICE_ID);
            //flush buffer
            buffer_flush();
        }else{
            log_warn("MQTT: connection failed, rc=%d", mqttClient.state());
            vTaskDelay(MQTT_RECONNECT_DELAY_MS/ portTICK_PERIOD_MS);
        }
    }
    
}bool mqtt_publish_json(const char* json, size_t len){
    if(!mqttClient.connected()) return false;
    bool ok = mqttClient.publish(mqttTopic, json, len);
    return ok;
}void MQTTTask(void *pvParameters){
    ensureSPIFFS();
    mqtt_connect_blocking();
    while(true){
        if(!mqttClient.connected()){
            mqtt_connect_blocking();
        }
        mqttClient.loop();

        WindowResult *resPtr = nullptr;
        if(xMeasurementsQueue != NULL){
            if(xQueueReceive(xMeasurementsQueue, &resPtr, 500/portTICK_PERIOD_MS) == pdPASS){
                if(!resPtr) continue;

                //serialize
                StaticJsonDocument<256> doc;
                doc["deviceId"] = DEVICE_ID;
                doc["fw"] = FIRMWARE_VERSION;
                //serialize
                StaticJsonDocument<256> doc;
                doc["deviceId"] = DEVICE_ID;
                doc["fw"] = FIRMWARE_VERSION;
                doc["voltage"] = resPtr->Vrms;
                doc["current"] = resPtr->Irms;
                doc["power"] = resPtr->Power;
                doc["energy_delta_wh"] = resPtr->EnergyWhDelta;
                doc["timestamp"] = resPtr->window_start_ts;                    log_info("MQTT: published V=%.1f P=%.2f", resPtr->Vrms, resPtr->Power);
                }else{
                    log_warn("MQTT: publish failed -> buffer");
                    buffer_append(String(buf));
                }                }                windowresult_free(resPtr);
            }
        }
        vTaskDelay(1/ portTICK_PERIOD_MS);
    }
}