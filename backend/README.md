# PlantGuard Backend Server 🌐
## MQTT ↔ WebSocket Bridge for Smart Irrigation System

This backend server acts as the communication bridge between the ESP32 irrigation controller and the PlantGuard web dashboard.

The server receives real-time sensor and status data from the ESP32 through MQTT and instantly forwards it to connected frontend clients using WebSockets.

It also receives dashboard commands and publishes them back to the ESP32 through MQTT.

---

# Features

## Real-Time Communication
- MQTT communication with ESP32
- WebSocket communication with frontend dashboard
- Instant data synchronization

## Live Sensor Updates
Receives and forwards:
- Soil moisture readings
- Pump status
- Irrigation mode
- Scheduling data

## Remote Pump Control
Supports dashboard commands:
- Pump ON
- Pump OFF
- AUTO mode

Commands are sent directly to the ESP32 through MQTT.

## Cloud MQTT Integration
Uses HiveMQ Cloud with secure MQTT over TLS.

## Multi-Client WebSocket Support
Multiple dashboard clients can connect simultaneously and receive live updates in real time.

## CORS Enabled
Allows frontend applications hosted on different domains to communicate with the backend.

---

# System Architecture

```text
ESP32
   │
   │ MQTT
   ▼
HiveMQ Cloud
   │
   ▼
Node.js Backend Server
   │
   │ WebSocket
   ▼
Frontend Dashboard
