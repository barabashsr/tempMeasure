/**
 * @file MQTTManager.h
 * @brief MQTT Manager for temperature controller using 256dpi/MQTT library
 * @author Claude Assistant
 * @date 2025-01-28
 * 
 * @details This class provides MQTT functionality using 256dpi/MQTT library
 * which has better ESP32 support and handles TLS connections properly.
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
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
    String topic_level1_type = "skip";
    String topic_level1_value = "";
    String topic_level2_type = "skip";
    String topic_level2_value = "";
    String topic_level3_type = "skip";
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
    
    // Test topics
    String test_publish_topic = "test/pub";
    String test_subscribe_topic = "test/sub";
    
    // Publishing intervals
    uint16_t telemetry_interval = 60;
};

/**
 * @class MQTTManager
 * @brief MQTT manager using 256dpi/MQTT library
 */
class MQTTManager {
private:
    static MQTTConfig config;
    static bool configLoaded;
    
    static WiFiClient wifiClient;
    static WiFiClientSecure wifiClientSecure;
    static MQTTClient mqttClient;
    
    static bool isConnected;
    static unsigned long lastReconnectAttempt;
    static constexpr unsigned long RECONNECT_INTERVAL = 5000;
    
    static unsigned long lastTelemetryPublish;
    static unsigned long publishIntervalMs;
    static unsigned long publishCounter;
    
    static void messageReceived(String &topic, String &payload);
    static void connect();
    static void loop();
    static bool publishTemperatureData(class TemperatureController& controller);
    static bool publishSystemStatus(class TemperatureController& controller);
    
    MQTTManager() = delete;

public:
    static bool begin();
    static void update(class TemperatureController& controller);  // Main update method
    
    static bool publish(const char* topic, const char* payload, bool retain = false, int qos = 0);
    static bool publish(const String& topic, const String& payload, bool retain = false, int qos = 0);
    
    static bool connected() { return mqttClient.connected(); }
    static bool isEnabled() { return configLoaded && config.enabled; }
    static void reconnect() { connect(); }  // Public wrapper for reconnection
    
    static bool loadConfig();
    static bool saveConfig();
    static const MQTTConfig& getConfig() { return config; }
    static bool setConfig(const MQTTConfig& newConfig);
    
    static String buildTopic(const String& topicType, const String& subtopic = "");
    static bool testPublish(const String& message);
    
    static String getConfigJson();
    static bool setConfigJson(const String& jsonStr);
    
    static void setPublishInterval(unsigned long seconds) { 
        publishIntervalMs = (seconds < 10 ? 10 : seconds) * 1000; 
    }
    static unsigned long getPublishInterval() { return publishIntervalMs; }
};

#endif // MQTT_MANAGER_H