#include "MQTTClient.h"
#include <Arduino.h>

MQTTClient::MQTTClient(const char* server, uint16_t port)
    : server(server), port(port), packetIdentifier(0), keepAlive(60), lastPingTime(0) {}

bool MQTTClient::connect(const char* clientID) {
    Serial.print("Connecting to MQTT server: ");
    Serial.print(server.c_str());
    Serial.print(":");
    Serial.println(port);

    if (!wifiClient.connect(server.c_str(), port)) {
        Serial.println("Connection to MQTT server failed");
        return false;
    }

    if (!sendConnectPacket(clientID)) {
        Serial.println("Failed to send CONNECT packet");
        return false;
    }

    delay(1000);  // Wait for response
    if (wifiClient.available() > 0) {
        uint8_t response[4];
        if (wifiClient.read(response, 4) == 4) {
            Serial.print("Received response: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(response[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            if (response[0] == 0x20 && response[1] == 0x02 && response[3] == 0x00) {  // Check CONNACK packet
                lastPingTime = millis();
                return true;
            }
        }
    }

    Serial.println("No response to CONNECT packet");
    return false;
}

void MQTTClient::disconnect() {
    wifiClient.stop();
    Serial.println("Disconnected from MQTT server");
}

void MQTTClient::loop() {
    if (wifiClient.available() > 0) {
        handlePacket();
    }
    if (millis() - lastPingTime >= keepAlive * 1000) {
        if (!mqttPing()) {
            Serial.println("MQTT connection lost, reconnecting...");
            connect("ESP32Client");
        } else {
            lastPingTime = millis();
        }
    }
}

bool MQTTClient::publish(const char* topic, const char* message, QoS qos, bool retained) {
    return sendPublishPacket(topic, message, qos, retained);
}

bool MQTTClient::subscribe(const char* topic, QoS qos) {
    return sendSubscribePacket(topic, qos);
}

void MQTTClient::setMessageCallback(MessageCallback callback) {
    this->callback = callback;
}

void MQTTClient::setKeepAlive(uint16_t keepAlive) {
    this->keepAlive = keepAlive;
}

bool MQTTClient::sendConnectPacket(const char* clientID) {
    Serial.println("Sending CONNECT packet");

    uint8_t protocolName[] = {0x00, 0x04, 'M', 'Q', 'T', 'T'};
    uint8_t protocolLevel = 0x04;
    uint8_t connectFlags = 0x02;  // Clean session
    uint16_t clientIDLength = strlen(clientID);

    uint8_t packet[12 + clientIDLength];
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

    return true;
}

bool MQTTClient::sendPublishPacket(const char* topic, const char* message, QoS qos, bool retained) {
    Serial.println("Sending PUBLISH packet");

    uint16_t topicLength = strlen(topic);
    uint16_t messageLength = strlen(message);

    uint8_t packet[4 + topicLength + messageLength];
    uint8_t packetIndex = 0;

    packet[packetIndex++] = 0x30 | (qos << 1) | retained;  // PUBLISH packet type with QoS and retained flag
    packet[packetIndex++] = 0;  // Remaining length placeholder

    packet[packetIndex++] = (topicLength >> 8) & 0xFF;
    packet[packetIndex++] = topicLength & 0xFF;
    memcpy(&packet[packetIndex], topic, topicLength);
    packetIndex += topicLength;

    if (qos > 0) {
        packet[packetIndex++] = (packetIdentifier >> 8) & 0xFF;
        packet[packetIndex++] = packetIdentifier & 0xFF;
        pendingQoS1Messages[packetIdentifier] = message;
        packetIdentifier++;
    }

    memcpy(&packet[packetIndex], message, messageLength);
    packetIndex += messageLength;

    packet[1] = packetIndex - 2;  // Update remaining length

    wifiClient.write(packet, packetIndex);

    return true;
}

bool MQTTClient::sendSubscribePacket(const char* topic, QoS qos) {
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
    packet[packetIndex++] = qos;  // QoS

    wifiClient.write(packet, packetIndex);

    return true;
}

void MQTTClient::handlePacket() {
    if (wifiClient.available() > 0) {
        uint8_t packetType = wifiClient.read();
        if (packetType == 0x30) {  // Publish packet
            uint8_t remainingLength = wifiClient.read(); // Read the remaining length

            // Read the topic length
            uint16_t topicLength = (wifiClient.read() << 8) | wifiClient.read();
            remainingLength -= 2;

            // Read the topic
            char topic[topicLength + 1];
            if (wifiClient.readBytes(topic, topicLength) != topicLength) {
                Serial.println("Failed to read topic");
                return;
            }
            topic[topicLength] = '\0';
            remainingLength -= topicLength;

            // Read the payload
            char payload[remainingLength + 1];
            if (wifiClient.readBytes(payload, remainingLength) != remainingLength) {
                Serial.println("Failed to read payload");
                return;
            }
            payload[remainingLength] = '\0';

            // Call the callback with the topic and payload
            if (callback) {
                callback(topic, payload);
            }
        } else if (packetType == 0x40 || packetType == 0x50) {  // PUBACK or PUBREC
            processAckPacket();
        }
    }
}

void MQTTClient::processAckPacket() {
    uint8_t packetIdentifierBytes[2];
    if (wifiClient.read(packetIdentifierBytes, 2) == 2) {
        uint16_t receivedPacketIdentifier = (packetIdentifierBytes[0] << 8) | packetIdentifierBytes[1];

        Serial.print("Received ACK for packet ID: ");
        Serial.println(receivedPacketIdentifier);

        pendingQoS1Messages.erase(receivedPacketIdentifier);
    }
}

bool MQTTClient::mqttPing() {
    Serial.println("Sending PINGREQ to MQTT server");
    uint8_t pingReqPacket[] = {0xC0, 0x00};  // PINGREQ packet
    if (wifiClient.write(pingReqPacket, sizeof(pingReqPacket)) != sizeof(pingReqPacket)) {
        Serial.println("Failed to send PINGREQ packet");
        return false;
    }

    // Wait for PINGRESP
    delay(1000);  // Wait for response
    if (wifiClient.available() > 0) {
        uint8_t response[2];
        if (wifiClient.read(response, 2) == 2) {
            Serial.print("Received response: ");
            for (int i = 0; i < 2; i++) {
                Serial.print(response[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            return response[0] == 0xD0 && response[1] == 0x00;  // Check PINGRESP packet
        }
    }
    Serial.println("Failed to receive PINGRESP from MQTT server");
    return false;
}
