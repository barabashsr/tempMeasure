/**
 * @file MQTTManager.cpp
 * @brief Implementation of basic MQTT Manager for temperature controller
 * @author Claude Assistant
 * @date 2025-01-27
 */

#include "MQTTManager.h"

// Initialize static members
MQTTConfig MQTTManager::config;
bool MQTTManager::configLoaded = false;
WiFiClientSecure MQTTManager::wifiClient;
WiFiClient MQTTManager::wifiClientInsecure;
PubSubClient MQTTManager::mqttClient(wifiClient);
bool MQTTManager::isConnected = false;
unsigned long MQTTManager::lastReconnectAttempt = 0;
unsigned long MQTTManager::lastPublishTime = 0;
unsigned long MQTTManager::publishCounter = 0;
bool MQTTManager::connectInProgress = false;
unsigned long MQTTManager::connectStartTime = 0;

/**
 * @brief Initialize MQTT manager
 * @return true if initialization successful
 */
bool MQTTManager::begin() {
    Serial.println("[MQTTManager] Initializing MQTT Manager...");
    
    // Load configuration from SD card
    if (!loadConfig()) {
        Serial.println("[MQTTManager] Failed to load configuration, using defaults");
        // Set some default values for testing
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
    
    // Check if MQTT is enabled
    if (!config.enabled) {
        Serial.println("[MQTTManager] MQTT is disabled in configuration");
        return false;
    }
    
    // Temporary: Test with non-TLS to isolate TLS issues
    // config.use_tls = false;
    // config.broker_port = 1883;
    // Serial.println("[MQTTManager] WARNING: Temporarily disabled TLS for testing!");
    
    Serial.println("[MQTTManager] Configuration:");
    Serial.print("[MQTTManager]   Server: ");
    Serial.println(config.broker_host.c_str());
    Serial.print("[MQTTManager]   Port: ");
    Serial.println(config.broker_port);
    Serial.print("[MQTTManager]   Use TLS: ");
    Serial.println(config.use_tls ? "Yes" : "No");
    Serial.print("[MQTTManager]   Username: ");
    Serial.println(config.username.c_str());
    Serial.print("[MQTTManager]   Client ID: ");
    Serial.println(config.client_id.c_str());
    Serial.print("[MQTTManager]   Device Name: ");
    Serial.println(config.device_name.c_str());
    Serial.print("[MQTTManager]   Test Publish Topic: ");
    Serial.println(config.test_publish_topic.c_str());
    Serial.print("[MQTTManager]   Test Subscribe Topic: ");
    Serial.println(config.test_subscribe_topic.c_str());
    
    // Configure WiFi client based on TLS setting
    if (config.use_tls) {
        Serial.println("[MQTTManager] Configuring WiFiClientSecure...");
        wifiClient.setInsecure();
        Serial.println("[MQTTManager] SSL verification disabled (setInsecure)");
        mqttClient.setClient(wifiClient);
    } else {
        Serial.println("[MQTTManager] Using insecure WiFi client...");
        mqttClient.setClient(wifiClientInsecure);
    }
    
    // Configure MQTT client with increased buffer size
    Serial.println("[MQTTManager] Configuring MQTT client...");
    mqttClient.setServer(config.broker_host.c_str(), config.broker_port);
    mqttClient.setCallback(messageCallback);
    mqttClient.setBufferSize(512);  // Increase buffer size for larger messages
    Serial.println("[MQTTManager] MQTT client configured");
    Serial.println("[MQTTManager] Buffer size set to 512 bytes");
    
    // Don't attempt connection here - let loop() handle it
    Serial.println("[MQTTManager] Initialization complete - connection will be attempted in loop");
    return true;
}

/**
 * @brief Main loop to handle MQTT operations
 */
void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        if (isConnected) {
            // First time noticing disconnection
            Serial.println("[MQTTManager] MQTT connection lost!");
            isConnected = false;
        }
        
        // Check if it's time to reconnect
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            Serial.println("[MQTTManager] Attempting reconnection...");
            lastReconnectAttempt = now;
            
            // Simple blocking connection like the example
            connectMQTT();
        }
    } else {
        // Maintain connection
        mqttClient.loop();
        
        // Mark as connected if not already
        if (!isConnected) {
            isConnected = true;
        }
        
        // Publish incrementing counter every second (only if config is loaded)
        if (configLoaded) {
            unsigned long now = millis();
            if (now - lastPublishTime >= PUBLISH_INTERVAL) {
                lastPublishTime = now;
                
                // Create message with incrementing counter
                String message = String(publishCounter++);
                
                Serial.print("[MQTTManager] Publishing counter: ");
                Serial.println(message);
                
                if (testPublish(message)) {
                    Serial.println("[MQTTManager] Counter published successfully");
                } else {
                    Serial.println("[MQTTManager] Failed to publish counter");
                }
            }
        }
    }
}

