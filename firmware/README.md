# **⚡️ Smart Energy Firmware (ESP32)**

## **⚡️ PHASE 1 GOAL**

✅ Deliver a firmware that can:

* Read voltage and current sensors reliably.  
* Compute power and energy and send the result structure to a **FreeRTOS Queue**.  
* Publish data periodically to the MQTT broker, with the payload formatted as clean JSON.  
* **Handle Wi-Fi provisioning (WiFiManager)** and MQTT/offline resiliency using SPIFFS buffering.

## **🧰 Hardware Setup**

| Component               | Example Model                   | Interface     | Notes                                             |
| :---- | :---- | :---- | :---- |
| **MCU**                 | ESP32 DevKit V1                 | –             | Wi-Fi \+ dual core \+ FreeRTOS                       |
| **Current Sensor**     | SCT-013 (with burden resistor) | Analog (ADC) | Measures current up to \~30 A                       |
| **Voltage Sensor**     | ZMPT101B AC Voltage Module     | Analog (ADC) | Measures line voltage (via transformer isolation) |
| **Display (optional)** | SSD1306 OLED 128×64             | I²C           | For local debug                                   |

Optional Add-ons

* INA219 or ADE7753 IC for digital energy measurement.  
* Relay output for future smart-switch expansion.

## **🧩 Firmware Architecture Overview (FreeRTOS Tasks)**

firmware/  
┣ include/  
┃ ┗ config.h  
┣ src/  
┃ ┣ main.cpp  
┃ ┣ drivers/  
┃ ┃ ┗ power\_sensor.cpp (ADC/Calculation logic)  
┃ ┣ tasks/  
┃ ┃ ┣ sensor\_task.cpp (Producer)  
┃ ┃ ┣ mqtt\_task.cpp (Consumer & Buffering)  
┃ ┃ ┗ wifi\_task.cpp (Connection Management)  
┣ platformio.ini  
┗ README.md

## **⚙️ System Design**

### **1️⃣ Tasks (FreeRTOS Producer/Consumer Pattern)**

| Task                 | Core   | Interval | Function                                 | Role |
| :---- | :---- | :---- | :---- | :---- |
| **SensorDataTask**   | Core 1 | 1 s       | Computes V/I/Power/Energy; **Pushes result to Queue**. | Producer |
| **MqttTask**         | Core 0 | 5 s       | **Pulls from Queue**, formats JSON, publishes, handles **SPIFFS buffer**. | Consumer |
| **WiFiTask**         | Core 0 | 10 s     | Handles **WiFiManager Provisioning**, checks connection status, and reconnects MQTT. | Monitor |
| **DisplayTask**     | Core 1 | 1 s       | Update OLED (optional)                   | Utility |

## **📟 Sensor Reading & Calculation**

### **⚡ Voltage Measurement (ZMPT101B)**

* **Connection:** Output → ADC pin (e.g. GPIO 34\)  
* **Sampling Rate:** \~1 kHz (to capture full 50 Hz AC cycle)  
* Computation:  
     
  $$ V\_{rms} \= \\sqrt{\\frac{1}{N}\\sum\_{i=1}^{N}(V\_i \- V\_{offset})^2} $$  
* Calibration:  
    \- Determine the actual voltage using a multimeter.  
    \- Adjust the calibration constant until the measured RMS value matches the real value.

### **🔌 Current Measurement (SCT-013)**

* **Connection:** Output → ADC pin (e.g. GPIO 35\)  
* **Computation:**

 $$ I\_{rms} \= \\sqrt{\\frac{1}{N}\\sum\_{i=1}^{N}(I\_i \- I\_{offset})^2} $$

* Calibration:  
    \- Use a known load (e.g., 100W bulb) to measure real current via multimeter.  
    \- Derive a calibration factor to convert ADC readings into amperes.

### **🔋 Power & Energy Calculation**

* **Formulas:**

$$  P \= V\_{rms} \\times I\_{rms} $$

$$  E\_{\\text{Wh}} \+= \\frac{P \\times \\Delta t}{3600} $$

* Explanation:  
    \- The SensorDataTask calculates instantaneous power (P) and a differential energy value (\\Delta E\_{\\text{Wh}}) for the last interval.  
* Implementation:  
    \- The WindowResult struct containing these values is posted to the FreeRTOS Queue.

### **🧠 MQTT Communication**

### **Payload Example (Final Structure)**

{  
"deviceId": "meter-01",  
"fw": "v1.0.0-rtos",  
"voltage": 221.3,  
"current": 1.42,  
"power": 314.2,  
"energy\_delta\_wh": 0.087,  
"timestamp": 1739930110  
}

## **📡 Wi-Fi \+ MQTT Resiliency**

### **Data Flow via FreeRTOS Queue**

The tasks communicate using a global FreeRTOS Queue, ensuring thread-safe data transfer between the high-frequency measurement task and the slow, network-dependent publishing task.

| Function | Task | Purpose |
| :---- | :---- | :---- |
| xQueueSend() | SensorDataTask | Used to post a new WindowResult to the queue. |
| xQueueReceive() | MqttTask | Used to retrieve the latest data struct for publishing. |

### **Offline Buffering**

If the MqttTask attempts to publish but the connection fails, it stores the data struct in a file on the **SPIFFS (Flash File System)**. Upon reconnection, the task prioritizes flushing the buffered data before processing new live data from the queue.

### **🧾 Example Pseudocode Flow**

```
// SensorDataTask (The Producer)  
void SensorDataTask(void\* pv) {  
  WindowResult result;  
  while(true) {  
    // 1\. Read ADC, compute Vrms, Irms, Power, Energy  
    // 2\. Populate 'result' struct  
    // 3\. Post to Queue  
    xQueueSend(dataQueueHandle, \&result, portMAX\_DELAY);  
    vTaskDelay(pdMS\_TO\_TICKS(1000));  
  }  
}

// MqttTask (The Consumer)  
void MqttTask(void\* pv) {  
  WindowResult data;  
  while(true) {  
    // 1\. Wait to receive data from the Queue  
    if(xQueueReceive(dataQueueHandle, \&data, portMAX\_DELAY) \== pdPASS) {  
      if(isMqttConnected()) {  
        // 2\. Publish data (and flush buffer if any)  
        publishData(data);  
      } else {  
        // 3\. Connection failed, write to SPIFFS buffer  
        writeToBuffer(data);  
      }  
    }  
    vTaskDelay(pdMS\_TO\_TICKS(5000)); // Publishing happens every 5s loop  
  }  
}

```

### **🧪 Testing Checklist**

| Step               | Verification                                         |
| :---- | :---- |
| ✅ Wi-Fi Provision | Device launches Captive Portal on first boot. |
| ✅ Mock Data Flow   | Verify predictable 240V/960W values in MQTT client.   |
| ✅ JSON Payload     | Valid JSON structure \+ correct fields including fw.   |
| ✅ Data Rate       | Verify publish interval (5 s) via MQTT updates.         |
| ✅ Buffer Test     | Unplug router, data buffers, reconnect, buffer flushes. |

### **📘 Deliverable by End of Phase 1**

| Output                   | Description                                   |
| :---- | :---- |
| firmware/ repo folder | Well-structured PlatformIO project with FreeRTOS. |
| Working hardware         | ESP32 \+ sensors sending live MQTT data         |
| README.md               | Wiring, calibration constants, test results   |
| Screenshot               | MQTT Explorer showing live JSON updates       |

