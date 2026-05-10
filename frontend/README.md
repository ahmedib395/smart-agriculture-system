# PlantGuard Frontend 🌿

A modern real-time web dashboard for the PlantGuard Smart Irrigation System.

This frontend provides live monitoring and remote control for the ESP32-based irrigation system using WebSockets and MQTT communication through the backend server.

---

## Features

### Real-Time Soil Monitoring
- Live soil moisture percentage display
- Dynamic moisture progress bar
- Moisture condition indicators:
  - 🟢 Good moisture
  - 🟡 Moderate moisture
  - 🔴 Dry soil warning

### Pump Control System
- Turn water pump ON/OFF remotely
- Automatic irrigation mode
- Real-time pump status updates
- Pump runtime progress indicator

### Smart Scheduling
- Schedule watering by:
  - Specific weekday
  - Daily mode
  - One-time watering
  - Weekly repeating schedules
- Custom watering duration
- "This Week" / "Next Week" scheduling logic
- Live countdown timer until next watering cycle

### Moisture History Analytics
- Real-time moisture chart using Chart.js
- Historical moisture trend visualization
- Auto-colored chart based on soil condition

### System Information Panel
- ESP32 connection monitoring
- Last seen tracker
- Watering count statistics
- Next scheduled irrigation display

### Responsive UI
- Mobile-friendly responsive layout
- Futuristic glassmorphism design
- Animated background effects
- Real-time connection status indicator

---

## Technologies Used

- HTML5
- CSS3
- Vanilla JavaScript
- WebSocket API
- Chart.js

---

## Frontend Architecture

The frontend communicates with the backend server using secure WebSockets:

```text
ESP32 → MQTT Broker → Node.js Backend → WebSocket → Frontend Dashboard
