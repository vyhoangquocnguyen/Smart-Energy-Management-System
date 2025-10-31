#include "sensor_task.h"
#include "config.h"
#include "../drivers/power_sensor.h"
#include "../utils/logger.h"
#include "task_common.h"
#include <stdlib.h>

QueueHandle_t xMeasurementsQueue = NULL;

WindowResult* windowresult_alloc(){
    WindowResult *p = (WindowResult *)malloc(sizeof(WindowResult));    return p;
}

void windowresult_free(WindowResult *wr){
    if (wr) free(wr);
}

void SensorTask(void *pvParameters){
    //allocate sample buffer of heap
    SamplePair *buffer = (SamplePair *)malloc(sizeof(SamplePair) * SAMPLES_PER_WINDOW);
    if(!buffer){
        log_error("Failed to allocate sample buffer");
        vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskSuspend(NULL);
    }
    
    // create queue if not created yet    if(xMeasurementsQueue == NULL) {
        log_error("Measurements queue not initialized");
        vTaskSuspend(NULL);
    }
//main loop
    while(true){
        //read samples

        power_sensor_capture_window(buffer, SAMPLES_PER_WINDOW);

        power_sensor_capture_window(buffer, SAMPLES_PER_WINDOW);
        
        WindowResult *res = windowresult_alloc();

        if(!res) {
            log_warn("Failed to allocate window result");
            continue;
        }
        power_sensor_compute(buffer, SAMPLES_PER_WINDOW, *res);

        // enqueue pointer (wait briefly if null)
        if(xQueueSend(xMeasurementsQueue, &res, 100/portTICK_PERIOD_MS) != pdPASS){
            log_warn("SensorTask: Queue full, dropping measurement");
            windowresult_free(res);
        } else {
            log_info("SensorTask: measurement queued V=%.1f I=%.3f P=%.2f dE=%.6f", res->Vrms, res->Irms, res->Power, res->EnergyWhDelta);
        }
        //sleep until next window done (capture already took SAMPLE_WINDOW_SEC)
        // A small yield to allow MQTT task to run
        vTaskDelay(1/ portTICK_PERIOD_MS);}