#include "MQTTClient.h"
#include <Arduino.h>

MQTTClient::MQTTClient(WiFiClient& client, const char* server, uint16_t port)
    : wifiClient(client), server(server), port(port), callback(nullptr), packetIdentifier(0) {}

void MQTTClient::setCallback(void (*callback)(char*, byte*, unsigned int)) {
    this->callback = callback;
}

bool MQTTClient::connect(const char* clientID) {
    Serial.print("Connecting to MQTT server: ");
    Serial.print(server);
    Serial.print(":");
    Serial.println(port);

    
    if (!wifiClient.connect(server, port)) {
        Serial.println("Connection to MQTT server failed");
        return false;
    } else {
        Serial.println("Connected to MQTT server");
    }

    if (!sendConnectPacket(clientID)) {
        Serial.println("Failed to send CONNECT packet");
        return false;
    } else {
        Serial.println("CONNECT packet sent successfully");
    }

    return true;
}


void MQTTClient::loop() {
    if (wifiClient.available()) {
        handlePacket();
    }
}

bool MQTTClient::publish(const char* topic, const char* message) {
    return sendPublishPacket(topic, message);
}

bool MQTTClient::subscribe(const char* topic) {
    return sendSubscribePacket(topic);
}

bool MQTTClient::sendConnectPacket(const char* clientID) {
    Serial.println("Sending CONNECT packet");

    uint8_t protocolName[] = {0x00, 0x04, 'M', 'Q', 'T', 'T'};
    uint8_t protocolLevel = 0x04;
    uint8_t connectFlags = 0x02;  // Clean session
    uint16_t keepAlive = 60;
    uint16_t clientIDLength = strlen(clientID);

    uint8_t packet[10 + clientIDLength];
    uint8_t packetIndex = 0;

    packet[packetIndex++] = 0x10;  // CONNECT packet type
    packet[packetIndex++] = 0;     // Remaining length (calculated later)

    memcpy(&packet[packetIndex], protocolName, sizeof(protocolName));
    packetIndex += sizeof(protocolName);
    packet[packetIndex++] = protocolLevel;
    packet[packetIndex++] = connectFlags;
    packet[packetIndex++] = (keepAlive >> 8) & 0xFF;
    packet[packetIndex++] = keepAlive & 0xFF;
    packet[packetIndex++] = (clientIDLength >> 8) & 0xFF;
    packet[packetIndex++] = clientIDLength & 0xFF;
    memcpy(&packet[packetIndex], clientID, clientIDLength);
    packetIndex += clientIDLength;

    // Set the remaining length
    packet[1] = packetIndex - 2;

    // Print packet for debugging
    Serial.print("CONNECT packet: ");
    for (uint8_t i = 0; i < packetIndex; i++) {
        Serial.print(packet[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    wifiClient.write(packet, packetIndex);

    // Wait for CONNACK
    delay(1000);  // Wait for response
    if (wifiClient.available() > 0) {
        uint8_t response[4];
        wifiClient.read(response, 4);
        Serial.print("Received response: ");
        for (int i = 0; i < 4; i++) {
            Serial.print(response[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        return response[0] == 0x20 && response[1] == 0x02 && response[3] == 0x00;  // Check CONNACK packet
    } else {
        Serial.println("No response to CONNECT packet");
    }

    return false;
}


bool MQTTClient::sendPublishPacket(const char* topic, const char* message) {
    Serial.println("Sending PUBLISH packet");

    uint16_t topicLength = strlen(topic);
    uint16_t messageLength = strlen(message);

    uint8_t packet[4 + topicLength + messageLength];
    uint8_t packetIndex = 0;

    packet[packetIndex++] = 0x30;  // PUBLISH packet type
    packet[packetIndex++] = topicLength + messageLength + 2;  // Remaining length

    packet[packetIndex++] = (topicLength >> 8) & 0xFF;
    packet[packetIndex++] = topicLength & 0xFF;
    memcpy(&packet[packetIndex], topic, topicLength);
    packetIndex += topicLength;
    memcpy(&packet[packetIndex], message, messageLength);
    packetIndex += messageLength;

    wifiClient.write(packet, packetIndex);

    return true;
}

bool MQTTClient::sendSubscribePacket(const char* topic) {
    Serial.println("Sending SUBSCRIBE packet");

    uint16_t topicLength = strlen(topic);

    uint8_t packet[7 + topicLength];
    uint8_t packetIndex = 0;

    packet[packetIndex++] = 0x82;  // SUBSCRIBE packet type
    packet[packetIndex++] = 5 + topicLength;  // Remaining length

    packetIdentifier++;
    packet[packetIndex++] = (packetIdentifier >> 8) & 0xFF;  // MSB
    packet[packetIndex++] = packetIdentifier & 0xFF;         // LSB

    packet[packetIndex++] = (topicLength >> 8) & 0xFF;
    packet[packetIndex++] = topicLength & 0xFF;
    memcpy(&packet[packetIndex], topic, topicLength);
    packetIndex += topicLength;
    packet[packetIndex++] = 0x00;  // QoS

    wifiClient.write(packet, packetIndex);

    return true;
}

void MQTTClient::handlePacket() {
    uint8_t packetType = wifiClient.read();
    if (packetType == 0x30) {  // Publish packet
        uint8_t length = wifiClient.read();
        char topic[length + 1];
        wifiClient.readBytes(topic, length);
        topic[length] = '\0';
        uint8_t messageLength = wifiClient.read();
        char message[messageLength + 1];
        wifiClient.readBytes(message, messageLength);
        message[messageLength] = '\0';

        if (callback) {
            callback(topic, reinterpret_cast<byte*>(message), messageLength);
        }
    }
}



