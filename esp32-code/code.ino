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

// ----------------------------------------------------------------
//  MQTT Topics
// ----------------------------------------------------------------
const char* TOPIC_MOISTURE = "plantguard_x7k92mf/moisture";
const char* TOPIC_PUMP     = "plantguard_x7k92mf/pump";
const char* TOPIC_STATUS   = "plantguard_x7k92mf/status";

// ----------------------------------------------------------------
//  Hardware Pins
// ----------------------------------------------------------------
const int sensorPin = 35;
const int relayPin  = 32;

// ----------------------------------------------------------------
//  Sensor Calibration
// ----------------------------------------------------------------
const int DRY_VALUE      = 3500;
const int WET_VALUE      = 1500;
const int AUTO_THRESHOLD = 40;

// ----------------------------------------------------------------
//  Relay Logic
// ----------------------------------------------------------------
const int PUMP_ON  = HIGH;
const int PUMP_OFF = LOW;

// ----------------------------------------------------------------
//  State
// ----------------------------------------------------------------
bool pumpState     = false;
bool autoModeState = false;

// ----------------------------------------------------------------
//  Schedule
// ----------------------------------------------------------------
int  scheduleHour     = -1;   // -1 = no schedule set
int  scheduleMinute   = -1;
int  scheduleDuration = 60;   // seconds
bool scheduleRunning  = false;
unsigned long pumpStartTime = 0;

// ----------------------------------------------------------------
//  MQTT / WiFi clients
// ----------------------------------------------------------------
WiFiClientSecure espClient;
PubSubClient     client(espClient);

// ----------------------------------------------------------------
//  Pump control
// ----------------------------------------------------------------
void pumpOn() {
  digitalWrite(relayPin, PUMP_ON);
  pumpState = true;
}

void pumpOff() {
  digitalWrite(relayPin, PUMP_OFF);
  pumpState = false;
}

// ----------------------------------------------------------------
//  Soil moisture reading (averaged over 10 samples)
// ----------------------------------------------------------------
int readMoisturePercent() {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(sensorPin);
    delay(10);
  }
  int raw     = total / 10;
  int percent = map(raw, DRY_VALUE, WET_VALUE, 0, 100);
  percent     = constrain(percent, 0, 100);
  return percent;
}

// ----------------------------------------------------------------
//  Publish full status JSON to website
// ----------------------------------------------------------------
void publishStatus(int percent) {
  String payload = "{";
  payload += "\"moisture\":"  + String(percent) + ",";
  payload += "\"pump\":\""    + String(pumpState     ? "ON"   : "OFF")    + "\",";
  payload += "\"mode\":\""    + String(autoModeState ? "AUTO" : "MANUAL") + "\",";
  payload += "\"schedule\":\"";
  if (scheduleHour != -1) {
    char buf[10];
    sprintf(buf, "%02d:%02d", scheduleHour, scheduleMinute);
    payload += String(buf) + " (" + String(scheduleDuration) + "s)";
  } else {
    payload += "none";
  }
  payload += "\"}";

  client.publish(TOPIC_STATUS, payload.c_str());
  Serial.print("Status sent: ");
  Serial.println(payload);
}

// ----------------------------------------------------------------
//  Sync time from internet (Egypt = UTC+2)
// ----------------------------------------------------------------
void syncTime() {
  configTime(2 * 3600, 0, "pool.ntp.org");
  Serial.print("Syncing time");
  struct tm t;
  while (!getLocalTime(&t)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" done");
  Serial.printf("Current time: %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
}

// ----------------------------------------------------------------
//  Check if scheduled watering should start or stop
// ----------------------------------------------------------------
void checkSchedule() {
  if (scheduleHour == -1) return;

  struct tm t;
  if (!getLocalTime(&t)) return;

  // Start pump at scheduled time
  if (!scheduleRunning &&
      t.tm_hour == scheduleHour &&
      t.tm_min  == scheduleMinute) {
    scheduleRunning = true;
    pumpStartTime   = millis();
    autoModeState   = false;
    pumpOn();
    Serial.println("Scheduled watering started");
    publishStatus(readMoisturePercent());
  }

  // Stop pump after duration
  if (scheduleRunning &&
      millis() - pumpStartTime >= (unsigned long)(scheduleDuration * 1000)) {
    scheduleRunning = false;
    pumpOff();
    Serial.println("Scheduled watering finished");
    publishStatus(readMoisturePercent());
  }
}

// ----------------------------------------------------------------
//  MQTT command handler
//  Plain text: "pump_on" / "pump_off" / "auto_mode"
//  JSON:       {"cmd":"set_schedule","hour":7,"minute":30,"duration":60}
//              {"cmd":"clear_schedule"}
// ----------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Command received: ");
  Serial.println(message);

  if (message == "pump_on") {
    autoModeState   = false;
    scheduleRunning = false;
    pumpOn();
  }
  else if (message == "pump_off") {
    autoModeState   = false;
    scheduleRunning = false;
    pumpOff();
  }
  else if (message == "auto_mode") {
    autoModeState   = true;
    scheduleRunning = false;
  }
  else if (message.startsWith("{")) {
    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, message);
    if (!err) {
      const char* cmd = doc["cmd"];
      if (cmd && strcmp(cmd, "set_schedule") == 0) {
        scheduleHour     = doc["hour"];
        scheduleMinute   = doc["minute"];
        scheduleDuration = doc["duration"];
        Serial.printf("Schedule set: %02d:%02d for %d seconds\n",
          scheduleHour, scheduleMinute, scheduleDuration);
      }
      else if (cmd && strcmp(cmd, "clear_schedule") == 0) {
        scheduleHour    = -1;
        scheduleMinute  = -1;
        scheduleRunning = false;
        Serial.println("Schedule cleared");
      }
    }
  }

  publishStatus(readMoisturePercent());
}

// ----------------------------------------------------------------
//  WiFi connect
// ----------------------------------------------------------------
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

// ----------------------------------------------------------------
//  MQTT reconnect
// ----------------------------------------------------------------
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

// ----------------------------------------------------------------
//  Setup
// ----------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pumpOff();

  setup_wifi();
  syncTime();

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ----------------------------------------------------------------
//  Loop
// ----------------------------------------------------------------
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

  checkSchedule();

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
