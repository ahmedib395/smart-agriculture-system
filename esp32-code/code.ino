#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ================= CONFIGURATION =================
// Replace these values before uploading to ESP32

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "YOUR_HIVEMQ_CLUSTER_URL";
const int mqtt_port = 8883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_pass = "YOUR_MQTT_PASSWORD";

// ================= MQTT TOPICS =================
const char* TOPIC_MOISTURE = "plantguard_x7k92mf/moisture";
const char* TOPIC_PUMP = "plantguard_x7k92mf/pump";
const char* TOPIC_STATUS = "plantguard_x7k92mf/status";

// ================= HARDWARE PINS =================
const int sensorPin = 35;
const int relayPin = 5;

// ================= SETTINGS =================
const int DRY_VALUE = 3500;
const int WET_VALUE = 1500;
const int AUTO_THRESHOLD = 40;

// Relay is active LOW
const int PUMP_ON = LOW;
const int PUMP_OFF = HIGH;

bool pumpState = false;
bool autoModeState = false;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void pumpOn() {
  digitalWrite(relayPin, PUMP_ON);
  pumpState = true;
}

void pumpOff() {
  digitalWrite(relayPin, PUMP_OFF);
  pumpState = false;
}

int readMoisturePercent() {
  int raw = analogRead(sensorPin);
  int percent = map(raw, DRY_VALUE, WET_VALUE, 0, 100);
  return constrain(percent, 0, 100);
}

void publishStatus(int percent) {
  String payload = "{";
  payload += "\"moisture\":" + String(percent) + ",";
  payload += "\"pump\":\"" + String(pumpState ? "ON" : "OFF") + "\",";
  payload += "\"mode\":\"" + String(autoModeState ? "AUTO" : "MANUAL") + "\"";
  payload += "}";

  client.publish(TOPIC_STATUS, payload.c_str());

  Serial.print("Status sent: ");
  Serial.println(payload);
}

void setup_wifi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Command received: ");
  Serial.println(message);

  if (message == "pump_on") {
    autoModeState = false;
    pumpOn();
  }
  else if (message == "pump_off") {
    autoModeState = false;
    pumpOff();
  }
  else if (message == "auto_mode") {
    autoModeState = true;
  }

  int percent = readMoisturePercent();
  publishStatus(percent);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    String clientId = "ESP32_Plant_Client_";
    clientId += String(random(1000, 9999));

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Connected");
      client.subscribe(TOPIC_PUMP);

      Serial.print("Subscribed to: ");
      Serial.println(TOPIC_PUMP);
    } else {
      Serial.print("Failed rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pumpOff();

  setup_wifi();

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int percent = readMoisturePercent();

  if (autoModeState) {
    if (percent < AUTO_THRESHOLD) {
      pumpOn();
    } else {
      pumpOff();
    }
  }

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 3000) {
    lastMsg = millis();

    client.publish(TOPIC_MOISTURE, String(percent).c_str());
    publishStatus(percent);

    Serial.print("Moisture sent to website: ");
    Serial.print(percent);
    Serial.println("%");
  }
}
