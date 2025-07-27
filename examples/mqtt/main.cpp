#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Tenda_B3E6F0_EXT";
const char* password = "a111222333";

// HiveMQ Cloud connection details
const char* mqtt_server = "987bfd99193b4a21a18a665a3812cc90.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;  // SSL port for HiveMQ Cloud
const char* mqtt_username = "ESP32-TempCont";
const char* mqtt_password = "PolzaOil2019";

// MQTT Topics
const char* temperature_topic = "esp32/temperature/data";
const char* command_topic = "esp32/commands/set_temp";
const char* status_topic = "esp32/status/response";

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Device info
String deviceId = "esp32_temp_controller_01";
float currentTemp = 23.5; // Simulated temperature

// Function declarations
void connectWiFi();
void setupMQTT();
void connectMQTT();
void onMqttMessage(char* topic, byte* payload, unsigned int length);
void handleTemperatureCommand(String command);
void sendTemperatureReading();
void sendStatusMessage(String message);
void sendCustomMessage(String message);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 MQTT Temperature Controller");
  Serial.println("Commands:");
  Serial.println("  temp - Send temperature reading");
  Serial.println("  status - Send device status");
  Serial.println("  msg:<text> - Send custom message");
  
  connectWiFi();
  setupMQTT();
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  // For HiveMQ Cloud, we need to skip certificate verification
  // In production, you should use proper certificates
  wifiClient.setInsecure();
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(onMqttMessage);
  
  connectMQTT();
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect(deviceId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println(" connected!");
      
      // Subscribe to command topic
      mqttClient.subscribe(command_topic);
      Serial.println("Subscribed to: " + String(command_topic));
      
      // Send online status
      sendStatusMessage("Device connected and ready");
      
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Received MQTT message:");
  Serial.println("Topic: " + String(topic));
  Serial.println("Message: " + message);
  
  // Handle temperature set commands
  if (String(topic) == command_topic) {
    handleTemperatureCommand(message);
  }
}

void handleTemperatureCommand(String command) {
  // Parse JSON command from n8n
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, command);
  
  if (doc.containsKey("action")) {
    String action = doc["action"];
    
    if (action == "set_temperature") {
      float newTemp = doc["value"];
      Serial.println("Setting temperature to: " + String(newTemp) + "°C");
      
      // Here you would control your actual temperature controller
      // For demo, we just acknowledge
      sendStatusMessage("Temperature set to " + String(newTemp) + "°C");
      
    } else if (action == "get_status") {
      sendTemperatureReading();
    }
  }
}

void sendTemperatureReading() {
  // Simulate temperature reading (add some variation)
  currentTemp += random(-10, 10) / 10.0;
  
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceId;
  doc["temperature"] = currentTemp;
  doc["unit"] = "celsius";
  doc["timestamp"] = millis();
  doc["wifi_rssi"] = WiFi.RSSI();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  if (mqttClient.publish(temperature_topic, jsonString.c_str())) {
    Serial.println("Temperature data sent: " + jsonString);
  } else {
    Serial.println("Failed to send temperature data");
  }
}

void sendStatusMessage(String message) {
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceId;
  doc["status"] = message;
  doc["timestamp"] = millis();
  doc["uptime"] = millis() / 1000;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  if (mqttClient.publish(status_topic, jsonString.c_str())) {
    Serial.println("Status sent: " + jsonString);
  } else {
    Serial.println("Failed to send status");
  }
}

void sendCustomMessage(String message) {
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceId;
  doc["custom_message"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  if (mqttClient.publish(temperature_topic, jsonString.c_str())) {
    Serial.println("Custom message sent: " + jsonString);
  } else {
    Serial.println("Failed to send custom message");
  }
}

void loop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    Serial.println("Received command: " + command);
    
    if (command == "temp") {
      sendTemperatureReading();
      
    } else if (command == "status") {
      sendStatusMessage("Device running normally");
      
    } else if (command.startsWith("msg:")) {
      String customMsg = command.substring(4);
      sendCustomMessage(customMsg);
      
    } else {
      Serial.println("Unknown command. Available: temp, status, msg:<text>");
    }
  }
  
  delay(100);
}
