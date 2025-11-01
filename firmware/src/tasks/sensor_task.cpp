#include "sensor_task.h"
#include "config.h"
#include "../drivers/power_sensor.h"
#include "../utils/logger.h"
#include "task_common.h"
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Global queue handle is defined as 'extern' in task_common.h and created in main.cpp.
// It holds WindowResult structs by value.
extern QueueHandle_t xMeasurementsQueue;

// The previous memory management functions (windowresult_alloc/free) have been removed.

void SensorTask(void *pvParameters){
    // Allocate sample buffer on the heap for the entire window
    // Size = (Voltage Sample + Current Sample) * SAMPLES_PER_WINDOW (e.g., 2000 samples)
    SamplePair *buffer = (SamplePair *)malloc(sizeof(SamplePair) * SAMPLES_PER_WINDOW);
    if(!buffer){
        log_error("Failed to allocate sample buffer");
        // Critical failure: suspend the task indefinitely
        vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskSuspend(NULL); 
    }
    
    // Check if the measurement queue was initialized by main.cpp
    if(xMeasurementsQueue == NULL) {
        log_error("Measurements queue not initialized");
        vTaskSuspend(NULL);
    }

    log_info("SensorTask initialized. Sample buffer size: %u bytes", sizeof(SamplePair) * SAMPLES_PER_WINDOW);

    // Main loop runs approximately every SAMPLE_WINDOW_SEC (e.g., 1 second)
    while(true){
        
        // 1. Blocking capture of all samples for the window.
        // This function's execution time defines the measurement interval.
        power_sensor_capture_window(buffer, SAMPLES_PER_WINDOW);
        
        // 2. Allocate the result struct on the task's stack.
        // This is safe because it's copied into the queue immediately.
        WindowResult res;
        
        // 3. Compute RMS, Power, and Energy Delta using the captured buffer.
        power_sensor_compute(buffer, SAMPLES_PER_WINDOW, res);

        // 4. Enqueue the struct by value (FreeRTOS copies the data to the queue).
        // Wait up to 100ms if the queue is full before dropping the data.
        if(xQueueSend(xMeasurementsQueue, &res, 100/portTICK_PERIOD_MS) != pdPASS){
            log_warn("SensorTask: Queue full, dropping measurement.");
            // No free() needed as 'res' is on the stack.
        } else {
            log_info("SensorTask: measurement queued V=%.1f I=%.3f P=%.2f dE=%.6f", 
                     res.Vrms, res.Irms, res.Power, res.EnergyWhDelta);
        }
        
        // Give a small yield to allow the MQTT task (or others) to run,
        // especially important if the capture took slightly less than the window time.
        vTaskDelay(1/ portTICK_PERIOD_MS);
    }
}