#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "We";
const char* password = "Mazen2005";

const char* mqtt_server = "491651ab41c341269c0b6942b3a72aaf.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "ahmedhassan";
const char* mqtt_pass = "Sas51015";

const char* TOPIC_MOISTURE = "plantguard_x7k92mf/moisture";
const char* TOPIC_PUMP = "plantguard_x7k92mf/pump";
const char* TOPIC_STATUS = "plantguard_x7k92mf/status";

const int sensorPin = 35;
const int relayPin = 5;

const int DRY_VALUE = 3500;
const int WET_VALUE = 1500;
const int AUTO_THRESHOLD = 40;

const int PUMP_ON = LOW;
const int PUMP_OFF = HIGH;

bool pumpState = false;
bool autoModeState = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
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

  Serial.println(payload);
}

void setup_wifi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  lcd.clear();
  lcd.print("WiFi Connected");
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

    if (client.connect("ESP32_Plant_Client", mqtt_user, mqtt_pass)) {
      Serial.println("Connected");
      client.subscribe(TOPIC_PUMP);
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

  lcd.init();
  lcd.backlight();
  lcd.print("Connecting...");

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

    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");
    lcd.print(percent);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    lcd.print("Pump:");
    lcd.print(pumpState ? "ON " : "OFF");
    lcd.print(autoModeState ? " A" : " M");
  }
} 

