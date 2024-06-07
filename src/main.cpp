#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char *ssid = "OnePlus 9 Pro";             // Replace with your WiFi name
const char *password = "12345678";    

// MQTT Broker IP address
const char* mqtt_server = "156.17.228.87";

// Pin for the button
const int buttonPin = 4;  // Replace with the pin number connected to your button

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(); // Declare setup_wifi function
void callback(char* topic, byte* payload, unsigned int length); // Declare callback function
void reconnect(); // Declare reconnect function

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);  // Configure the button pin as input

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.publish("test/topic", "ESP32 connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Check if the button is pressed
  if (digitalRead(buttonPin) == HIGH) {
    client.publish("test/topic", "Button pressed");
    Serial.println("Button pressed, message sent");
    delay(1000);  // Debounce delay
  }
}