/**
 * @brief Connect to MQTT broker (blocking, like the example)
 */
void MQTTManager::connectMQTT() {
    Serial.print("[MQTTManager] Attempting MQTT connection...");
    Serial.printf(" (Free heap: %d bytes)\n", ESP.getFreeHeap());
    
    // Simple connection like the example
    if (mqttClient.connect(config.client_id.c_str(), 
                          config.username.c_str(), 
                          config.password.c_str())) {
        Serial.println("[MQTTManager] Connected!");
        
        // Subscribe to test topic
        if (mqttClient.subscribe(config.test_subscribe_topic.c_str())) {
            Serial.printf("[MQTTManager] Subscribed to: %s\n", config.test_subscribe_topic.c_str());
        } else {
            Serial.println("[MQTTManager] Failed to subscribe!");
        }
        
        // Send connection announcement
        testPublish("Device connected and ready");
        
    } else {
        Serial.print("[MQTTManager] Failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" trying again in 5 seconds");
        
        // Decode error for debugging
        switch(mqttClient.state()) {
            case -4: Serial.println("[MQTTManager] MQTT_CONNECTION_TIMEOUT"); break;
            case -3: Serial.println("[MQTTManager] MQTT_CONNECTION_LOST"); break;
            case -2: Serial.println("[MQTTManager] MQTT_CONNECT_FAILED"); break;
            case -1: Serial.println("[MQTTManager] MQTT_DISCONNECTED"); break;
            case 1:  Serial.println("[MQTTManager] MQTT_CONNECT_BAD_PROTOCOL"); break;
            case 2:  Serial.println("[MQTTManager] MQTT_CONNECT_BAD_CLIENT_ID"); break;
            case 3:  Serial.println("[MQTTManager] MQTT_CONNECT_UNAVAILABLE"); break;
            case 4:  Serial.println("[MQTTManager] MQTT_CONNECT_BAD_CREDENTIALS"); break;
            case 5:  Serial.println("[MQTTManager] MQTT_CONNECT_UNAUTHORIZED"); break;
        }
    }
}

/**
 * @brief Attempt to connect to MQTT broker (non-blocking) - DEPRECATED
 * @return true if connection started
 */
bool MQTTManager::attemptConnection() {
    // This method is deprecated - using simple blocking connection instead
    connectMQTT();
    return true;
}

/**
 * @brief Check non-blocking connection progress
 * @return true if connection complete (success or failure)
 */
bool MQTTManager::checkConnectionProgress() {
    // Check timeout
    unsigned long elapsed = millis() - connectStartTime;
    if (elapsed > CONNECT_TIMEOUT) {
        Serial.println("[MQTTManager] Connection timeout!");
        Serial.printf("[MQTTManager] Elapsed time: %lu ms\n", elapsed);
        isConnected = false;
        connectInProgress = false;
        return true; // Connection complete (failed)
    }
    
    // Show progress every second
    static unsigned long lastProgress = 0;
    if (millis() - lastProgress > 1000) {
        lastProgress = millis();
        Serial.printf("[MQTTManager] Connection in progress... (%lu ms)\n", elapsed);
    }
    
    // Try to connect
    Serial.println("[MQTTManager] Attempting MQTT connection...");
    bool connected = false;
    
    // For debugging, let's use a simpler connection first
    connected = mqttClient.connect(config.client_id.c_str(), 
                                 config.username.c_str(), 
                                 config.password.c_str());
    
    if (connected) {
        Serial.println("[MQTTManager] CONNECTED!");
        Serial.printf("[MQTTManager] Free heap after connection: %d bytes\n", ESP.getFreeHeap());
        isConnected = true;
        connectInProgress = false;
        
        // Subscribe to test topic
        Serial.print("[MQTTManager] Subscribing to topic: ");
        Serial.print(config.test_subscribe_topic.c_str());
        
        if (mqttClient.subscribe(config.test_subscribe_topic.c_str())) {
            Serial.println(" - SUCCESS!");
        } else {
            Serial.println(" - FAILED!");
        }
        
        // Send connection announcement
        Serial.println("[MQTTManager] Sending connection announcement...");
        testPublish("Device connected and ready");
        
        return true; // Connection complete (success)
    } else {
        int state = mqttClient.state();
        Serial.printf("[MQTTManager] Connection attempt failed, state: %d\n", state);
        
        // Decode error codes
        switch(state) {
            case -4: 
                Serial.println("[MQTTManager] MQTT_CONNECTION_TIMEOUT - Network/TLS timeout"); 
                connectInProgress = false;
                return true; // Stop trying
            case -3: 
                Serial.println("[MQTTManager] MQTT_CONNECTION_LOST"); 
                break;
            case -2: 
                Serial.println("[MQTTManager] MQTT_CONNECT_FAILED - Still connecting..."); 
                return false; // Keep trying
            case -1: 
                Serial.println("[MQTTManager] MQTT_DISCONNECTED"); 
                break;
            case 0:  
                Serial.println("[MQTTManager] MQTT_CONNECTED"); 
                break;
            case 1:  
                Serial.println("[MQTTManager] MQTT_CONNECT_BAD_PROTOCOL"); 
                connectInProgress = false;
                return true;
            case 2:  
                Serial.println("[MQTTManager] MQTT_CONNECT_BAD_CLIENT_ID"); 
                connectInProgress = false;
                return true;
            case 3:  
                Serial.println("[MQTTManager] MQTT_CONNECT_UNAVAILABLE"); 
                connectInProgress = false;
                return true;
            case 4:  
                Serial.println("[MQTTManager] MQTT_CONNECT_BAD_CREDENTIALS"); 
                connectInProgress = false;
                return true;
            case 5:  
                Serial.println("[MQTTManager] MQTT_CONNECT_UNAUTHORIZED"); 
                connectInProgress = false;
                return true;
            default: 
                Serial.printf("[MQTTManager] UNKNOWN ERROR: %d\n", state); 
                break;
        }
        
        // For specific errors, stop trying
        if (state > 0) {
            isConnected = false;
            connectInProgress = false;
            return true; // Connection complete (failed)
        }
        
        // Still trying to connect
        return false;
    }
}

/**
 * @brief Static callback function for MQTT messages
 */
void MQTTManager::messageCallback(char* topic, byte* payload, unsigned int length) {
    handleMessage(topic, payload, length);
}

/**
 * @brief Handle received MQTT messages
 */
void MQTTManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    Serial.println("[MQTTManager] ========== MESSAGE RECEIVED ==========");
    Serial.print("[MQTTManager] Topic: ");
    Serial.println(topic);
    Serial.print("[MQTTManager] Length: ");
    Serial.print(length);
    Serial.println(" bytes");
    Serial.print("[MQTTManager] Payload (raw): ");
    
    // Print raw bytes
    for (unsigned int i = 0; i < length; i++) {
        Serial.print("0x");
        if (payload[i] < 16) Serial.print("0");
        Serial.print(payload[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Convert to string and print
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("[MQTTManager] Payload (string): ");
    Serial.println(message);
    Serial.println("[MQTTManager] =====================================");
}

/**
 * @brief Publish message to configured topic
 * @param message Message to publish
 * @return true if publish successful
 */
bool MQTTManager::publish(const char* message) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish - not connected to broker");
        return false;
    }
    
    // Use test publish topic from configuration
    return testPublish(message);
}

/**
 * @brief Publish message to configured topic
 * @param message Message to publish as String
 * @return true if publish successful
 */
bool MQTTManager::publish(const String& message) {
    return publish(message.c_str());
}

/**
 * @brief Check if connected to MQTT broker
 * @return true if connected
 * @note Cannot be const due to PubSubClient library limitations
 */
bool MQTTManager::connected() {
    return mqttClient.connected();
}

/**
 * @brief Force reconnection to MQTT broker
 */
void MQTTManager::reconnect() {
    Serial.println("[MQTTManager] Force reconnection requested");
    
    if (mqttClient.connected()) {
        Serial.println("[MQTTManager] Disconnecting current connection...");
        mqttClient.disconnect();
        delay(100);
    }
    
    Serial.println("[MQTTManager] Attempting new connection...");
    attemptConnection();
}

/**
 * @brief Get current MQTT client state
 * @return MQTT client state code
 * @note Cannot be const due to PubSubClient library limitations
 */
int MQTTManager::getState() {
    return mqttClient.state();
}

/**
 * @brief Load configuration from JSON file on SD card
 * @return true if configuration loaded successfully
 */
bool MQTTManager::loadConfig() {
    Serial.println("[MQTTManager] Loading configuration from SD card...");
    
    // Check if SD card is available
    if (!SD.begin()) {
        Serial.println("[MQTTManager] SD card not available");
        return false;
    }
    
    // Check if config file exists
    const char* configPath = "/config/mqtt.json";
    if (!SD.exists(configPath)) {
        Serial.println("[MQTTManager] Config file not found: /config/mqtt.json");
        return false;
    }
    
    // Open config file
    File configFile = SD.open(configPath, FILE_READ);
    if (!configFile) {
        Serial.println("[MQTTManager] Failed to open config file");
        return false;
    }
    
    // Read file content
    String jsonContent = "";
    while (configFile.available()) {
        jsonContent += configFile.readString();
    }
    configFile.close();
    
    Serial.print("[MQTTManager] Config file size: ");
    Serial.print(jsonContent.length());
    Serial.println(" bytes");
    
    // Parse JSON
    return setConfigJson(jsonContent);
}

/**
 * @brief Save configuration to JSON file on SD card
 * @return true if configuration saved successfully
 */
bool MQTTManager::saveConfig() {
    Serial.println("[MQTTManager] Saving configuration to SD card...");
    
    // Check if SD card is available
    if (!SD.begin()) {
        Serial.println("[MQTTManager] SD card not available");
        return false;
    }
    
    // Create config directory if it doesn't exist
    if (!SD.exists("/config")) {
        Serial.println("[MQTTManager] Creating /config directory");
        if (!SD.mkdir("/config")) {
            Serial.println("[MQTTManager] Failed to create config directory");
            return false;
        }
    }
    
    // Get JSON string
    String jsonContent = getConfigJson();
    
    // Open file for writing
    const char* configPath = "/config/mqtt.json";
    File configFile = SD.open(configPath, FILE_WRITE);
    if (!configFile) {
        Serial.println("[MQTTManager] Failed to open config file for writing");
        return false;
    }
    
    // Write content
    size_t written = configFile.print(jsonContent);
    configFile.close();
    
    Serial.print("[MQTTManager] Written ");
    Serial.print(written);
    Serial.println(" bytes to config file");
    
    return written == jsonContent.length();
}

/**
 * @brief Set new configuration
 * @param newConfig New configuration to apply
 * @return true if configuration applied successfully
 */
bool MQTTManager::setConfig(const MQTTConfig& newConfig) {
    Serial.println("[MQTTManager] Applying new configuration...");
    
    // Store new configuration
    config = newConfig;
    configLoaded = true;
    
    // Save to SD card
    if (!saveConfig()) {
        Serial.println("[MQTTManager] Warning: Failed to save configuration to SD card");
    }
    
    // If connected, disconnect to apply new settings
    if (mqttClient.connected()) {
        Serial.println("[MQTTManager] Disconnecting to apply new configuration...");
        mqttClient.disconnect();
        delay(100);
    }
    
    // Reconfigure client based on new settings
    if (config.enabled) {
        // Configure WiFi client based on TLS setting
        if (config.use_tls) {
            wifiClient.setInsecure();
            mqttClient.setClient(wifiClient);
        } else {
            mqttClient.setClient(wifiClientInsecure);
        }
        
        // Set server
        mqttClient.setServer(config.broker_host.c_str(), config.broker_port);
        
        // Attempt reconnection
        Serial.println("[MQTTManager] Attempting to connect with new configuration...");
        return attemptConnection();
    }
    
    return true;
}

/**
 * @brief Build topic string based on configuration
 * @param topicType Type of topic (telemetry, command, alarm, etc.)
 * @param subtopic Specific subtopic
 * @return Complete topic string
 */
String MQTTManager::buildTopic(const String& topicType, const String& subtopic) {
    String topic = "";
    
    // Add level 1 if not skipped
    if (config.topic_level1_type != "skip" && !config.topic_level1_value.isEmpty()) {
        topic += config.topic_level1_value + "/";
    }
    
    // Add level 2 if not skipped
    if (config.topic_level2_type != "skip" && !config.topic_level2_value.isEmpty()) {
        topic += config.topic_level2_value + "/";
    }
    
    // Add level 3 if not skipped
    if (config.topic_level3_type != "skip" && !config.topic_level3_value.isEmpty()) {
        topic += config.topic_level3_value + "/";
    }
    
    // Add device name
    topic += config.device_name + "/";
    
    // Add topic type
    topic += topicType;
    
    // Add subtopic if provided
    if (!subtopic.isEmpty()) {
        topic += "/" + subtopic;
    }
    
    return topic;
}

/**
 * @brief Test publish to configured test topic
 * @param message Message to publish
 * @return true if publish successful
 */
bool MQTTManager::testPublish(const String& message) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTTManager] Cannot publish - not connected to broker");
        return false;
    }
    
    if (config.test_publish_topic.isEmpty()) {
        Serial.println("[MQTTManager] Test publish topic not configured");
        return false;
    }
    
    Serial.print("[MQTTManager] Test publishing to topic: ");
    Serial.println(config.test_publish_topic);
    Serial.print("[MQTTManager] Message: ");
    Serial.println(message);
    
    bool result = mqttClient.publish(config.test_publish_topic.c_str(), message.c_str());
    
    if (result) {
        Serial.println("[MQTTManager] Test publish successful");
    } else {
        Serial.println("[MQTTManager] Test publish failed");
    }
    
    return result;
}

