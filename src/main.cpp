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

MQTTClient mqttClient(mqtt_server, mqtt_port);

void messageReceived(const std::string& topic, const std::string& payload) {
    Serial.print("Message received on topic: ");
    Serial.print(topic.c_str());
    Serial.print(". Payload: ");
    Serial.println(payload.c_str());
}

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);

    WiFiManager wifiManager(ssid, password);
    wifiManager.connect();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        if (mqttClient.connect("ESP32Client")) {
            Serial.println("Connected to MQTT server");
            mqttClient.subscribe("test/topic", 1); // Subskrybuj z QoS 1
            mqttClient.setMessageCallback(messageReceived);
        } else {
            Serial.println("Failed to connect to MQTT server");
        }
    } else {
        Serial.println("Failed to connect to WiFi");
    }
}

void loop() {
    mqttClient.loop();

    if (digitalRead(buttonPin) == HIGH) {
        if (mqttClient.publish("test/topic", "Button pressed", 1, true)) { // Publikuj z QoS 1 i retained flag
            Serial.println("Button pressed, message sent");
        } else {
            Serial.println("Failed to send message");
        }
        delay(1000);  
    }
}
