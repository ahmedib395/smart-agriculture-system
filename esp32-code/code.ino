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

// ── Hardware Pins ──
const int sensorPin = 35;
const int relayPin  = 32;

// ── Sensor Calibration ──
const int DRY_VALUE      = 3500;
const int WET_VALUE      = 1500;
int AUTO_THRESHOLD = 40;
// ── Relay Logic (adjust if your relay is active-LOW) ──
const int PUMP_ON  = HIGH;
const int PUMP_OFF = LOW;

// ── Timezone (Egypt = UTC+2) ──
const long  TZ_OFFSET_SEC = 2 * 3600;

// ================================================================
//  STATE
// ================================================================
bool pumpState     = false;
bool autoModeState = false;

// ── Schedule ──
// scheduleDay: -1 = every day, 0–6 = Sunday–Saturday (tm_wday)
int  scheduleDay      = -1;
int  scheduleHour     = -1;   // -1 = no schedule set
int  scheduleMinute   = -1;
int  scheduleDuration = 60;   // seconds
bool scheduleRepeat   = true; // true = weekly/daily, false = once

bool scheduleRunning  = false;
unsigned long pumpStartTime = 0;

// ── MQTT / WiFi ──
WiFiClientSecure espClient;
PubSubClient     client(espClient);

// ================================================================
//  PUMP CONTROL
// ================================================================
void pumpOn() {
  digitalWrite(relayPin, PUMP_ON);
  pumpState = true;
}

void pumpOff() {
  digitalWrite(relayPin, PUMP_OFF);
  pumpState = false;
}

// ================================================================
//  SOIL MOISTURE READING  (averaged over 10 samples)
// ================================================================
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