/**
 * @brief Get configuration as JSON string
 * @return JSON string representation of configuration
 */
String MQTTManager::getConfigJson() {
    // Use JsonDocument to avoid stack overflow
    JsonDocument doc;
    
    // Basic settings
    doc["enabled"] = config.enabled;
    
    // Broker settings
    doc["broker_host"] = config.broker_host;
    doc["broker_port"] = config.broker_port;
    doc["use_tls"] = config.use_tls;
    
    // Credentials
    doc["username"] = config.username;
    doc["password"] = config.password;
    doc["client_id"] = config.client_id;
    doc["device_name"] = config.device_name;
    
    // Topic configuration
    JsonObject topics = doc["topics"].to<JsonObject>();
    topics["level1_type"] = config.topic_level1_type;
    topics["level1_value"] = config.topic_level1_value;
    topics["level2_type"] = config.topic_level2_type;
    topics["level2_value"] = config.topic_level2_value;
    topics["level3_type"] = config.topic_level3_type;
    topics["level3_value"] = config.topic_level3_value;
    
    // Publishing settings
    JsonObject publishing = doc["publishing"].to<JsonObject>();
    publishing["retain_telemetry"] = config.retain_telemetry;
    publishing["retain_alarms"] = config.retain_alarms;
    publishing["retain_state"] = config.retain_state;
    
    // QoS levels
    JsonObject qos = doc["qos"].to<JsonObject>();
    qos["telemetry"] = config.qos_telemetry;
    qos["alarms"] = config.qos_alarms;
    qos["commands"] = config.qos_commands;
    
    // LWT settings
    JsonObject lwt = doc["lwt"].to<JsonObject>();
    lwt["enabled"] = config.lwt_enabled;
    lwt["topic"] = config.lwt_topic;
    lwt["message"] = config.lwt_message;
    lwt["qos"] = config.lwt_qos;
    lwt["retain"] = config.lwt_retain;
    
    // Test topics
    JsonObject test = doc["test"].to<JsonObject>();
    test["publish_topic"] = config.test_publish_topic;
    test["subscribe_topic"] = config.test_subscribe_topic;
    
    // Serialize to string
    String output;
    output.reserve(1024); // Pre-allocate to avoid reallocation
    serializeJsonPretty(doc, output);
    return output;
}

