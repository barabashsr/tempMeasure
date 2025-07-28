/**
 * @file MQTTManager.cpp
 * @brief Implementation of MQTT Manager using 256dpi/MQTT library
 * @author Claude Assistant
 * @date 2025-01-28
 */

#include "MQTTManager.h"
#include "TemperatureController.h"
#include <WiFi.h>

// Initialize static members
MQTTConfig MQTTManager::config;
bool MQTTManager::configLoaded = false;
WiFiClient MQTTManager::wifiClient;
WiFiClientSecure MQTTManager::wifiClientSecure;
MQTTClient MQTTManager::mqttClient(4096); // 4KB buffer for large messages
bool MQTTManager::isConnected = false;
unsigned long MQTTManager::lastReconnectAttempt = 0;
unsigned long MQTTManager::lastTelemetryPublish = 0;
unsigned long MQTTManager::publishIntervalMs = 60000;
unsigned long MQTTManager::publishCounter = 0;

/**
 * @brief Initialize MQTT manager
 */
bool MQTTManager::begin() {
    Serial.println("[MQTTManager] Initializing MQTT Manager (256dpi/MQTT)...");
    
    // Load configuration
    if (!loadConfig()) {
        Serial.println("[MQTTManager] Failed to load configuration, using defaults");
        // Set default HiveMQ Cloud values
        config.enabled = true;
        config.broker_host = "987bfd99193b4a21a18a665a3812cc90.s1.eu.hivemq.cloud";
        config.broker_port = 8883;
        config.use_tls = true;
        config.username = "ESP32-TempCont";
        config.password = "PolzaOil2019";
        config.client_id = "esp32_temp_controller";
        config.device_name = "esp32_temp_controller";
        config.test_publish_topic = "temp/pub";
        config.test_subscribe_topic = "temp/sub";
        configLoaded = true;
    }
    
    if (!config.enabled) {
        Serial.println("[MQTTManager] MQTT is disabled in configuration");
        return false;
    }
    
    Serial.println("[MQTTManager] Configuration:");
    Serial.printf("[MQTTManager]   Server: %s\n", config.broker_host.c_str());
    Serial.printf("[MQTTManager]   Port: %d\n", config.broker_port);
    Serial.printf("[MQTTManager]   Use TLS: %s\n", config.use_tls ? "Yes" : "No");
    Serial.printf("[MQTTManager]   Username: %s\n", config.username.c_str());
    Serial.printf("[MQTTManager]   Client ID: %s\n", config.client_id.c_str());
    
    // Configure WiFi client
    if (config.use_tls) {
        wifiClientSecure.setInsecure(); // Skip certificate verification for now
        mqttClient.begin(config.broker_host.c_str(), config.broker_port, wifiClientSecure);
    } else {
        mqttClient.begin(config.broker_host.c_str(), config.broker_port, wifiClient);
    }
    
    // Set callback
    mqttClient.onMessage(messageReceived);
    
    // Set options
    mqttClient.setOptions(30, true, 5000); // keepAlive, cleanSession, timeout
    
    Serial.println("[MQTTManager] MQTT Manager initialized");
    return true;
}

/**
 * @brief Connect to MQTT broker
 */
