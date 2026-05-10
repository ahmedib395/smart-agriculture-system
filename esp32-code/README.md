# PlantGuard ESP32 Firmware 🌿

ESP32 firmware for the PlantGuard Smart Irrigation System.

This firmware handles:
- Soil moisture monitoring
- Automatic irrigation logic
- Pump control
- MQTT communication
- Real-time status reporting
- Scheduled watering
- Web dashboard synchronization

The ESP32 acts as the core controller of the entire PlantGuard system.

---

## Features

### Real-Time Soil Monitoring
- Reads soil moisture using an analog moisture sensor
- Uses averaged readings for improved stability
- Converts raw sensor values into percentage-based moisture levels

### Automatic Irrigation
- Auto watering mode based on configurable moisture threshold
- Pump activates when soil becomes too dry
- Threshold adjustable remotely from the dashboard

### Manual Pump Control
Supports remote commands:
- Pump ON
- Pump OFF
- AUTO mode

Commands are received through MQTT and WebSocket communication.

### Smart Scheduling System
Supports:
- Daily watering schedules
- Weekly watering schedules
- One-time irrigation events
- Custom watering duration
- Automatic schedule clearing after one-time runs

### MQTT Communication
Uses secure MQTT over TLS with HiveMQ Cloud.

Publishes:
- Soil moisture
- Pump status
- Irrigation mode
- Schedule information

Subscribes to:
- Pump commands
- Schedule commands
- Threshold updates

### Dashboard Synchronization
The ESP32 continuously sends status packets so the frontend dashboard can:
- Rebuild countdown timers after refresh
- Display live pump status
- Show active schedules
- Track watering activity

### WiFi Auto-Reconnect
- Automatic MQTT reconnection
- WiFi connection recovery
- System restart if WiFi fails during boot

### NTP Time Synchronization
Uses internet time servers to:
- Maintain accurate schedules
- Support weekday scheduling
- Execute irrigation events precisely

---

## Hardware Requirements

### Main Components
- ESP32 Development Board
- Soil Moisture Sensor
- Relay Module
- Water Pump
- External Water Supply

### GPIO Connections

| Component | ESP32 Pin |
|---|---|
| Soil Moisture Sensor | GPIO 35 |
| Relay Module | GPIO 32 |

---

## Libraries Used

Install these libraries from the Arduino Library Manager:

WiFi
WiFiClientSecure
PubSubClient
ArduinoJson
