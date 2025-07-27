/**
 * @file MQTTManager.h
 * @brief Basic MQTT Manager for temperature controller with HiveMQ Cloud connection
 * @author Claude Assistant
 * @date 2025-01-27
 * 
 * @details This class provides basic MQTT functionality with hardcoded HiveMQ Cloud
 * credentials and topics. It includes connection management, auto-reconnect, 
 * and message subscription capabilities with verbose debug output.
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

/**
 * @class MQTTManager
 * @brief Manages MQTT connection and communication with HiveMQ Cloud broker
 * 
 * @details This class handles all MQTT operations including:
 * - Secure connection to HiveMQ Cloud broker
 * - Auto-reconnection on connection loss
 * - Message publishing to hardcoded topic
 * - Message subscription with callback handling
 * - Verbose debug output for troubleshooting
 */
class MQTTManager {
private:
    // HiveMQ Cloud connection details (hardcoded as requested)
    static constexpr const char* MQTT_SERVER = "987bfd99193b4a21a18a665a3812cc90.s1.eu.hivemq.cloud";
    static constexpr int MQTT_PORT = 8883;  // SSL port for HiveMQ Cloud
    static constexpr const char* MQTT_USERNAME = "ESP32-TempCont";
    static constexpr const char* MQTT_PASSWORD = "PolzaOil2019";
    
    // MQTT Topics (hardcoded as requested)
    static constexpr const char* PUBLISH_TOPIC = "temp/pub";
    static constexpr const char* SUBSCRIBE_TOPIC = "temp/sub";
    
    // Device ID
    static constexpr const char* DEVICE_ID = "esp32_temp_controller";
    
    WiFiClientSecure wifiClient;     ///< Secure WiFi client for SSL connection
    PubSubClient mqttClient;         ///< MQTT client instance
    
    bool isConnected;                ///< Connection status flag
    unsigned long lastReconnectAttempt; ///< Timestamp of last reconnection attempt
    static constexpr unsigned long RECONNECT_INTERVAL = 5000; ///< Reconnection interval in milliseconds
    
    unsigned long lastPublishTime;       ///< Timestamp of last publish
    static constexpr unsigned long PUBLISH_INTERVAL = 1000; ///< Publish interval in milliseconds (1 second)
    unsigned long publishCounter;        ///< Counter for incrementing numbers
    
    /**
     * @brief Static callback function for MQTT message reception
     * @param topic The topic on which message was received
     * @param payload The message payload as byte array
     * @param length The length of the payload
     */
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Static pointer to the current instance for callback routing
     */
    static MQTTManager* instance;
    
    /**
     * @brief Instance method to handle received messages
     * @param topic The topic on which message was received
     * @param payload The message payload as byte array
     * @param length The length of the payload
     */
    void handleMessage(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Attempt to connect to MQTT broker
     * @return true if connection successful, false otherwise
     */
    bool attemptConnection();

public:
    /**
     * @brief Constructor for MQTTManager
     */
    MQTTManager();
    
    /**
     * @brief Destructor for MQTTManager
     */
    ~MQTTManager();
    
    /**
     * @brief Initialize MQTT manager and establish connection
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Main loop function to handle MQTT operations
     * @details Must be called regularly to maintain connection and process messages
     */
    void loop();
    
    /**
     * @brief Publish a message to the hardcoded publish topic
     * @param message The message to publish
     * @return true if publish successful, false otherwise
     */
    bool publish(const char* message);
    
    /**
     * @brief Publish a message to the hardcoded publish topic
     * @param message The message to publish as String
     * @return true if publish successful, false otherwise
     */
    bool publish(const String& message);
    
    /**
     * @brief Check if connected to MQTT broker
     * @return true if connected, false otherwise
     * @note Cannot be const due to PubSubClient library limitations
     */
    bool connected();
    
    /**
     * @brief Force reconnection to MQTT broker
     */
    void reconnect();
    
    /**
     * @brief Get the current MQTT client state
     * @return MQTT client state code
     * @note Cannot be const due to PubSubClient library limitations
     */
    int getState();
};

#endif // MQTT_MANAGER_H