void MQTTManager::connect() {
    Serial.print("[MQTTManager] Connecting to MQTT broker...");
    
    // Set LWT if enabled
    if (config.lwt_enabled) {
        if (config.lwt_topic.isEmpty()) {
            config.lwt_topic = buildTopic("state", "connection");
        }
        if (config.lwt_message.isEmpty()) {
            config.lwt_message = "{\"status\":\"offline\"}";
        }
        mqttClient.setWill(config.lwt_topic.c_str(), config.lwt_message.c_str(), 
                          config.lwt_retain, config.lwt_qos);
    }
    
    bool connected = false;
    if (!config.username.isEmpty()) {
        connected = mqttClient.connect(config.client_id.c_str(), 
                                     config.username.c_str(), 
                                     config.password.c_str());
    } else {
        connected = mqttClient.connect(config.client_id.c_str());
    }
    
    if (connected) {
        Serial.println(" connected!");
        isConnected = true;
        
        // Subscribe to test topic
        mqttClient.subscribe(config.test_subscribe_topic, config.qos_commands);
        Serial.printf("[MQTTManager] Subscribed to: %s\n", config.test_subscribe_topic.c_str());
        
        // Send connection announcement
        testPublish("Device connected and ready");
        
        // Send online status
        if (config.lwt_enabled) {
            String onlineMsg = "{\"status\":\"online\",\"timestamp\":" + String(millis()) + "}";
            mqttClient.publish(config.lwt_topic, onlineMsg, config.lwt_retain, config.lwt_qos);
        }
    } else {
        Serial.printf(" failed, error = %d\n", mqttClient.lastError());
        switch (mqttClient.lastError()) {
            case LWMQTT_CONNECTION_DENIED:
                Serial.println("[MQTTManager] Connection denied");
                break;
            case LWMQTT_NETWORK_TIMEOUT:
                Serial.println("[MQTTManager] Network timeout");
                break;
            case LWMQTT_NETWORK_FAILED_CONNECT:
                Serial.println("[MQTTManager] Network connection failed");
                break;
            case LWMQTT_MISSING_OR_WRONG_PACKET:
                Serial.println("[MQTTManager] Protocol error");
                break;
            default:
                Serial.printf("[MQTTManager] Unknown error: %d\n", mqttClient.lastError());
        }
    }
}

/**
 * @brief Main update function that handles connection and publishing
 */
void MQTTManager::update(TemperatureController& controller) {
    if (!config.enabled) {
        return;
    }
    
    // Handle connection maintenance
    loop();
    
    // Publish data at configured interval
    if (mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastTelemetryPublish >= publishIntervalMs) {
            lastTelemetryPublish = now;
            
            // Publish temperature data
            if (publishTemperatureData(controller)) {
                Serial.println("[MQTTManager] Temperature data published to MQTT");
            }
            
            // Publish system status
            if (publishSystemStatus(controller)) {
                Serial.println("[MQTTManager] System status published to MQTT");
            }
        }
    }
}

/**
 * @brief Internal loop function for connection maintenance
 */
void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        if (isConnected) {
            Serial.println("[MQTTManager] MQTT connection lost!");
            isConnected = false;
        }
        
        // Reconnect with interval
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            connect();
        }
    } else {
        // Maintain connection
        mqttClient.loop();
        
        if (!isConnected) {
            isConnected = true;
        }
    }
}

/**
 * @brief Message received callback
 */
void MQTTManager::messageReceived(String &topic, String &payload) {
    Serial.println("[MQTTManager] ========== MESSAGE RECEIVED ==========");
    Serial.printf("[MQTTManager] Topic: %s\n", topic.c_str());
    Serial.printf("[MQTTManager] Payload: %s\n", payload.c_str());
    Serial.println("[MQTTManager] =====================================");
}

/**
 * @brief Publish message
 */
bool MQTTManager::publish(const char* topic, const char* payload, bool retain, int qos) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish - not connected");
        return false;
    }
    
    return mqttClient.publish(topic, payload, retain, qos);
}

/**
 * @brief Publish message (String version)
 */
bool MQTTManager::publish(const String& topic, const String& payload, bool retain, int qos) {
    return publish(topic.c_str(), payload.c_str(), retain, qos);
}

/**
 * @brief Test publish to configured test topic
 */
bool MQTTManager::testPublish(const String& message) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish - not connected");
        return false;
    }
    
    if (config.test_publish_topic.isEmpty()) {
        Serial.println("[MQTTManager] Test publish topic not configured");
        return false;
    }
    
    Serial.printf("[MQTTManager] Test publishing to %s: %s\n", 
                  config.test_publish_topic.c_str(), message.c_str());
    
    bool result = mqttClient.publish(config.test_publish_topic, message, 
                                   config.retain_telemetry, config.qos_telemetry);
    
    if (result) {
        Serial.println("[MQTTManager] Test publish successful");
    } else {
        Serial.println("[MQTTManager] Test publish failed");
    }
    
    return result;
}

/**
 * @brief Build topic string based on configuration
 */
