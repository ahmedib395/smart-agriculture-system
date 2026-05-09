🌱 Smart Irrigation System — ESP32 MQTT
A smart irrigation system using an ESP32 microcontroller, MQTT protocol over HiveMQ Cloud, and a web dashboard for remote control.
---
📡 Architecture
```
ESP32 (Sensor + Relay)
        ↕ MQTT over TLS (HiveMQ Cloud)
Node.js Backend (Railway)
        ↕ WebSocket
Web Dashboard (Netlify)
```
---
⚙️ Hardware
Component	Pin
Soil Moisture Sensor	GPIO 35
Relay (Water Pump)	GPIO 32
---
📦 Libraries Required
Install via Arduino Library Manager:
Library	Author	How
WiFi.h	—	Built-in (ESP32)
WiFiClientSecure.h	—	Built-in (ESP32)
PubSubClient	Nick O'Leary	Library Manager
ArduinoJson	Benoit Blanchon	Library Manager
time.h	—	Built-in (ESP32)
---
🔧 Configuration
Open `esp32_irrigation.ino` and update these lines:
```cpp
const char* ssid        = "YOUR_WIFI_NAME";
const char* password    = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR.hivemq.cloud";
const char* mqtt_user   = "YOUR_USERNAME";
const char* mqtt_pass   = "YOUR_PASSWORD";
```
---
📤 MQTT Topics
Topic	Direction	Description
`plantguard_x7k92mf/moisture`	ESP32 → Cloud	Raw moisture %
`plantguard_x7k92mf/status`	ESP32 → Cloud	Full JSON status
`plantguard_x7k92mf/pump`	Cloud → ESP32	Commands
---
🎮 Commands (sent to pump topic)
Command	Type	Description
`pump_on`	plain text	Turn pump ON (manual)
`pump_off`	plain text	Turn pump OFF (manual)
`auto_mode`	plain text	Enable auto mode (triggers below 40% moisture)
`{"cmd":"set_schedule","hour":7,"minute":30,"duration":60}`	JSON	Schedule watering at 07:30 for 60 seconds
`{"cmd":"clear_schedule"}`	JSON	Cancel scheduled watering
---
📊 Status JSON (published every 3 seconds)
```json
{
  "moisture": 45,
  "pump": "OFF",
  "mode": "AUTO",
  "schedule": "07:30 (60s)"
}
```
---
🌍 Timezone
Currently set to UTC+2 (Egypt). To change, update this line in `setup()`:
```cpp
configTime(2 * 3600, 0, "pool.ntp.org");
//         ^ change this (seconds offset from UTC)
```
---
🔗 Links
HiveMQ Cloud — Free MQTT broker
PubSubClient Docs
ArduinoJson Docs
