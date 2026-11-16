#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Onboard / External LED pin (GPIO 2)
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
String stateTopic;

// Track real physical LED state
bool ledState = false;

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

    // Physical LED control & Instant State Feedback
    if (command == "LED_ON") {
        digitalWrite(LED_PIN, HIGH);
        ledState = true;
        client.publish(stateTopic.c_str(), "ON", true); // retained state
        Serial.println(">> LED is ON (State reported: ON)");
    } else if (command == "LED_OFF") {
        digitalWrite(LED_PIN, LOW);
        ledState = false;
        client.publish(stateTopic.c_str(), "OFF", true); // retained state
        Serial.println(">> LED is OFF (State reported: OFF)");
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

            // Report initial state on connection
            client.publish(stateTopic.c_str(), ledState ? "ON" : "OFF", true);
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
    ledState = false;

    // Read full 64-bit hardware MAC (efuse)
    uint64_t chipMac = ESP.getEfuseMac();

    // Format full 12-character MAC hex string
    char fullMacId[13];
    snprintf(fullMacId, sizeof(fullMacId), "%04X%08X", (uint16_t)(chipMac >> 32), (uint32_t)chipMac);

    deviceId = "ESP32_" + String(fullMacId);

    // Dynamically generate topics
    telemetryTopic = "supun/esp32/" + deviceId + "/telemetry";
    commandTopic   = "supun/esp32/" + deviceId + "/command";
    stateTopic     = "supun/esp32/" + deviceId + "/state";

    Serial.println("\n==================================");
    Serial.print("Device Initialized: ");
    Serial.println(deviceId);
    Serial.print("Telemetry Topic:   ");
    Serial.println(telemetryTopic);
    Serial.print("Command Topic:     ");
    Serial.println(commandTopic);
    Serial.print("State Topic:       ");
    Serial.println(stateTopic);
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

        // Simulated temperature
        float mockTemp = 26.0 + (random(0, 60) / 10.0);
        
        // Include both temperature and current real LED state in telemetry
        String message = "{\"deviceId\":\"" + deviceId + 
                         "\",\"temperature\":" + String(mockTemp, 1) + 
                         ",\"led\":\"" + (ledState ? "ON" : "OFF") + "\"}";
                         
        client.publish(telemetryTopic.c_str(), message.c_str());
        
        Serial.print("[");
        Serial.print(deviceId);
        Serial.print("] Published: ");
        Serial.println(message);
    }
}
