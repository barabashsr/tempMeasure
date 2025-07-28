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
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <FS.h>

/**
 * @struct MQTTConfig
 * @brief MQTT configuration structure for JSON storage
 */
struct MQTTConfig {
    // Basic settings
    bool enabled = false;
    
    // Broker settings
    String broker_host = "";
    uint16_t broker_port = 1883;
    bool use_tls = false;
    
    // Credentials
    String username = "";
    String password = "";
    String client_id = "";
    String device_name = "";
    
    // Topic configuration (ISA-95 hierarchy)
    String topic_level1_type = "skip";  // "custom", "plant", "enterprise", "skip"
    String topic_level1_value = "";
    String topic_level2_type = "skip";  // "custom", "area", "department", "skip"
    String topic_level2_value = "";
    String topic_level3_type = "skip";  // "custom", "line", "cell", "skip"
    String topic_level3_value = "";
    
    // Publishing settings
    bool retain_telemetry = false;
    bool retain_alarms = true;
    bool retain_state = true;
    
    // QoS levels
    uint8_t qos_telemetry = 0;
    uint8_t qos_alarms = 1;
    uint8_t qos_commands = 2;
    
    // LWT settings
    bool lwt_enabled = true;
    String lwt_topic = "";
    String lwt_message = "";
    uint8_t lwt_qos = 1;
    bool lwt_retain = true;
    
    // Test topics for web interface
    String test_publish_topic = "test/pub";
    String test_subscribe_topic = "test/sub";
};

/**
 * @class MQTTManager
 * @brief Manages MQTT connection and communication with broker
 * 
 * @details This class handles all MQTT operations including:
 * - Configurable connection to MQTT broker
 * - Auto-reconnection on connection loss
 * - Message publishing with configurable topics
 * - Message subscription with callback handling
 * - JSON configuration from SD card
 * - Verbose debug output for troubleshooting
 */
class MQTTManager {
private:
    MQTTConfig config;               ///< Configuration loaded from JSON
    bool configLoaded;               ///< Flag indicating if config is loaded
    
    WiFiClientSecure wifiClient;     ///< Secure WiFi client for SSL connection
    WiFiClient wifiClientInsecure;   ///< Insecure WiFi client for non-SSL
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
    
    /**
     * @brief Load configuration from JSON file on SD card
     * @return true if configuration loaded successfully
     */
    bool loadConfig();
    
    /**
     * @brief Save configuration to JSON file on SD card
     * @return true if configuration saved successfully
     */
    bool saveConfig();
    
    /**
     * @brief Get current configuration
     * @return Reference to current configuration
     */
    const MQTTConfig& getConfig() const { return config; }
    
    /**
     * @brief Set new configuration
     * @param newConfig New configuration to apply
     * @return true if configuration applied successfully
     */
    bool setConfig(const MQTTConfig& newConfig);
    
    /**
     * @brief Check if MQTT is enabled
     * @return true if MQTT is enabled in configuration
     */
    bool isEnabled() const { return configLoaded && config.enabled; }
    
    /**
     * @brief Build topic string based on configuration
     * @param topicType Type of topic (telemetry, command, alarm, etc.)
     * @param subtopic Specific subtopic
     * @return Complete topic string
     */
    String buildTopic(const String& topicType, const String& subtopic = "") const;
    
    /**
     * @brief Test publish to configured test topic
     * @param message Message to publish
     * @return true if publish successful
     */
    bool testPublish(const String& message);
    
    /**
     * @brief Get configuration as JSON string
     * @return JSON string representation of configuration
     */
    String getConfigJson() const;
    
    /**
     * @brief Set configuration from JSON string
     * @param jsonStr JSON string containing configuration
     * @return true if configuration parsed and applied successfully
     */
    bool setConfigJson(const String& jsonStr);
};

#endif // MQTT_MANAGER_H