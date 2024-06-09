#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <WiFiClient.h>
#include <functional>
#include <string>
#include <map>

class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;
    using QoS = uint8_t;

    MQTTClient(const char* server, uint16_t port);
    bool connect(const char* clientID);
    void disconnect();
    void loop();
    bool publish(const char* topic, const char* message, QoS qos = 0, bool retained = false);
    bool subscribe(const char* topic, QoS qos = 0);
    void setMessageCallback(MessageCallback callback);
    void setKeepAlive(uint16_t keepAlive);

private:
    bool sendConnectPacket(const char* clientID);
    bool sendPublishPacket(const char* topic, const char* message, QoS qos, bool retained);
    bool sendSubscribePacket(const char* topic, QoS qos);
    void handlePacket();
    void processAckPacket();
    bool mqttPing();

    WiFiClient wifiClient;
    std::string server;
    uint16_t port;
    uint16_t keepAlive;
    uint32_t lastPingTime;
    MessageCallback callback;
    uint16_t packetIdentifier;
    uint8_t buffer[128]; // Buffer for incoming data

    std::map<uint16_t, std::string> pendingQoS1Messages; // Stores messages pending QoS 1 acknowledgment
};

#endif // MQTTCLIENT_H