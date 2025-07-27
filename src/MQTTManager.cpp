/**
 * @file MQTTManager.cpp
 * @brief Implementation of basic MQTT Manager for temperature controller
 * @author Claude Assistant
 * @date 2025-01-27
 */

#include "MQTTManager.h"

// Initialize static member
MQTTManager* MQTTManager::instance = nullptr;

/**
 * @brief Constructor
 */
MQTTManager::MQTTManager() : mqttClient(wifiClient), isConnected(false), 
                             lastReconnectAttempt(0), lastPublishTime(0), publishCounter(0) {
    Serial.println("[MQTTManager] Constructor called");
    instance = this;
}

/**
 * @brief Destructor
 */
MQTTManager::~MQTTManager() {
    Serial.println("[MQTTManager] Destructor called");
    if (mqttClient.connected()) {
        Serial.println("[MQTTManager] Disconnecting from MQTT broker...");
        mqttClient.disconnect();
    }
    instance = nullptr;
}

/**
 * @brief Initialize MQTT manager
 * @return true if initialization successful
 */
bool MQTTManager::begin() {
    Serial.println("[MQTTManager] Initializing MQTT Manager...");
    Serial.println("[MQTTManager] Configuration:");
    Serial.print("[MQTTManager]   Server: ");
    Serial.println(MQTT_SERVER);
    Serial.print("[MQTTManager]   Port: ");
    Serial.println(MQTT_PORT);
    Serial.print("[MQTTManager]   Username: ");
    Serial.println(MQTT_USERNAME);
    Serial.print("[MQTTManager]   Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.print("[MQTTManager]   Publish Topic: ");
    Serial.println(PUBLISH_TOPIC);
    Serial.print("[MQTTManager]   Subscribe Topic: ");
    Serial.println(SUBSCRIBE_TOPIC);
    
    // Configure WiFi client for SSL (skip certificate verification like in example)
    Serial.println("[MQTTManager] Configuring WiFiClientSecure...");
    wifiClient.setInsecure();
    Serial.println("[MQTTManager] SSL verification disabled (setInsecure)");
    
    // Configure MQTT client
    Serial.println("[MQTTManager] Configuring MQTT client...");
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(messageCallback);
    Serial.println("[MQTTManager] MQTT client configured");
    
    // Attempt initial connection
    Serial.println("[MQTTManager] Attempting initial connection...");
    bool result = attemptConnection();
    
    if (result) {
        Serial.println("[MQTTManager] Initialization successful!");
    } else {
        Serial.println("[MQTTManager] Initialization failed - will retry in loop()");
    }
    
    return result;
}

/**
 * @brief Main loop to handle MQTT operations
 */
void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        if (!isConnected) {
            // First time noticing disconnection
            Serial.println("[MQTTManager] MQTT connection lost!");
            isConnected = false;
        }
        
        // Check if it's time to reconnect
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            Serial.println("[MQTTManager] Attempting reconnection...");
            lastReconnectAttempt = now;
            
            if (attemptConnection()) {
                Serial.println("[MQTTManager] Reconnection successful!");
                lastReconnectAttempt = 0;
            } else {
                Serial.print("[MQTTManager] Reconnection failed. Will retry in ");
                Serial.print(RECONNECT_INTERVAL / 1000);
                Serial.println(" seconds.");
            }
        }
    } else {
        // Maintain connection
        mqttClient.loop();
        
        // Publish incrementing counter every second
        unsigned long now = millis();
        if (now - lastPublishTime >= PUBLISH_INTERVAL) {
            lastPublishTime = now;
            
            // Create message with incrementing counter
            String message = String(publishCounter++);
            
            Serial.print("[MQTTManager] Publishing counter: ");
            Serial.println(message);
            
            if (publish(message)) {
                Serial.println("[MQTTManager] Counter published successfully");
            } else {
                Serial.println("[MQTTManager] Failed to publish counter");
            }
        }
    }
}

/**
 * @brief Attempt to connect to MQTT broker
 * @return true if connection successful
 */
bool MQTTManager::attemptConnection() {
    Serial.print("[MQTTManager] Connecting to MQTT broker... ");
    
    if (mqttClient.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("CONNECTED!");
        isConnected = true;
        
        // Subscribe to topic
        Serial.print("[MQTTManager] Subscribing to topic: ");
        Serial.print(SUBSCRIBE_TOPIC);
        
        if (mqttClient.subscribe(SUBSCRIBE_TOPIC)) {
            Serial.println(" - SUCCESS!");
        } else {
            Serial.println(" - FAILED!");
        }
        
        // Send connection announcement
        Serial.println("[MQTTManager] Sending connection announcement...");
        publish("Device connected and ready");
        
        return true;
    } else {
        Serial.print("FAILED! rc=");
        Serial.print(mqttClient.state());
        Serial.print(" - ");
        
        // Decode error codes
        switch(mqttClient.state()) {
            case -4: Serial.println("MQTT_CONNECTION_TIMEOUT"); break;
            case -3: Serial.println("MQTT_CONNECTION_LOST"); break;
            case -2: Serial.println("MQTT_CONNECT_FAILED"); break;
            case -1: Serial.println("MQTT_DISCONNECTED"); break;
            case 0:  Serial.println("MQTT_CONNECTED"); break;
            case 1:  Serial.println("MQTT_CONNECT_BAD_PROTOCOL"); break;
            case 2:  Serial.println("MQTT_CONNECT_BAD_CLIENT_ID"); break;
            case 3:  Serial.println("MQTT_CONNECT_UNAVAILABLE"); break;
            case 4:  Serial.println("MQTT_CONNECT_BAD_CREDENTIALS"); break;
            case 5:  Serial.println("MQTT_CONNECT_UNAUTHORIZED"); break;
            default: Serial.println("UNKNOWN ERROR"); break;
        }
        
        isConnected = false;
        return false;
    }
}

/**
 * @brief Static callback function for MQTT messages
 */
void MQTTManager::messageCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->handleMessage(topic, payload, length);
    }
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
    
    Serial.println("[MQTTManager] Publishing message...");
    Serial.print("[MQTTManager]   Topic: ");
    Serial.println(PUBLISH_TOPIC);
    Serial.print("[MQTTManager]   Message: ");
    Serial.println(message);
    
    bool result = mqttClient.publish(PUBLISH_TOPIC, message);
    
    if (result) {
        Serial.println("[MQTTManager]   Result: SUCCESS");
    } else {
        Serial.println("[MQTTManager]   Result: FAILED");
    }
    
    return result;
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