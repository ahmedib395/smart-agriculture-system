# PlantGuard 🌿
## Smart Irrigation & Remote Plant Monitoring System

PlantGuard is a full-stack IoT smart irrigation system built using an ESP32, MQTT communication, a Node.js backend bridge, and a real-time web dashboard. The system monitors soil moisture in real-time and controls a water pump remotely or automatically based on configurable thresholds and weekly irrigation schedules.

---

## Features

- 🌱 Real-time soil moisture monitoring
- 💧 Manual, automatic, and scheduled pump control
- ⏰ Smart weekly irrigation scheduling with day/time/duration configuration
- 📊 Live moisture history chart (last 12 readings)
- 📡 MQTT + WebSocket real-time communication
- 🌐 Locally-hosted web dashboard
- 🔄 WebSocket auto-reconnect logic
- 📱 Responsive modern UI

---

## System Architecture

```text
┌─────────────────┐      MQTT (TLS)    ┌─────────────────┐
│                 │ ──────────────────► │                 │
│   ESP32 Device  │                     │  HiveMQ Cloud   │
│ (Sensor + Pump) │ ◄────────────────── │  MQTT Broker    │
│                 │      MQTT (TLS)     │                 │
└─────────────────┘                     └────────┬────────┘
                                                 │
                                                 ▼
                                        ┌─────────────────┐
                                        │ Node.js Backend │
                                        │ MQTT ↔ WS Bridge│
                                        │  localhost:3000  │
                                        └────────┬────────┘
                                                 │ WebSocket
                                                 ▼
                                        ┌─────────────────┐
                                        │ Frontend Web UI │
                                        │  (index.html)   │
                                        │  opened locally │
                                        └─────────────────┘
```

---

## How It Works

The ESP32 reads soil moisture every 3 seconds and publishes readings to HiveMQ Cloud over MQTT. The Node.js backend subscribes to those MQTT topics and forwards all data to the browser over a WebSocket connection. Commands from the dashboard (pump on/off, schedules) travel the reverse path: browser → WebSocket → backend → MQTT → ESP32.

The frontend is a plain HTML file opened directly in the browser — no web server needed for the frontend itself. The backend runs locally on your machine.

---

## Components

### ESP32 Firmware
- Reads soil moisture sensor (averaged over 10 samples for noise reduction)
- Controls relay/pump based on commands or thresholds
- Executes weekly irrigation schedules with NTP time sync (UTC+2 Egypt)
- Publishes full status JSON to MQTT every 3 seconds
- Subscribes to pump command topic for remote control

**Libraries required:**
- PubSubClient
- ArduinoJson
- WiFiClientSecure

---

### Backend Server (Node.js)
Acts as a bridge between HiveMQ Cloud (MQTT) and the browser (WebSocket).

- Connects to HiveMQ over MQTTS (port 8883)
- Subscribes to moisture and status topics
- Caches last known status — new dashboard connections immediately receive current state
- Forwards pump commands and schedule commands from the browser to ESP32 via MQTT
- Runs on **localhost:3000**

**Technologies:** Node.js, Express, MQTT.js, ws, dotenv

---

### Frontend Dashboard
A single `index.html` file opened directly in the browser. Connects to the backend WebSocket at `ws://localhost:3000` (or your deployed backend URL).

**Features:**
- Live soil moisture percentage + color-coded status bar
- Pump ON / OFF / AUTO mode buttons
- Auto-mode threshold slider
- Weekly schedule: pick day (or every day), this week / next week toggle, time, duration, repeat mode
- Countdown timer with days+hours display (e.g. `5d 03:22:11`)
- Moisture history line chart (last 12 readings)
- System info panel: last sensor reading time, watering count today, next scheduled run
- ESP32 connection tracking

**Technologies:** HTML5, CSS3, Vanilla JavaScript, Chart.js

---

## Hardware Requirements

| Component | Description |
|---|---|
| ESP32 Development Board | Main microcontroller |
| Capacitive Soil Moisture Sensor | Analog moisture readings (GPIO 35) |
| Relay Module | Switches the water pump |
| Water Pump | Performs irrigation |
| Power Supply | Matched to pump voltage |

---

## GPIO Connections

