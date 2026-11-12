#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Onboard LED pin (GPIO 2 on most ESP32 Dev Modules)
#define LED_PIN 2

// WiFi credentials
const char* ssid = "Galaxy A52B2DE";
const char* password = "supun4748";

// VPS MQTT broker Details
const char* mqtt_server = "198.144.177.27"; 
const int mqtt_port = 1883;                 
const char* mqtt_user = "esp_test_user";       
const char* mqtt_pass = "Supun@2001"; 

// Dynamic Device & Topic strings
String deviceId;
String telemetryTopic;
String commandTopic;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
    String command = "";
    for (int i = 0; i < length; i++) {
        command += (char)payload[i];
    }
    
    Serial.print("[");
    Serial.print(deviceId);
    Serial.print("] Command received: ");
    Serial.println(command);

    // Physical LED control
    if (command == "LED_ON") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println(">> LED is ON");
    } else if (command == "LED_OFF") {
        digitalWrite(LED_PIN, LOW);
        Serial.println(">> LED is OFF");
    }
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT broker as [");
        Serial.print(deviceId);
        Serial.print("]...");

        if (client.connect(deviceId.c_str(), mqtt_user, mqtt_pass)) {
            Serial.println(" Connected!");
            client.subscribe(commandTopic.c_str());
            Serial.print("Subscribed to: ");
            Serial.println(commandTopic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(". Retrying in 3 seconds...");
            delay(3000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Read full 64-bit hardware MAC (efuse)
    uint64_t chipMac = ESP.getEfuseMac();

    // Format full 12-character MAC hex string (e.g. D83A3425BF58 vs DC2F3425BF58)
    char fullMacId[13];
    snprintf(fullMacId, sizeof(fullMacId), "%04X%08X", (uint16_t)(chipMac >> 32), (uint32_t)chipMac);

    deviceId = "ESP32_" + String(fullMacId);

    // Dynamically generate topics
    telemetryTopic = "supun/esp32/" + deviceId + "/telemetry";
    commandTopic   = "supun/esp32/" + deviceId + "/command";

    Serial.println("\n==================================");
    Serial.print("Device Initialized: ");
    Serial.println(deviceId);
    Serial.print("Telemetry Topic:   ");
    Serial.println(telemetryTopic);
    Serial.print("Command Topic:     ");
    Serial.println(commandTopic);
    Serial.println("==================================\n");

    connectWiFi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 5000) {
        lastSend = millis();

        // Slightly varying simulated temperature (25.0 - 32.0 C)
        float mockTemp = 26.0 + (random(0, 60) / 10.0);
        
        String message = "{\"deviceId\":\"" + deviceId + "\",\"temperature\":" + String(mockTemp, 1) + "}";
        client.publish(telemetryTopic.c_str(), message.c_str());
        
        Serial.print("[");
        Serial.print(deviceId);
        Serial.print("] Published: ");
        Serial.println(message);
    }
}