String MQTTManager::buildTopic(const String& topicType, const String& subtopic) {
    String topic = "";
    
    if (config.topic_level1_type != "skip" && !config.topic_level1_value.isEmpty()) {
        topic += config.topic_level1_value + "/";
    }
    
    if (config.topic_level2_type != "skip" && !config.topic_level2_value.isEmpty()) {
        topic += config.topic_level2_value + "/";
    }
    
    if (config.topic_level3_type != "skip" && !config.topic_level3_value.isEmpty()) {
        topic += config.topic_level3_value + "/";
    }
    
    topic += config.device_name + "/";
    topic += topicType;
    
    if (!subtopic.isEmpty()) {
        topic += "/" + subtopic;
    }
    
    return topic;
}

/**
 * @brief Load configuration from JSON file
 */
bool MQTTManager::loadConfig() {
    Serial.println("[MQTTManager] Loading configuration from SD card...");
    
    if (!SD.begin()) {
        Serial.println("[MQTTManager] SD card not available");
        return false;
    }
    
    const char* configPath = "/config/mqtt.json";
    if (!SD.exists(configPath)) {
        Serial.println("[MQTTManager] Config file not found: /config/mqtt.json");
        return false;
    }
    
    File configFile = SD.open(configPath, FILE_READ);
    if (!configFile) {
        Serial.println("[MQTTManager] Failed to open config file");
        return false;
    }
    
    String jsonContent = "";
    while (configFile.available()) {
        jsonContent += configFile.readString();
    }
    configFile.close();
    
    Serial.printf("[MQTTManager] Config file size: %d bytes\n", jsonContent.length());
    
    return setConfigJson(jsonContent);
}

/**
 * @brief Save configuration to JSON file
 */
bool MQTTManager::saveConfig() {
    Serial.println("[MQTTManager] Saving configuration to SD card...");
    
    if (!SD.begin()) {
        Serial.println("[MQTTManager] SD card not available");
        return false;
    }
    
    if (!SD.exists("/config")) {
        Serial.println("[MQTTManager] Creating /config directory");
        if (!SD.mkdir("/config")) {
            Serial.println("[MQTTManager] Failed to create config directory");
            return false;
        }
    }
    
    String jsonContent = getConfigJson();
    
    const char* configPath = "/config/mqtt.json";
    File configFile = SD.open(configPath, FILE_WRITE);
    if (!configFile) {
        Serial.println("[MQTTManager] Failed to open config file for writing");
        return false;
    }
    
    size_t written = configFile.print(jsonContent);
    configFile.close();
    
    Serial.printf("[MQTTManager] Written %d bytes to config file\n", written);
    
    return written == jsonContent.length();
}

/**
 * @brief Get configuration as JSON string
 */
String MQTTManager::getConfigJson() {
    JsonDocument doc;
    
    doc["enabled"] = config.enabled;
    doc["broker_host"] = config.broker_host;
    doc["broker_port"] = config.broker_port;
    doc["use_tls"] = config.use_tls;
    doc["username"] = config.username;
    doc["password"] = config.password;
    doc["client_id"] = config.client_id;
    doc["device_name"] = config.device_name;
    
    JsonObject topics = doc["topics"].to<JsonObject>();
    topics["level1_type"] = config.topic_level1_type;
    topics["level1_value"] = config.topic_level1_value;
    topics["level2_type"] = config.topic_level2_type;
    topics["level2_value"] = config.topic_level2_value;
    topics["level3_type"] = config.topic_level3_type;
    topics["level3_value"] = config.topic_level3_value;
    
    JsonObject publishing = doc["publishing"].to<JsonObject>();
    publishing["retain_telemetry"] = config.retain_telemetry;
    publishing["retain_alarms"] = config.retain_alarms;
    publishing["retain_state"] = config.retain_state;
    
    JsonObject qos = doc["qos"].to<JsonObject>();
    qos["telemetry"] = config.qos_telemetry;
    qos["alarms"] = config.qos_alarms;
    qos["commands"] = config.qos_commands;
    
    JsonObject lwt = doc["lwt"].to<JsonObject>();
    lwt["enabled"] = config.lwt_enabled;
    lwt["topic"] = config.lwt_topic;
    lwt["message"] = config.lwt_message;
    lwt["qos"] = config.lwt_qos;
    lwt["retain"] = config.lwt_retain;
    
    JsonObject test = doc["test"].to<JsonObject>();
    test["publish_topic"] = config.test_publish_topic;
    test["subscribe_topic"] = config.test_subscribe_topic;
    
    JsonObject intervals = doc["intervals"].to<JsonObject>();
    intervals["telemetry"] = config.telemetry_interval;
    
    String output;
    output.reserve(1024);
    serializeJsonPretty(doc, output);
    return output;
}

