# **⚡️ Smart Energy Management System**

## **🎯 Project Goal**

Build a complete IoT system that monitors power usage, logs data to the cloud, and visualizes consumption trends and costs in real time.  
Optional advanced features include billing simulation, power anomaly alerts, and AI-based prediction.

## **✨ Key Highlights**

* **Resilient Embedded Processing:** Utilizes FreeRTOS for stable, concurrent operation, including **Wi-Fi Provisioning** and **SPIFFS buffering** to prevent data loss during network outages.  
* **Centralized Data API:** Data is channeled via MQTT to a Node.js backend for secure storage, analysis, and exposure via a **REST API**.

## **🧱 System Architecture (3-Tier)**

### **1\. Embedded Layer (ESP32 Firmware \- FreeRTOS)**

#### **📟 Function**

* Measure current, voltage, and compute power/energy in real-time.  
* **Provide Captive Portal** for initial Wi-Fi configuration (using WiFiManager).  
* Handle network connection loss gracefully using SPIFFS buffering.

#### **🔧 Hardware**

* **Microcontroller:** ESP32 / ESP8266  
* **Current Sensor:** SCT013 or INA219 (I²C)  
* **Voltage Sensor:** ZMPT101B or derived from INA219  
* **Optional:** OLED display for local readout

#### **🧩 Firmware Modules (Reflecting FreeRTOS Task Separation)**

| Module | Purpose |
| :---- | :---- |
| drivers/power\_sensor.cpp | Implements ADC reading, RMS computation, and energy accumulation. |
| tasks/sensor\_task.cpp | **Producer Task:** Reads data and pushes the result struct onto the FreeRTOS Queue. |
| tasks/mqtt\_task.cpp | **Consumer Task:** Pulls data from the Queue, formats JSON, publishes, and manages SPIFFS buffering. |
| tasks/wifi\_task.cpp | Manages Wi-Fi/MQTT connection state and implements **Wi-FiManager Provisioning**. |
| config.h | Device ID, MQTT topics, calibration constants, and **TEST\_MODE flag**. |

#### **🔁 Features**

* Publishes every 5 seconds to MQTT.  
* **Robust Network Resiliency:** Handles offline buffering using **SPIFFS (Flash File System)**.  
* **Easy Setup:** Implements **Wi-FiManager Captive Portal** for zero-config deployment.  
* Example Published Payload:

{  
  "deviceId": "meter-01",  
  "fw": "v1.0.0-rtos",  
  "voltage": 220.4,  
  "current": 1.34,  
  "power": 295.4,  
  "energy\_delta\_wh": 0.2667,  
  "timestamp": 1739930110  
}

### **2\. Backend Layer (Node.js \+ MongoDB)**

#### **⚙️ Core Features**

* **MQTT subscription** → saves readings to MongoDB.  
* **REST API** for:  
  * /api/devices — list devices  
  * /api/readings?deviceId=\&from=\&to= — fetch readings  
  * /api/alerts — get threshold-based alerts  
* **Data aggregation** for:  
  * Hourly / daily summaries  
  * Cost calculation based on tariff rates

#### **🧩 Folder Structure**

backend/  
┣ src/  
┃ ┣ models/  
┃ ┃ ┗ Reading.js  
┃ ┣ routes/  
┃ ┃ ┣ readings.js  
┃ ┃ ┗ devices.js  
┃ ┣ services/  
┃ ┃ ┣ mqtt.js  
┃ ┃ ┗ analytics.js  
┃ ┣ index.js  
┃ ┗ config.js  
┣ package.json  
┗ README.md

### **💡 Backend Add-ons**

* Predict usage using a moving average or simple regression model.  
* Send email/web alerts if load spikes exceed threshold.  
* Use WebSocket or Socket.io for live frontend updates.

### **3\. Frontend Layer (React \+ Tailwind \+ Recharts)**

#### **🎨 Dashboard Features**

| Component | Function |
| :---- | :---- |
| **Device List** | Displays all connected meters with status |
| **Live Data Card** | Shows real-time voltage/current/power readings |
| **Usage Chart** | Hourly/daily energy consumption graph |
| **Cost Breakdown** | Visualizes cost estimation |
| **Alerts Panel** | Highlights overcurrent/voltage events |
| **Settings Page** | Configure tariff rates and alert thresholds |

#### **🧩 Folder Structure**

frontend/  
┣ src/  
┃ ┣ components/  
┃ ┃ ┣ DeviceList.jsx  
┃ ┃ ┣ LiveCard.jsx  
┃ ┃ ┣ UsageChart.jsx  
┃ ┃ ┗ CostBreakdown.jsx  
┃ ┣ pages/  
┃ ┃ ┣ Dashboard.jsx  
┃ ┃ ┗ Alerts.jsx  
┃ ┣ services/  
┃ ┃ ┗ api.js  
┃ ┣ App.jsx  
┃ ┗ main.jsx  
┣ package.json  
┗ README.md

## **📊 Screens & Data Flow**

#### **🖼 Example Screens**

**Dashboard Page**

* Live cards for voltage, current, power  
* Chart with hourly consumption  
* Alert banner (red when power spike)

**Analytics Page**

* Graph comparing consumption over the week  
* Cost vs time chart

## **🧩 Data Flow Summary**

The data flows from the embedded device through the cloud message queue to the persistent database, and finally to the user interface:

\[ESP32 Sensor Task (Producer)\]  
      ↓ (Data Struct via FreeRTOS Queue)  
\[ESP32 MQTT Task (Consumer)\]  
      ↓ (MQTT Payload / Buffering)  
\[Node.js Backend\]  
      ↓ (Storage & REST API)  
\[MongoDB Time-Series\] \<--- \[Node.js Backend\] \---\> \[React Dashboard\]

## **🚀 Suggested Repo Structure**

smart-energy-system/  
┣ firmware/  
┣ backend/  
┣ frontend/  
┣ README.md  
┗ docs/  
┗ architecture-diagram.png

## **🧭 Project Roadmap**

### **Phase 1 – Hardware \+ Firmware**

* Calibrate voltage/current sensors  
* Implement measurement logic (average RMS)  
* Publish to MQTT topic /energy/\<deviceId\>/data  
* Test with MQTT Explorer

### **Phase 2 – Backend**

* Set up Node.js server \+ MongoDB Atlas  
* Create MQTT subscriber service  
* Store readings in DB with timestamp and device ID  
* Expose REST APIs for readings \+ devices

### **Phase 3 – Frontend**

* Build dashboard layout with Tailwind  
* Fetch and visualize data from backend APIs  
* Show live readings using WebSocket or polling

### **Phase 4 – Enhancement**

* Add cost analytics and configurable tariffs  
* Implement alert system for high load  
* Predictive usage chart (simple moving average)  
* Deploy backend (Render/EC2) \+ frontend (Vercel)
