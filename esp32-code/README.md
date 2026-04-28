# ESP32 Code - Smart Irrigation System

This folder contains the Arduino code uploaded to the ESP32.

## Function
The ESP32:
- Connects to WiFi
- Connects to HiveMQ Cloud using MQTT
- Reads soil moisture sensor values
- Publishes moisture data
- Receives pump commands
- Controls the relay and water pump
- Supports manual and auto mode

## Hardware Pins
- Soil moisture sensor AO → GPIO 35
- Relay IN → GPIO 5
- Relay VCC → 5V
- Relay GND → GND

## Libraries Required
Install from Arduino Library Manager:
- PubSubClient
- LiquidCrystal I2C

## Auto Mode
If moisture is below 40%, the pump turns ON.  
If moisture is 40% or higher, the pump turns OFF.
