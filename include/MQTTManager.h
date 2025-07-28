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
 * @brief Static/Singleton MQTT manager for global access
 * 
 * @details This class handles all MQTT operations including:
 * - Configurable connection to MQTT broker
 * - Auto-reconnection on connection loss
 * - Message publishing with configurable topics
 * - Message subscription with callback handling
 * - JSON configuration from SD card
 * - Verbose debug output for troubleshooting
 * - Non-blocking connection with timeout
 */
class MQTTManager {
private:
    static MQTTConfig config;               ///< Configuration loaded from JSON
    static bool configLoaded;               ///< Flag indicating if config is loaded
    
    static WiFiClientSecure wifiClient;     ///< Secure WiFi client for SSL connection
    static WiFiClient wifiClientInsecure;   ///< Insecure WiFi client for non-SSL
    static PubSubClient mqttClient;         ///< MQTT client instance
    
    static bool isConnected;                ///< Connection status flag
    static unsigned long lastReconnectAttempt; ///< Timestamp of last reconnection attempt
    static constexpr unsigned long RECONNECT_INTERVAL = 5000; ///< Reconnection interval in milliseconds
    
    static unsigned long lastPublishTime;       ///< Timestamp of last publish
    static constexpr unsigned long PUBLISH_INTERVAL = 1000; ///< Publish interval in milliseconds (1 second)
    static unsigned long publishCounter;        ///< Counter for incrementing numbers
    
    static bool connectInProgress;          ///< Non-blocking connection in progress
    static unsigned long connectStartTime;  ///< Connection attempt start time
    static constexpr unsigned long CONNECT_TIMEOUT = 5000; ///< Connection timeout in milliseconds
    
    /**
     * @brief Static callback function for MQTT message reception
     * @param topic The topic on which message was received
     * @param payload The message payload as byte array
     * @param length The length of the payload
     */
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Static method to handle received messages
     * @param topic The topic on which message was received
     * @param payload The message payload as byte array
     * @param length The length of the payload
     */
    static void handleMessage(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Attempt to connect to MQTT broker (non-blocking)
     * @return true if connection successful, false if still connecting, throws on error
     */
    static bool attemptConnection();
    
    /**
     * @brief Check non-blocking connection progress
     * @return true if connected, false if still connecting
     */
    static bool checkConnectionProgress();
    
    /**
     * @brief Private constructor to prevent instantiation
     */
    MQTTManager() = delete;

public:
    /**
     * @brief Initialize MQTT manager and establish connection
     * @return true if initialization successful, false otherwise
     */
    static bool begin();
    
    /**
     * @brief Main loop function to handle MQTT operations
     * @details Must be called regularly to maintain connection and process messages
     */
    static void loop();
    
    /**
     * @brief Publish a message to the hardcoded publish topic
     * @param message The message to publish
     * @return true if publish successful, false otherwise
     */
    static bool publish(const char* message);
    
    /**
     * @brief Publish a message to the hardcoded publish topic
     * @param message The message to publish as String
     * @return true if publish successful, false otherwise
     */
    static bool publish(const String& message);
    
    /**
     * @brief Check if connected to MQTT broker
     * @return true if connected, false otherwise
     */
    static bool connected();
    
    /**
     * @brief Force reconnection to MQTT broker
     */
    static void reconnect();
    
    /**
     * @brief Get the current MQTT client state
     * @return MQTT client state code
     */
    static int getState();
    
    /**
     * @brief Load configuration from JSON file on SD card
     * @return true if configuration loaded successfully
     */
    static bool loadConfig();
    
    /**
     * @brief Save configuration to JSON file on SD card
     * @return true if configuration saved successfully
     */
    static bool saveConfig();
    
    /**
     * @brief Get current configuration
     * @return Reference to current configuration
     */
    static const MQTTConfig& getConfig() { return config; }
    
    /**
     * @brief Set new configuration
     * @param newConfig New configuration to apply
     * @return true if configuration applied successfully
     */
    static bool setConfig(const MQTTConfig& newConfig);
    
    /**
     * @brief Check if MQTT is enabled
     * @return true if MQTT is enabled in configuration
     */
    static bool isEnabled() { return configLoaded && config.enabled; }
    
    /**
     * @brief Build topic string based on configuration
     * @param topicType Type of topic (telemetry, command, alarm, etc.)
     * @param subtopic Specific subtopic
     * @return Complete topic string
     */
    static String buildTopic(const String& topicType, const String& subtopic = "");
    
    /**
     * @brief Test publish to configured test topic
     * @param message Message to publish
     * @return true if publish successful
     */
    static bool testPublish(const String& message);
    
    /**
     * @brief Get configuration as JSON string
     * @return JSON string representation of configuration
     */
    static String getConfigJson();
    
    /**
     * @brief Set configuration from JSON string
     * @param jsonStr JSON string containing configuration
     * @return true if configuration parsed and applied successfully
     */
    static bool setConfigJson(const String& jsonStr);
};

#endif // MQTT_MANAGER_H