# ⚡️ Smart Energy Management System

## 🎯 Project Goal
Build a complete **IoT system** that monitors power usage, logs data to the cloud, and visualizes consumption trends and costs in real time.  
Optional advanced features include **billing simulation**, **power anomaly alerts**, and **AI-based prediction**.

---

## 🧱 System Architecture (3-Tier)

### 1. Embedded Layer (ESP32 Firmware)

#### 📟 Function
- Measure current, voltage, and compute power/energy in real time.

#### 🔧 Hardware
- **Microcontroller:** ESP32 / ESP8266  
- **Current Sensor:** SCT013 or INA219 (I²C)  
- **Voltage Sensor:** ZMPT101B or derived from INA219  
- **Optional:** OLED display for local readout

#### 🧩 Firmware Modules

| Module          | Purpose                                  |
|-----------------|-------------------------------------------|
| `sensors.cpp`   | Read voltage/current sensors              |
| `calculation.cpp`| Compute RMS, active power, energy (Wh)   |
| `wifi_mqtt.cpp` | Connect to Wi-Fi + MQTT broker            |
| `main.cpp`      | RTOS tasks, periodic sampling, publishing |
| `config.h`      | Device ID, MQTT topics, calibration constants |

#### 🔁 Features
- Publish every 5 seconds to MQTT:
