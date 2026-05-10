# PlantGuard 🌿
## Smart Irrigation & Remote Plant Monitoring System

PlantGuard is a full-stack IoT smart irrigation system built using an ESP32, MQTT communication, a Node.js backend bridge deployed on Railway, and a real-time web dashboard. The system monitors soil moisture in real-time and controls a water pump remotely or automatically based on configurable thresholds and weekly irrigation schedules.

---

## Features

- 🌱 Real-time soil moisture monitoring
- 💧 Manual, automatic, and scheduled pump control
- ⏰ Smart weekly irrigation scheduling with day/time/duration configuration
- 📊 Live moisture history chart (last 12 readings)
- 📡 MQTT + WebSocket real-time communication
- 🌐 Cloud-connected web dashboard (backend hosted on Railway)
- 🔄 WebSocket auto-reconnect logic
- 📱 Responsive modern UI

---

## System Architecture

```text
┌─────────────────┐    MQTT/TLS (8883)   ┌─────────────────┐
│                 │ ──────────────────►  │                 │
│   ESP32 Device  │                      │  HiveMQ Cloud   │
│ (Sensor + Pump) │ ◄──────────────────  │  MQTT Broker    │
│                 │    MQTT/TLS (8883)   │                 │
└─────────────────┘                      └────────┬────────┘
                                                  │
                                                  ▼
                                         ┌─────────────────────────────────┐
                                         │        Node.js Backend          │
                                         │       MQTT ↔ WS Bridge          │
                                         │  Deployed on Railway (cloud)    │
                                         └────────────────┬────────────────┘
                                                          │ WebSocket (WSS)
                                                          ▼
                                         ┌─────────────────────────────────┐
                                         │       Frontend Web UI           │
                                         │   index.html (opened locally)   │
                                         │  connects to Railway backend    │
                                         └─────────────────────────────────┘
```

---

## How It Works

The ESP32 reads soil moisture every 3 seconds and publishes readings to HiveMQ Cloud over MQTT. The Node.js backend — deployed on Railway — subscribes to those MQTT topics and forwards all data to the browser over a secure WebSocket connection (`wss://`). Commands from the dashboard (pump on/off, schedules, threshold changes) travel the reverse path: browser → WebSocket → backend → MQTT → ESP32.

The frontend is a plain `index.html` file opened directly in the browser — no frontend server needed. It connects to the cloud-hosted Railway backend automatically.

---

## Components

### ESP32 Firmware

- Reads soil moisture sensor (averaged over 10 samples for noise reduction)
- Controls relay/pump based on manual commands, auto-threshold, or schedule
- Executes weekly irrigation schedules with NTP time sync (UTC+2 — Egypt)
- Publishes full status JSON to MQTT every 3 seconds
- Subscribes to pump command topic for remote control
- Supports dynamic threshold updates from the dashboard

**Libraries required (install via Arduino Library Manager):**
- PubSubClient
- ArduinoJson
- WiFiClientSecure (included with ESP32 core)

---

### Backend Server (Node.js)

Acts as a bridge between HiveMQ Cloud (MQTT) and the browser (WebSocket). Deployed on **Railway**.

- Connects to HiveMQ over MQTTS (port 8883)
- Subscribes to moisture and status topics
- Caches last known status — new dashboard connections immediately receive current state instead of showing blank values
- Forwards pump commands, schedule commands, and threshold changes from the browser to ESP32 via MQTT
- Credentials loaded from environment variables set in Railway dashboard

**Technologies:** Node.js, Express, MQTT.js, ws, cors

---

### Frontend Dashboard

A single `index.html` file opened directly in your browser. Connects to the Railway backend via WebSocket — no local server required.

**Features:**
- Live soil moisture percentage with color-coded bar (green / amber / red)
- Pump ON / OFF / AUTO mode buttons
- Auto-mode moisture threshold slider
- Weekly schedule: pick day (Sun–Sat or Every Day), This Week / Next Week toggle, start time, duration, repeat mode (Once / Weekly)
- Countdown timer — shows `Xd HH:MM:SS` format when more than 24 hours remain
- Moisture history line chart (last 12 readings, color shifts with moisture level)
- System info panel: last sensor reading time, today's watering count, next scheduled run
- ESP32 last-seen tracking with stale connection indicator

**Technologies:** HTML5, CSS3, Vanilla JavaScript, Chart.js

---

## Hardware Requirements

| Component | Description |
|---|---|
| ESP32 Development Board | Main microcontroller |
| Capacitive Soil Moisture Sensor | Analog moisture readings |
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

The firmware maps raw ADC values to 0–100% moisture. Defaults:

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

**To deploy on Railway:**
- Push the backend folder to GitHub
- Create a new Railway project and link the repo
- Add the environment variables above in the Railway dashboard under Variables
- Railway auto-deploys on every push

---

### 3. ESP32 Setup

Open `esp32-code/plantguard.ino` and fill in your credentials at the top of the file:

```cpp
// ── WiFi ──
const char* ssid     = "YOUR_WIFI_NAME";      // put your WiFi name here
const char* password = "YOUR_WIFI_PASSWORD";  // put your WiFi password here

// ── HiveMQ Cloud ──
const char* mqtt_server = "YOUR_HIVEMQ_CLUSTER_URL"; // put your HiveMQ URL here
const int   mqtt_port   = 8883;
const char* mqtt_user   = "YOUR_MQTT_USERNAME";       // put your MQTT username here
const char* mqtt_pass   = "YOUR_MQTT_PASSWORD";       // put your MQTT password here
```

Upload `plantguard.ino` to your ESP32 via Arduino IDE.

> ⚠️ Never commit real credentials to GitHub. Add the `.ino` file to `.gitignore` or use a separate `secrets.h` if sharing publicly.

---

### 4. Frontend Setup

No server needed. Open `frontend/index.html` directly in your browser.

The dashboard connects automatically to the Railway backend via WebSocket. Make sure the backend is deployed and running before opening the dashboard.

---

## MQTT Topics

### Published by ESP32

| Topic | Content |
|---|---|
| `plantguard_x7k92mf/status` | Full JSON: moisture, pump state, mode, schedule, threshold |
| `plantguard_x7k92mf/moisture` | Raw moisture percentage (plain text) |

### Subscribed by ESP32

| Topic | Accepted values |
|---|---|
| `plantguard_x7k92mf/pump` | `pump_on`, `pump_off`, `auto_mode`, or JSON commands below |

### JSON Command Format

```json
// Set schedule
{ "cmd": "set_schedule", "day": 6, "hour": 7, "minute": 30, "duration": 300, "repeat": "weekly" }

// Clear schedule
{ "cmd": "clear_schedule" }

// Update auto threshold
{ "cmd": "set_threshold", "value": 40 }
```

`day`: -1 = every day, 0 = Sunday … 6 = Saturday  
`duration`: in seconds

---

## Scheduling

- Pick a **specific day** (Sun–Sat) or **Every Day**
- When a specific day is selected, choose **This Week** or **Next Week** to avoid ambiguity
- If you pick "This Week" and the time has already passed today, the dashboard shows an error and highlights "Next Week" automatically
- Set **start time**, **duration** in minutes, and **repeat mode** (Once or Weekly)
- The ESP32 echoes schedule state back in its status JSON so the countdown survives a page refresh

---

## Project Structure

```text
smart-agriculture-system/
│
├── esp32-code/
│   └── plantguard.ino       # Main firmware (fill in credentials at top)
│
├── backend/
│   ├── server.js            # MQTT ↔ WebSocket bridge
│   ├── package.json
│   ├── .env                 # Credentials — never commit this
│   └── .gitignore
│
├── frontend/
│   └── index.html           # Full dashboard — open directly in browser
│
└── README.md
```

---

## Security Notes

- Never commit `.env` or real credentials to version control — add `.env` to `.gitignore`
- The ESP32 uses TLS (`WiFiClientSecure`) but skips certificate verification (`setInsecure()`) — acceptable for private/home use
- The backend reads credentials from environment variables set in Railway, not hardcoded

---

## Future Improvements

- Multi-plant / multi-zone support
- Database logging for historical moisture data
- Mobile app
- Push notifications when soil is critically dry
- OTA firmware updates
- Weather API integration to skip watering after rain
- AI-based irrigation optimization

---

## License

MIT License

---

Developed as the PlantGuard Smart Agriculture IoT Project 🌱