// ================================================================
//  PUBLISH STATUS JSON
//  Includes schedule fields so the dashboard can reconstruct
//  the countdown after a page refresh.
// ================================================================
void publishStatus(int percent) {
  StaticJsonDocument<300> doc;
  doc["moisture"]        = percent;
  doc["pump"]            = pumpState     ? "ON"   : "OFF";
  doc["mode"]            = autoModeState ? "AUTO" : "MANUAL";
  doc["scheduleDay"]     = scheduleDay;
  doc["scheduleHour"]    = scheduleHour;
  doc["scheduleMinute"]  = scheduleMinute;
  doc["scheduleDuration"]= scheduleDuration;
  doc["scheduleRepeat"]  = scheduleRepeat;

  // Human-readable schedule string for the status panel
  if (scheduleHour != -1) {
    char timeBuf[8];
    sprintf(timeBuf, "%02d:%02d", scheduleHour, scheduleMinute);
    String schedStr = "";
    if (scheduleDay == -1) {
      schedStr = String("Every Day ") + timeBuf;
    } else {
      const char* dayNames[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
      schedStr = String(dayNames[scheduleDay]) + " " + timeBuf;
    }
    schedStr += " (" + String(scheduleDuration) + "s)";
    doc["schedule"] = schedStr;
  } else {
    doc["schedule"] = "none";
  }

  char payload[300];
  serializeJson(doc, payload);

  client.publish(TOPIC_STATUS, payload);
  Serial.print("Status sent: ");
  Serial.println(payload);
}

// ================================================================
//  TIME SYNC
// ================================================================
void syncTime() {
  configTime(TZ_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Syncing time");
  struct tm t;
  int attempts = 0;
  while (!getLocalTime(&t) && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  if (attempts < 20) {
    Serial.printf("\nTime synced: %02d:%02d:%02d  weekday=%d\n",
      t.tm_hour, t.tm_min, t.tm_sec, t.tm_wday);
  } else {
    Serial.println("\nTime sync failed — schedule won't work until sync succeeds");
  }
}

// ================================================================
//  SCHEDULE CHECK  (called every loop iteration)
// ================================================================
void checkSchedule() {
  if (scheduleHour == -1) return;  // No schedule set

  struct tm t;
  if (!getLocalTime(&t)) return;

  // ── Should watering START? ──
  bool dayMatch = (scheduleDay == -1) || (t.tm_wday == scheduleDay);

  if (!scheduleRunning && dayMatch &&
      t.tm_hour == scheduleHour &&
      t.tm_min  == scheduleMinute) {
    scheduleRunning = true;
    pumpStartTime   = millis();
    autoModeState   = false;   // disable auto while scheduled run is active
    pumpOn();
    Serial.printf("Scheduled watering started (day=%d %02d:%02d dur=%ds)\n",
      scheduleDay, scheduleHour, scheduleMinute, scheduleDuration);
    publishStatus(readMoisturePercent());
  }

  // ── Should watering STOP? ──
  if (scheduleRunning &&
      millis() - pumpStartTime >= (unsigned long)(scheduleDuration * 1000UL)) {
    scheduleRunning = false;
    pumpOff();
    Serial.println("Scheduled watering finished");

    // If one-time schedule, clear it
    if (!scheduleRepeat) {
      scheduleHour   = -1;
      scheduleMinute = -1;
      scheduleDay    = -1;
      Serial.println("One-time schedule cleared");
    }

    publishStatus(readMoisturePercent());
  }
}

// ================================================================
//  MQTT COMMAND HANDLER
// ================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Command received: ");
  Serial.println(message);

  // ── Plain text commands ──
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
  // ── JSON commands ──
  else if (message.startsWith("{")) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, message);
    if (err) {
      Serial.print("JSON parse error: ");
      Serial.println(err.c_str());
      return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "set_schedule") == 0) {
      scheduleDay      = doc["day"]      | -1;
      scheduleHour     = doc["hour"]     | -1;
      scheduleMinute   = doc["minute"]   | 0;
      scheduleDuration = doc["duration"] | 60;

      // "weekly" or "daily" both mean repeat; "once" means clear after run
      const char* repeat = doc["repeat"] | "weekly";
      scheduleRepeat = (strcmp(repeat, "once") != 0);

      Serial.printf("Schedule set → day=%d %02d:%02d duration=%ds repeat=%s\n",
        scheduleDay, scheduleHour, scheduleMinute,
        scheduleDuration, scheduleRepeat ? "yes" : "no");
    }
   else if (strcmp(cmd, "clear_schedule") == 0) {
      scheduleHour    = -1;
      scheduleMinute  = -1;
      scheduleDay     = -1;
      scheduleRunning = false;
      Serial.println("Schedule cleared");
    }
    else if (strcmp(cmd, "set_threshold") == 0) {
      int val = doc["value"] | 40;
      AUTO_THRESHOLD = constrain(val, 10, 80);
      Serial.printf("Auto threshold set to %d%%\n", AUTO_THRESHOLD);
    }
  }  
  publishStatus(readMoisturePercent());
}

// ================================================================
//  WIFI + MQTT SETUP
// ================================================================
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected — IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi FAILED — restarting");
    ESP.restart();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "ESP32_PlantGuard_";
    clientId += String(random(1000, 9999));
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Connected");
      client.subscribe(TOPIC_PUMP);
      Serial.print("Subscribed to: ");
      Serial.println(TOPIC_PUMP);
    } else {
      Serial.print("Failed rc=");
      Serial.print(client.state());
      Serial.println(" — retry in 5s");
      delay(5000);
    }
  }
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pumpOff();

  setup_wifi();
  syncTime();

  espClient.setInsecure();   // TLS without cert verification (OK for private use)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int percent = readMoisturePercent();

  // Auto mode: turn pump on/off based on threshold
  if (autoModeState) {
    if (percent < AUTO_THRESHOLD) {
      pumpOn();
    } else {
      pumpOff();
    }
  }

  // Check if a scheduled watering should start or stop
  checkSchedule();

  // Publish status every 3 seconds
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 3000) {
    lastMsg = millis();
    publishStatus(percent);
    Serial.printf("Moisture: %d%%  Pump: %s  Mode: %s\n",
      percent,
      pumpState     ? "ON"   : "OFF",
      autoModeState ? "AUTO" : "MANUAL");
  }
}
