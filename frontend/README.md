
# Frontend - Smart Irrigation System

This folder contains the web dashboard for the Smart Irrigation System.

## Features
- Displays live soil moisture percentage
- Shows pump status
- Manual pump ON/OFF control
- Auto mode button
- Connects to the backend server using WebSocket

## Hosting
The frontend is deployed on Netlify.

Live website 
 http://smart-agriculture-iot.neltify.app/

## Communication
The frontend does not connect directly to ESP32.  
It sends commands to the backend server using WebSocket.

Frontend → Backend → MQTT → ESP32
