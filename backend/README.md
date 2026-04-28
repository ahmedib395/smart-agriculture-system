# Backend - Smart Irrigation System

This folder contains the Node.js backend server.

## Role
The backend acts as a bridge between the web dashboard and the ESP32.

## Technologies
- Node.js
- Express.js
- WebSocket
- MQTT
- HiveMQ Cloud

## Communication Flow
ESP32 publishes sensor data to HiveMQ Cloud using MQTT.  
The backend subscribes to the MQTT topics and sends data to the frontend using WebSocket.

The frontend sends pump commands to the backend, and the backend publishes them to MQTT for the ESP32.

## MQTT Topics
- Sensor data: plantguard_x7k92mf/moisture
- Pump command: plantguard_x7k92mf/pump
- Status: plantguard_x7k92mf/status

## Hosting
The backend is deployed on Railway.

Backend URL:
https://smartagriculturebackend-production-63f7.up.railway.app