/**
 * @brief Set configuration from JSON string
 * @param jsonStr JSON string containing configuration
 * @return true if configuration parsed and applied successfully
 */
bool MQTTManager::setConfigJson(const String& jsonStr) {
    Serial.println("[MQTTManager] Parsing JSON configuration...");
    
    // Use JsonDocument to avoid stack overflow
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.print("[MQTTManager] JSON parse error: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Parse basic settings
    config.enabled = doc["enabled"] | false;
    
    // Parse broker settings
    config.broker_host = doc["broker_host"] | "";
    config.broker_port = doc["broker_port"] | 1883;
    config.use_tls = doc["use_tls"] | false;
    
    // Parse credentials
    config.username = doc["username"] | "";
    config.password = doc["password"] | "";
    config.client_id = doc["client_id"] | "esp32_temp_controller";
    config.device_name = doc["device_name"] | "esp32_temp_controller";
    
    // Parse topic configuration
    JsonObject topics = doc["topics"];
    if (!topics.isNull()) {
        config.topic_level1_type = topics["level1_type"] | "skip";
        config.topic_level1_value = topics["level1_value"] | "";
        config.topic_level2_type = topics["level2_type"] | "skip";
        config.topic_level2_value = topics["level2_value"] | "";
        config.topic_level3_type = topics["level3_type"] | "skip";
        config.topic_level3_value = topics["level3_value"] | "";
    }
    
    // Parse publishing settings
    JsonObject publishing = doc["publishing"];
    if (!publishing.isNull()) {
        config.retain_telemetry = publishing["retain_telemetry"] | false;
        config.retain_alarms = publishing["retain_alarms"] | true;
        config.retain_state = publishing["retain_state"] | true;
    }
    
    // Parse QoS levels
    JsonObject qos = doc["qos"];
    if (!qos.isNull()) {
        config.qos_telemetry = qos["telemetry"] | 0;
        config.qos_alarms = qos["alarms"] | 1;
        config.qos_commands = qos["commands"] | 2;
    }
    
    // Parse LWT settings
    JsonObject lwt = doc["lwt"];
    if (!lwt.isNull()) {
        config.lwt_enabled = lwt["enabled"] | true;
        config.lwt_topic = lwt["topic"] | "";
        config.lwt_message = lwt["message"] | "";
        config.lwt_qos = lwt["qos"] | 1;
        config.lwt_retain = lwt["retain"] | true;
    }
    
    // Parse test topics
    JsonObject test = doc["test"];
    if (!test.isNull()) {
        config.test_publish_topic = test["publish_topic"] | "test/pub";
        config.test_subscribe_topic = test["subscribe_topic"] | "test/sub";
    }
    
    // Generate LWT topic if not set
    if (config.lwt_enabled && config.lwt_topic.isEmpty()) {
        config.lwt_topic = buildTopic("state", "connection");
    }
    
    // Generate LWT message if not set
    if (config.lwt_enabled && config.lwt_message.isEmpty()) {
        config.lwt_message = "{\"status\":\"offline\",\"timestamp\":\"" + String(millis()) + "\"}";
    }
    
    configLoaded = true;
    Serial.println("[MQTTManager] Configuration parsed successfully");
    return true;
}