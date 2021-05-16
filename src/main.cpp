#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID  = "CHANGEME";
const char* WIFI_PASSWORD = "CHANGEME";
const char* MQTT_SERVER = "CHANGEME";
const uint MQTT_PORT = 1883;
const char* MQTT_CLIENTID = "TheFinger3000";
const char* MQTT_TOPIC = "finger3000";
const char* MQTT_USER = "";
const char* MQTT_PSSWORD = "";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#define SERVO_PLATFORM_PIN D1
#define SERVO_FINGER_PIN D2
Servo servoFinger;
uint8 servoFingerCurPos;
Servo servoPlatform;
uint8 servoPlatformCurPos;

bool move(uint8 degreeFinger, uint8 degreePlatform) {
  if(degreeFinger < 0 || degreeFinger > 180 || degreePlatform < 0 || degreePlatform > 180) {
    Serial.println("Error: Servo movement out of range");
    return false;
  }

  uint8 deltaFinger = 0;
  if(degreeFinger < servoFingerCurPos) {
    deltaFinger = servoFingerCurPos - degreeFinger;
  }
  else {
    deltaFinger = degreeFinger - servoFingerCurPos;
  }

  uint8 deltaPlatform = 0;
  if(degreePlatform < servoPlatformCurPos) {
    deltaPlatform = servoPlatformCurPos - degreePlatform;
  }
  else {
    deltaPlatform = degreePlatform - servoPlatformCurPos;
  }

  servoFingerCurPos = degreeFinger;
  servoPlatformCurPos = degreePlatform;

  servoFinger.write(degreeFinger);
  servoPlatform.write(degreePlatform);
 
  if(deltaFinger > deltaPlatform) {
    delay(deltaFinger*2);
  }
  else {
    delay(deltaPlatform*2);
  }
  return true;
}

void mqttOnMessage(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s]\nPayload: ", topic);
  for (uint i = 0; i < length; i++) {
    Serial.printf("%c", payload[i]);
  }
  Serial.println();
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  uint8 finger = doc["finger"];
  uint8 platform = doc["platform"];
  if(move(finger, platform)) {
    Serial.printf("Moving Finger to %i° and Platform to %i°\n", finger, platform);
  }
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PSSWORD)) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.printf("failed, rc=%i\n", mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting");

  servoFinger.attach(SERVO_FINGER_PIN);
  servoFingerCurPos = 90;
  servoFinger.write(servoFingerCurPos);

  servoPlatform.attach(SERVO_PLATFORM_PIN);
  servoPlatformCurPos = 90;
  servoPlatform.write(servoPlatformCurPos);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to SSID %s\n", WIFI_SSID);

  int wifiConnTimer = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.printf("%i ", wifiConnTimer);
    wifiConnTimer++;
  }
  Serial.println();

  Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttOnMessage);
}

void loop() {
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();
}