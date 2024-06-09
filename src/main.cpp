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

bool connectToMQTT() {
    Serial.print("Connecting to MQTT server: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    if (mqttClient.connect("ESP32Client")) {
        Serial.println("Connected to MQTT server");
        mqttClient.subscribe("test/topic");
        return true;
    } else {
        Serial.println("Failed to connect to MQTT server");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);

    WiFiManager wifiManager(ssid, password);
    wifiManager.connect();

    Serial.print("WiFi status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Attempting to connect to MQTT server...");
        if (!connectToMQTT()) {
            Serial.println("Failed to connect to MQTT server");
        }
    } else {
        Serial.println("Failed to connect to WiFi");
    }
}

void loop() {
    mqttClient.loop();

    if (digitalRead(buttonPin) == HIGH) {
        if (mqttClient.publish("test/topic", "Button pressed")) {
            Serial.println("Button pressed, message sent");
        } else {
            Serial.println("Failed to send message");
        }
        delay(1000);  
    }
}
