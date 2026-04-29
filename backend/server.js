require("dotenv").config();
const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const mqtt = require("mqtt");
const cors = require("cors");

const app = express();
app.use(cors());

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = process.env.PORT || 3000;


const mqttClient = mqtt.connect(process.env.MQTT_HOST, {
  username: process.env.MQTT_USER,
  password: process.env.MQTT_PASS,
  rejectUnauthorized: false
});
const TOPIC_MOISTURE = "plantguard_x7k92mf/moisture";
const TOPIC_PUMP = "plantguard_x7k92mf/pump";
const TOPIC_STATUS = "plantguard_x7k92mf/status";

app.get("/", (req, res) => {
  res.send("Smart Agriculture Backend Server is running");
});

mqttClient.on("connect", () => {
  console.log("Connected to HiveMQ Cloud");
  mqttClient.subscribe(TOPIC_MOISTURE);
  mqttClient.subscribe(TOPIC_STATUS);
});

mqttClient.on("message", (topic, message) => {
  const text = message.toString();

  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      if (topic === TOPIC_MOISTURE) {
        client.send(JSON.stringify({
          type: "moisture",
          value: text
        }));
      }

      if (topic === TOPIC_STATUS) {
        client.send(JSON.stringify({
          type: "status",
          value: JSON.parse(text)
        }));
      }
    }
  });
});

wss.on("connection", (ws) => {
  console.log("Web Dashboard connected");

  ws.on("message", (msg) => {
    const data = JSON.parse(msg.toString());

    if (data.type === "pump_command") {
      mqttClient.publish(TOPIC_PUMP, data.value);
      console.log("Command sent to ESP32:", data.value);
    }
  });
});

server.listen(PORT, () => {
  console.log(`Server bridge running on port ${PORT}`);
});