| Component | ESP32 Pin |
|---|---|
| Soil Moisture Sensor (Signal) | GPIO 35 |
| Relay Module (IN) | GPIO 32 |

---

## Sensor Calibration

The firmware maps raw ADC values to a 0–100% moisture percentage:

| State | Raw ADC Value |
|---|---|
| Dry (0%) | 3500 |
| Wet (100%) | 1500 |

Adjust `DRY_VALUE` and `WET_VALUE` in the firmware to match your specific sensor.

---

## Installation

### 1. Clone Repository

```bash
git clone https://github.com/ahmedib395/smart-agriculture-system.git
cd smart-agriculture-system
```

---

### 2. Backend Setup

```bash
cd backend
npm install
```

Create a `.env` file:

```env
PORT=3000
MQTT_URL=mqtts://your-hivemq-cluster-url:8883
MQTT_USER=your_username
MQTT_PASS=your_password
```

Run the server:

```bash
node server.js
```

The backend starts on **http://localhost:3000**. Keep this running while using the dashboard.

---

### 3. ESP32 Setup

Install the following Arduino libraries via Library Manager:
- PubSubClient
- ArduinoJson

Create a `secrets.h` file in the `esp32-code/` folder:

```cpp
#pragma once

const char* WIFI_SSID     = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";

const char* MQTT_SERVER = "your-hivemq-cluster-url";
const int   MQTT_PORT   = 8883;

const char* MQTT_USER = "your_mqtt_username";
const char* MQTT_PASS = "your_mqtt_password";
```

Upload `plantguard.ino` to your ESP32.

---

### 4. Frontend Setup

No server needed. Just open `frontend/index.html` directly in your browser.

Make sure the backend is running first so the WebSocket connection succeeds.

---

## MQTT Topics

### Published by ESP32

| Topic | Content |
|---|---|
| `plantguard_x7k92mf/status` | Full JSON status (moisture, pump state, mode, schedule info) |
| `plantguard_x7k92mf/moisture` | Raw moisture percentage (plain text) |

### Subscribed by ESP32

| Topic | Commands accepted |
|---|---|
| `plantguard_x7k92mf/pump` | `pump_on`, `pump_off`, `auto_mode`, or JSON schedule commands |

### Schedule Command Format (JSON via pump topic)

```json
{ "cmd": "set_schedule", "day": 6, "hour": 7, "minute": 30, "duration": 300, "repeat": "weekly" }
{ "cmd": "clear_schedule" }
```

`day`: -1 = every day, 0 = Sunday … 6 = Saturday  
`duration`: seconds

---

## Scheduling

The dashboard supports weekly or daily irrigation schedules:

- Pick a **specific day** (Sun–Sat) or **Every Day**
- When a specific day is selected, choose **This Week** or **Next Week** to disambiguate
- Set **start time** and **duration** in minutes
- Choose **Once** or **Weekly** repeat mode
- Countdown displays as `Xd HH:MM:SS` when more than 24 hours remain

Schedule state is echoed back in the ESP32 status JSON, so the countdown survives a page refresh.

---

## Project Structure

```text
smart-agriculture-system/
│
├── esp32-code/
│   ├── plantguard.ino       # Main firmware
│   ├── secrets.h            # WiFi + MQTT credentials (not committed)
│   └── README.md
│
├── backend/
│   ├── server.js            # MQTT ↔ WebSocket bridge
│   ├── package.json
│   ├── .env                 # Backend credentials (not committed)
│   └── README.md
│
├── frontend/
│   ├── index.html           # Full dashboard (open directly in browser)
│   └── README.md
│
└── README.md
```

---

## Security Notes

- Never commit `secrets.h` or `.env` to version control
- Add both to `.gitignore`
- The ESP32 uses TLS (`WiFiClientSecure`) but skips certificate verification (`setInsecure()`) — acceptable for private/home use
- HiveMQ Cloud credentials should be rotated if ever exposed

---

## Future Improvements

- Multi-plant / multi-zone support
- Database logging for historical moisture data
- Mobile app (React Native / Flutter)
- Push notifications when soil is critically dry
- OTA firmware updates
- Weather API integration to skip watering after rain
- AI-based irrigation optimization

---

## License

MIT License

---

Developed as the PlantGuard Smart Agriculture IoT Project 🌱
