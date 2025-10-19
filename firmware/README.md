⚡️ PHASE 1 GOAL

✅ Deliver a firmware that can:

Read voltage and current sensors reliably.

Compute instantaneous power and accumulate energy (Wh).

Publish data periodically to the MQTT broker.

Handle Wi-Fi reconnection and MQTT resiliency.

Provide optional local debugging (OLED or Serial).

🧰 Hardware Setup

| Component              | Example Model                  | Interface    | Notes                                             |
| ---------------------- | ------------------------------ | ------------ | ------------------------------------------------- |
| **MCU**                | ESP32 DevKit V1                | –            | Wi-Fi + dual core + FreeRTOS                      |
| **Current Sensor**     | SCT-013 (with burden resistor) | Analog (ADC) | Measures current up to ~30 A                      |
| **Voltage Sensor**     | ZMPT101B AC Voltage Module     | Analog (ADC) | Measures line voltage (via transformer isolation) |
| **Display (optional)** | SSD1306 OLED 128×64            | I²C          | For local debug                                   |

Optional Add-ons

- INA219 or ADE7753 IC for digital energy measurement.

- Relay output for future smart-switch expansion.

## 🧩 Firmware Architecture Overview

```
firmware/
┣ include/
┃ ┗ config.h
┣ src/
┃ ┣ main.cpp
┃ ┣ sensors.cpp
┃ ┣ wifi_mqtt.cpp
┃ ┣ calculations.cpp
┃ ┗ display.cpp (optional)
┣ platformio.ini (or Arduino IDE project)
┗ README.md
```

## ⚙️ System Design

### 1️⃣ Tasks (FreeRTOS)

| Task                | Core   | Interval | Function                                 |
| ------------------- | ------ | -------- | ---------------------------------------- |
| **SensorTask**      | Core 1 | 100 ms   | Read ADC samples, compute RMS            |
| **CalcTask**        | Core 1 | 1 s      | Compute power = V × I, accumulate energy |
| **MqttTask**        | Core 0 | 5 s      | Publish JSON to MQTT broker              |
| **WiFiMonitorTask** | Core 0 | 10 s     | Check Wi-Fi + MQTT connection            |
| **DisplayTask**     | Core 1 | 1 s      | Update OLED (optional)                   |

## 📟 Sensor Reading & Calculation

### ⚡ Voltage Measurement (ZMPT101B)

- **Connection:** Output → ADC pin (e.g. `GPIO 34`)
- **Sampling Rate:** ~1 kHz (to capture full 50 Hz AC cycle)
- **Computation:**
  \[
  V*{rms} = \sqrt{\frac{1}{N}\sum*{i=1}^{N}(V*i - V*{offset})^2}
  \]
- **Calibration:**
  - Determine the actual voltage using a multimeter.
  - Adjust the calibration constant until the measured RMS value matches the real value.
- **Notes:**
  - The ZMPT101B outputs a low-voltage AC signal proportional to the line voltage.
  - Ensure proper isolation and scaling to stay within the ADC 0–3.3V range.

---

### 🔌 Current Measurement (SCT-013)

- **Connection:** Output → ADC pin (e.g. `GPIO 35`)
- **Computation:**
  \[
  I*{rms} = \sqrt{\frac{1}{N}\sum*{i=1}^{N}(I*i - I*{offset})^2}
  \]
- **Calibration:**
  - Use a known load (e.g., 100W bulb) to measure real current via multimeter.
  - Derive a calibration factor to convert ADC readings into amperes.
- **Notes:**
  - The SCT-013 is a non-invasive current transformer.
  - Include a **burden resistor** (typically 62Ω to 100Ω) to convert current into voltage.
  - Ensure the output voltage is within the ESP32 ADC input range.

---

### 🔋 Power & Energy Calculation

- **Formulas:**
  \[
  P = V*{rms} \times I*{rms}
  \]
  \[
  E\_{Wh} += \frac{P \times \Delta t}{3600}
  \]
- **Explanation:**
  - `P` represents **instantaneous power** (in watts).
  - `E_Wh` accumulates **energy consumption** over time (in watt-hours).
  - `Δt` is the time between calculations (e.g., 1 second).
- **Implementation:**
  - Store accumulated energy in **RTC memory** to preserve data across soft resets.
  - Optionally save to EEPROM or SPIFFS for long-term persistence.
- **Example Output:**
  ```json
  {
    "voltage": 220.4,
    "current": 1.34,
    "power": 295.4,
    "energy": 1.24
  }
  ```

## 🧠 MQTT Communication

### Broker Setup

| Parameter         | Example                                       |
| ----------------- | --------------------------------------------- |
| Host              | `mqtt://broker.hivemq.com` or local Mosquitto |
| Port              | `1883`                                        |
| Username/Password | Optional                                      |
| Publish Topic     | `energy/<deviceId>/data`                      |
| Retain Flag       | false                                         |

### Payload Example

{
"deviceId": "meter-01",
"voltage": 221.3,
"current": 1.42,
"power": 314.2,
"energy": 2.84,
"timestamp": 1739930110
}

## 📡 Wi-Fi + MQTT Resiliency

Use WiFiClientSecure if TLS required.

Auto-reconnect logic:

```cpp
if (WiFi.status() != WL_CONNECTED) reconnectWiFi();
if (!mqttClient.connected()) reconnectMQTT();
```

Store unsent packets in a queue (or SPIFFS if offline > 30 s).

### 🧾 Example Pseudocode Flow

```cpp
void setup() {
  initWiFi();
  initMQTT();
  initSensors();
  xTaskCreatePinnedToCore(SensorTask, ...);
  xTaskCreatePinnedToCore(CalcTask, ...);
  xTaskCreatePinnedToCore(MqttTask, ...);
}

void loop() {
  // FreeRTOS handles all tasks; no blocking code here
}

```

SensorTask

```cpp
void SensorTask(void* pv) {
  while(true) {
    readVoltageSamples();
    readCurrentSamples();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

CalcTask

```cpp
void CalcTask(void* pv) {
  while(true) {
    float Vrms = computeVrms();
    float Irms = computeIrms();
    float Power = Vrms * Irms;
    Energy_Wh += Power * 1.0 / 3600.0; // every second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

MqttTask

```cpp
void MqttTask(void* pv) {
  while(true) {
    if(mqttClient.connected()) {
      StaticJsonDocument<256> doc;
      doc["deviceId"] = DEVICE_ID;
      doc["voltage"] = Vrms;
      doc["current"] = Irms;
      doc["power"] = Power;
      doc["energy"] = Energy_Wh;
      doc["timestamp"] = now();
      char buffer[256];
      serializeJson(doc, buffer);
      mqttClient.publish(MQTT_TOPIC, buffer);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
```

### 🧪 Testing Checklist

| Step               | Verification                                         |
| ------------------ | ---------------------------------------------------- |
| ✅ ADC Calibration | Compare raw sensor output to known multimeter values |
| ✅ MQTT Connection | Confirm in MQTT Explorer / broker logs               |
| ✅ JSON Payload    | Valid JSON structure + correct fields                |
| ✅ Data Rate       | Verify publish interval (5 s)                        |
| ✅ Power Accuracy  | ±5 % acceptable for demo                             |

### 📘 Deliverable by End of Phase 1

| Output                  | Description                                   |
| ----------------------- | --------------------------------------------- |
| `firmware/` repo folder | Well-structured PlatformIO or Arduino project |
| Working hardware        | ESP32 + sensors sending live MQTT data        |
| README.md               | Wiring, calibration constants, test results   |
| Screenshot              | MQTT Explorer showing live JSON updates       |
