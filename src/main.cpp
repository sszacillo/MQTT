#include <Arduino.h>
#include "WiFiManager.h"
#include "MQTTClient.h"
#include <WiFi.h>

// WiFi credentials
const char *ssid = "jack-spot";
const char *password = "Jacek123";

// MQTT Broker IP address
const char* mqtt_server = "192.168.45.188";
const int mqtt_port = 1883;

// Pin for the button
const int buttonPin = 4;

WiFiClient wifiClient;
MQTTClient mqttClient(wifiClient, mqtt_server, mqtt_port);

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);

  WiFiManager wifiManager(ssid, password);
  wifiManager.connect();

  mqttClient.setCallback([](char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  });

  if (mqttClient.connect("ESP32Client-jack")) {
    mqttClient.subscribe("test/topic");
  } else {
    Serial.println("Failed to connect to MQTT server");
  }
}

void loop() {
  mqttClient.loop();

  // Check if the button is pressed
  if (digitalRead(buttonPin) == HIGH) {
    if (mqttClient.publish("test/topic", "Button pressed")) {
      Serial.println("Button pressed, message sent");
    } else {
      Serial.println("Failed to send message");
    }
    delay(1000);  // Debounce delay
  }
}
