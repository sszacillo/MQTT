#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>

class MQTTClient {
public:
    MQTTClient(WiFiClient& client, const char* server, uint16_t port);

    void setCallback(void (*callback)(char*, byte*, unsigned int));
    bool connect(const char* clientID);
    void loop();
    bool publish(const char* topic, const char* message);
    bool subscribe(const char* topic);

private:
    const char* server;
    uint16_t port;
    WiFiClient& wifiClient;
    void (*callback)(char*, byte*, unsigned int);
    uint16_t packetIdentifier;

    bool sendConnectPacket(const char* clientID);
    bool sendPublishPacket(const char* topic, const char* message);
    bool sendSubscribePacket(const char* topic);
    void handlePacket();
};

#endif
