⚡️ Smart Energy Management System – Overview
🎯 Project Goal
Build a complete IoT system that monitors power usage, logs data to the cloud, and visualizes consumption trends and costs in real time.
 Optional advanced features include billing simulation, power anomaly alerts, and AI-based prediction.

🧱 System Architecture (3-Tier)
1. Embedded Layer (ESP32 firmware)
📟 Function:
Measure current, voltage, and compute power/energy in real-time.
🔧 Hardware:
ESP32/ESP8266


Current Sensor: SCT013 or INA219 (I²C)


Voltage Sensor: ZMPT101B or derived from INA219


Optional: OLED display for local readout


🧩 Firmware Modules:
Module
Purpose
sensors.cpp
Read voltage/current sensors
calculation.cpp
Compute RMS, active power, energy (Wh)
wifi_mqtt.cpp
Connect to Wi-Fi + MQTT broker
main.cpp
RTOS tasks, periodic sampling, and publishing
config.h
Device ID, MQTT topics, calibration constants

🔁 Features:
Publish every 5 seconds to MQTT:

 {
  "deviceId": "meter-01",
  "voltage": 220.4,
  "current": 1.34,
  "power": 295.4,
  "energy": 1.24,
  "timestamp": 1739930110
}


Handle offline buffering (e.g. using SPIFFS).


OTA firmware update (optional later phase).



2. Backend Layer (Node.js + MongoDB)
⚙️ Core Features:
MQTT subscription → saves readings to MongoDB.


REST API for:


/api/devices — list devices.


/api/readings?deviceId=&from=&to= — fetch readings.


/api/alerts — get threshold-based alerts.


Data aggregation for:


Hourly / daily summaries.


Cost calculation based on tariff rates.


🧩 Folder Structure
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

💡 Extra Ideas:
Predict usage using a moving average (or a simple regression model).


Email/web alert if load spikes > defined threshold.


WebSocket or Socket.io for live frontend updates.



3. Frontend Layer (React + Tailwind + Recharts)
🎨 Dashboard Features:
Component
Function
Device List
Displays all connected meters with status
Live Data Card
Shows real-time V/I/P readings
Usage Chart
Hourly/daily energy consumption graph
Cost Breakdown
Visualizes cost estimation
Alerts Panel
Highlights overcurrent/voltage events
Settings Page
Configure tariff rates and alert thresholds

🧩 Folder Structure
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

🖼 Example Screens:
Dashboard Page


Live cards for voltage, current, power.


Chart with consumption per hour.


Alert banner (red when power spike).


Analytics Page


Graph comparing consumption over the week.


Cost vs time chart.



🧩 Data Flow Summary
[ESP32 + Sensors] → (MQTT) → [Node.js Backend] → (MongoDB)
                                         ↓
                                    [React Dashboard]


🚀 Suggested Repo Structure
smart-energy-system/
 ┣ firmware/
 ┣ backend/
 ┣ frontend/
 ┣ README.md
 ┗ docs/
     ┗ architecture-diagram.png


🧭 Project Roadmap
Phase 1 – Hardware + Firmware
Calibrate voltage/current sensors.


Implement measurement logic (average RMS).


Publish to MQTT topic /energy/<deviceId>/data.


Test with MQTT Explorer.


Phase 2 – Backend
Set up Node.js server + MongoDB Atlas.


Create MQTT subscriber service.


Store readings in DB, with timestamp and device ID.


Expose REST APIs for readings + devices.


Phase 3 – Frontend
Build dashboard layout with Tailwind.


Fetch and visualize data from backend APIs.


Show live readings using WebSocket or polling.


Phase 4 – Enhancement
Add cost analytics and configurable tariffs.


Alert system for high load.


Predictive usage chart (simple moving average).


Deploy backend (Render/EC2) + frontend (Vercel).



🧠 Optional Add-ons (for senior-level wow factor)
Add multi-user support (JWT-based authentication).


Integrate Grafana dashboard via API for pro analytics.


Add mobile PWA support using React.


Integrate Edge AI anomaly detection on ESP32.