/**
 * @brief Set configuration from JSON string
 */
bool MQTTManager::setConfigJson(const String& jsonStr) {
    Serial.println("[MQTTManager] Parsing JSON configuration...");
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.printf("[MQTTManager] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    config.enabled = doc["enabled"] | false;
    config.broker_host = doc["broker_host"] | "";
    config.broker_port = doc["broker_port"] | 1883;
    config.use_tls = doc["use_tls"] | false;
    config.username = doc["username"] | "";
    config.password = doc["password"] | "";
    config.client_id = doc["client_id"] | "esp32_temp_controller";
    config.device_name = doc["device_name"] | "esp32_temp_controller";
    
    JsonObject topics = doc["topics"];
    if (!topics.isNull()) {
        config.topic_level1_type = topics["level1_type"] | "skip";
        config.topic_level1_value = topics["level1_value"] | "";
        config.topic_level2_type = topics["level2_type"] | "skip";
        config.topic_level2_value = topics["level2_value"] | "";
        config.topic_level3_type = topics["level3_type"] | "skip";
        config.topic_level3_value = topics["level3_value"] | "";
    }
    
    JsonObject publishing = doc["publishing"];
    if (!publishing.isNull()) {
        config.retain_telemetry = publishing["retain_telemetry"] | false;
        config.retain_alarms = publishing["retain_alarms"] | true;
        config.retain_state = publishing["retain_state"] | true;
    }
    
    JsonObject qos = doc["qos"];
    if (!qos.isNull()) {
        config.qos_telemetry = qos["telemetry"] | 0;
        config.qos_alarms = qos["alarms"] | 1;
        config.qos_commands = qos["commands"] | 2;
    }
    
    JsonObject lwt = doc["lwt"];
    if (!lwt.isNull()) {
        config.lwt_enabled = lwt["enabled"] | true;
        config.lwt_topic = lwt["topic"] | "";
        config.lwt_message = lwt["message"] | "";
        config.lwt_qos = lwt["qos"] | 1;
        config.lwt_retain = lwt["retain"] | true;
    }
    
    JsonObject test = doc["test"];
    if (!test.isNull()) {
        config.test_publish_topic = test["publish_topic"] | "test/pub";
        config.test_subscribe_topic = test["subscribe_topic"] | "test/sub";
    }
    
    JsonObject intervals = doc["intervals"];
    if (!intervals.isNull()) {
        config.telemetry_interval = intervals["telemetry"] | 60;
        if (config.telemetry_interval < 10) {
            config.telemetry_interval = 10;
        }
    }
    
    setPublishInterval(config.telemetry_interval);
    
    if (config.lwt_enabled && config.lwt_topic.isEmpty()) {
        config.lwt_topic = buildTopic("state", "connection");
    }
    
    if (config.lwt_enabled && config.lwt_message.isEmpty()) {
        config.lwt_message = "{\"status\":\"offline\",\"timestamp\":\"" + String(millis()) + "\"}";
    }
    
    configLoaded = true;
    Serial.println("[MQTTManager] Configuration parsed successfully");
    return true;
}

/**
 * @brief Publish temperature data
 */
bool MQTTManager::publishTemperatureData(TemperatureController& controller) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish temperature data - not connected");
        return false;
    }
    
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["device_name"] = config.device_name;
    
    JsonArray points = doc["measurement_points"].to<JsonArray>();
    
    // Add DS18B20 points
    for (uint8_t i = 0; i < 50; i++) {
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (point && point->getBoundSensor() != nullptr) {
            JsonObject pointObj = points.add<JsonObject>();
            pointObj["address"] = point->getAddress();
            pointObj["name"] = point->getName();
            pointObj["type"] = "DS18B20";
            pointObj["value"] = point->getCurrentTemp();
            pointObj["min"] = point->getMinTemp();
            pointObj["max"] = point->getMaxTemp();
            pointObj["alarm_status"] = point->getAlarmStatus();
            pointObj["error_status"] = point->getErrorStatus();
        }
    }
    
    // Add PT1000 points
    for (uint8_t i = 0; i < 10; i++) {
        MeasurementPoint* point = controller.getPT1000Point(i);
        if (point && point->getBoundSensor() != nullptr) {
            JsonObject pointObj = points.add<JsonObject>();
            pointObj["address"] = point->getAddress();
            pointObj["name"] = point->getName();
            pointObj["type"] = "PT1000";
            pointObj["value"] = point->getCurrentTemp();
            pointObj["min"] = point->getMinTemp();
            pointObj["max"] = point->getMaxTemp();
            pointObj["alarm_status"] = point->getAlarmStatus();
            pointObj["error_status"] = point->getErrorStatus();
        }
    }
    
    String topic = buildTopic("telemetry", "temperature");
    String payload;
    serializeJson(doc, payload);
    
    if (payload.length() > 4096) {
        Serial.printf("[MQTTManager] Warning: Large payload size: %d bytes\n", payload.length());
    }
    
    bool result = mqttClient.publish(topic, payload, config.retain_telemetry, config.qos_telemetry);
    
    if (result) {
        Serial.printf("[MQTTManager] Temperature data published to %s (%d bytes)\n", 
                      topic.c_str(), payload.length());
    } else {
        Serial.println("[MQTTManager] Failed to publish temperature data");
    }
    
    return result;
}

