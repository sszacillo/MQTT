#include <Arduino.h>
#include <WiFi.h>
#include "WiFiManager.h"
#include "MQTTClient.h"

#define SSID "jack-spot"
#define PASSWORD "Jacek123"
#define MQTT_SERVER "192.168.45.188"
#define MQTT_PORT 1883

MQTTClient mqttClient(MQTT_SERVER, MQTT_PORT);

void messageReceived(const std::string& topic, const std::string& payload) {
    Serial.print("Message received on topic: ");
    Serial.print(topic.c_str());
    Serial.print(". Payload: ");
    Serial.println(payload.c_str());
}

void setup() {
    Serial.begin(115200);

    WiFiManager wifiManager(SSID, PASSWORD);
    wifiManager.connect();

    if (WiFi.status() == WL_CONNECTED) {
        if (mqttClient.connect("ESP32Client")) {
            Serial.println("Connected to MQTT server");
            mqttClient.subscribe("test/topic", 1); // Subscribe with QoS 1
            mqttClient.setMessageCallback(messageReceived);
        } else {
            Serial.println("Failed to connect to MQTT server");
        }
    } else {
        Serial.println("Failed to connect to WiFi");
    }

    Serial.println("Start entering your messages:");
}

void loop() {
    mqttClient.loop();

    if (Serial.available() > 0) {
        String message = Serial.readStringUntil('\n');
        message.trim(); // Remove any trailing newline characters

        if (mqttClient.publish("test/topic", message.c_str(), 1, true)) { // Publish with QoS 1 and retained flag
            Serial.println("Message sent: " + message);
        } else {
            Serial.println("Failed to send message");
        }
    }
}