/**
 * @brief Publish system status
 */
bool MQTTManager::publishSystemStatus(TemperatureController& controller) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish system status - not connected");
        return false;
    }
    
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["device_name"] = config.device_name;
    doc["device_id"] = controller.getDeviceId();
    doc["firmware_version"] = controller.getFirmwareVersion();
    doc["uptime"] = millis() / 1000;
    
    JsonObject alarms = doc["alarms"].to<JsonObject>();
    alarms["active_count"] = controller.getActiveAlarms().size();
    alarms["acknowledged_count"] = controller.getAlarmCount(AlarmStage::ACKNOWLEDGED);
    alarms["total_count"] = controller.getAlarmCount();
    
    JsonObject network = doc["network"].to<JsonObject>();
    network["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    if (WiFi.status() == WL_CONNECTED) {
        network["wifi_ssid"] = WiFi.SSID();
        network["wifi_rssi"] = WiFi.RSSI();
        network["ip_address"] = WiFi.localIP().toString();
    }
    
    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["free_heap"] = ESP.getFreeHeap();
    memory["min_free_heap"] = ESP.getMinFreeHeap();
    memory["heap_size"] = ESP.getHeapSize();
    
    String topic = buildTopic("telemetry", "status");
    String payload;
    serializeJson(doc, payload);
    
    bool result = mqttClient.publish(topic, payload, config.retain_state, config.qos_telemetry);
    
    if (result) {
        Serial.printf("[MQTTManager] System status published to %s\n", topic.c_str());
    } else {
        Serial.println("[MQTTManager] Failed to publish system status");
    }
    
    return result;
}

/**
 * @brief Set new configuration
 */
bool MQTTManager::setConfig(const MQTTConfig& newConfig) {
    Serial.println("[MQTTManager] Applying new configuration...");
    
    config = newConfig;
    configLoaded = true;
    
    if (!saveConfig()) {
        Serial.println("[MQTTManager] Warning: Failed to save configuration to SD card");
    }
    
    // Disconnect if connected
    if (mqttClient.connected()) {
        Serial.println("[MQTTManager] Disconnecting to apply new configuration...");
        mqttClient.disconnect();
    }
    
    // Reconfigure client
    if (config.use_tls) {
        wifiClientSecure.setInsecure();
        mqttClient.begin(config.broker_host.c_str(), config.broker_port, wifiClientSecure);
    } else {
        mqttClient.begin(config.broker_host.c_str(), config.broker_port, wifiClient);
    }
    
    // Reconnect if enabled
    if (config.enabled && WiFi.isConnected()) {
        connect();
    }
    
    return true;
}