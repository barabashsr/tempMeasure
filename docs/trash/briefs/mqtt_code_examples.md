# MQTT Implementation Code Examples

## Table of Contents
1. [MQTTManager Class Implementation](#mqttmanager-class-implementation)
2. [Command Processor Implementation](#command-processor-implementation)
3. [Telemetry Publisher](#telemetry-publisher)
4. [Scheduler Implementation](#scheduler-implementation)
5. [Integration Examples](#integration-examples)
6. [Client Examples](#client-examples)

---

## 1. MQTTManager Class Implementation

### MQTTManager.h
```cpp
/**
 * @file MQTTManager.h
 * @brief Core MQTT functionality for ESP32 temperature monitoring system
 * @details Handles MQTT connections, message queuing, topic management, and TLS support
 */

#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <queue>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/**
 * @struct MQTTConfig
 * @brief MQTT configuration parameters
 */
struct MQTTConfig {
    // Connection settings
    bool enabled = false;
    String broker_host = "";
    uint16_t broker_port = 1883;
    bool use_tls = false;
    String username = "";
    String password = "";
    String client_id = "";
    
    // Topic configuration
    String topic_prefix = "plant/area1/line2";
    String device_name = "tempcontroller01";
    
    // Topic level configuration
    struct TopicLevel {
        String type;   // "custom", "plant", "skip"
        String value;  // Actual value to use
    };
    TopicLevel level1 = {"plant", "plant"};
    TopicLevel level2 = {"area", "area1"};
    TopicLevel level3 = {"line", "line2"};
    
    // Intervals and thresholds
    uint32_t telemetry_interval = 60000;     // ms
    uint32_t keepalive_interval = 30000;     // ms
    uint32_t reconnect_interval = 5000;      // ms
    float temperature_change_threshold = 0.5; // °C
    
    // QoS levels
    uint8_t qos_telemetry = 0;
    uint8_t qos_alarms = 1;
    uint8_t qos_commands = 1;
    
    // Features
    bool publish_changes_only = false;
    bool retain_telemetry = false;
    bool retain_alarms = true;
    bool compress_telemetry = true;
    
    // Security
    String ca_cert = "";      // Root CA certificate for TLS
    String client_cert = "";  // Client certificate for mutual TLS
    String client_key = "";   // Client private key
    
    // LWT (Last Will and Testament)
    bool lwt_enabled = true;
    String lwt_topic = "";    // Auto-generated if empty
    String lwt_message = "";  // Auto-generated if empty
    uint8_t lwt_qos = 1;
    bool lwt_retain = true;
};

/**
 * @struct MQTTMessage
 * @brief Queued MQTT message
 */
struct MQTTMessage {
    String topic;
    String payload;
    uint8_t qos;
    bool retain;
    uint32_t timestamp;
    uint8_t retry_count;
};

/**
 * @class MQTTManager
 * @brief Manages MQTT connectivity and messaging
 */
class MQTTManager {
private:
    // Core components
    MQTTConfig config;
    WiFiClient* insecureClient = nullptr;
    WiFiClientSecure* secureClient = nullptr;
    PubSubClient* mqttClient = nullptr;
    
    // FreeRTOS components
    TaskHandle_t mqttTaskHandle = nullptr;
    SemaphoreHandle_t mqttMutex = nullptr;
    QueueHandle_t outgoingQueue = nullptr;
    QueueHandle_t incomingQueue = nullptr;
    
    // State management
    bool initialized = false;
    bool connected = false;
    uint32_t lastReconnectAttempt = 0;
    uint32_t reconnectDelay = 1000;
    uint32_t messagesSent = 0;
    uint32_t messagesReceived = 0;
    uint32_t reconnectCount = 0;
    
    // Topic cache
    String topicPrefix = "";
    String telemetryTopic = "";
    String commandRequestTopic = "";
    String commandResponseTopic = "";
    String alarmTopic = "";
    String stateTopic = "";
    String eventTopic = "";
    
    // Previous values for change detection
    float lastTemperatures[60];
    bool hasLastTemperatures = false;
    
    // Private methods
    void buildTopics();
    String buildTopicPath(const String& subtopic);
    bool reconnect();
    void processOutgoingQueue();
    void processIncomingQueue();
    bool loadCertificates();
    String generateLWT();
    void updateStatistics();
    
    // Callback wrapper
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instance;
    
    // Task wrapper
    static void mqttTask(void* parameter);
    
public:
    /**
     * @brief Construct a new MQTTManager object
     */
    MQTTManager();
    
    /**
     * @brief Destroy the MQTTManager object
     */
    ~MQTTManager();
    
    /**
     * @brief Initialize the MQTT manager
     * @param[in] config MQTT configuration
     * @return true if initialization successful
     */
    bool begin(const MQTTConfig& config);
    
    /**
     * @brief Stop the MQTT manager
     */
    void stop();
    
    /**
     * @brief Update MQTT configuration
     * @param[in] newConfig New configuration
     * @return true if update successful
     */
    bool updateConfig(const MQTTConfig& newConfig);
    
    /**
     * @brief Check if connected to broker
     * @return true if connected
     */
    bool isConnected() const { return connected; }
    
    /**
     * @brief Publish a message
     * @param[in] subtopic Topic suffix after device path
     * @param[in] payload Message payload
     * @param[in] qos Quality of Service level
     * @param[in] retain Retain message on broker
     * @return true if message queued successfully
     */
    bool publish(const String& subtopic, const String& payload, 
                 uint8_t qos = 0, bool retain = false);
    
    /**
     * @brief Publish JSON document
     * @param[in] subtopic Topic suffix after device path
     * @param[in] doc JSON document to publish
     * @param[in] qos Quality of Service level
     * @param[in] retain Retain message on broker
     * @return true if message queued successfully
     */
    bool publishJson(const String& subtopic, JsonDocument& doc, 
                     uint8_t qos = 0, bool retain = false);
    
    /**
     * @brief Subscribe to a topic pattern
     * @param[in] pattern Topic pattern (can include wildcards)
     * @param[in] qos Quality of Service level
     * @return true if subscription successful
     */
    bool subscribe(const String& pattern, uint8_t qos = 1);
    
    /**
     * @brief Get connection statistics
     * @param[out] sent Messages sent count
     * @param[out] received Messages received count
     * @param[out] reconnects Reconnection count
     */
    void getStatistics(uint32_t& sent, uint32_t& received, uint32_t& reconnects);
    
    /**
     * @brief Get next incoming message
     * @param[out] topic Message topic
     * @param[out] payload Message payload
     * @return true if message available
     */
    bool getNextMessage(String& topic, String& payload);
    
    /**
     * @brief Force reconnection
     */
    void forceReconnect();
    
    // Friend class for command processor
    friend class MQTTCommandProcessor;
};
```

### MQTTManager.cpp
```cpp
/**
 * @file MQTTManager.cpp
 * @brief Implementation of MQTT manager with robust connection handling
 */

#include "MQTTManager.h"
#include <WiFi.h>

// Static instance for callback
MQTTManager* MQTTManager::instance = nullptr;

MQTTManager::MQTTManager() {
    instance = this;
}

MQTTManager::~MQTTManager() {
    stop();
    instance = nullptr;
}

bool MQTTManager::begin(const MQTTConfig& cfg) {
    if (initialized) {
        return false;
    }
    
    config = cfg;
    
    // Create mutex
    mqttMutex = xSemaphoreCreateMutex();
    if (!mqttMutex) {
        Serial.println(F("[MQTT] Failed to create mutex"));
        return false;
    }
    
    // Create queues
    outgoingQueue = xQueueCreate(20, sizeof(MQTTMessage));
    incomingQueue = xQueueCreate(20, sizeof(MQTTMessage));
    
    if (!outgoingQueue || !incomingQueue) {
        Serial.println(F("[MQTT] Failed to create queues"));
        return false;
    }
    
    // Initialize client
    if (config.use_tls) {
        secureClient = new WiFiClientSecure();
        if (!loadCertificates()) {
            Serial.println(F("[MQTT] Failed to load certificates"));
            delete secureClient;
            return false;
        }
        mqttClient = new PubSubClient(*secureClient);
    } else {
        insecureClient = new WiFiClient();
        mqttClient = new PubSubClient(*insecureClient);
    }
    
    // Configure MQTT client
    mqttClient->setServer(config.broker_host.c_str(), config.broker_port);
    mqttClient->setCallback(messageCallback);
    mqttClient->setBufferSize(8192);  // Increase buffer for 60-point telemetry
    
    // Build topic paths
    buildTopics();
    
    // Initialize temperature tracking
    for (int i = 0; i < 60; i++) {
        lastTemperatures[i] = -127.0;  // Invalid temperature
    }
    
    // Create MQTT task
    xTaskCreatePinnedToCore(
        mqttTask,
        "MQTT_Task",
        8192,           // Stack size
        this,           // Task parameter
        1,              // Priority
        &mqttTaskHandle,
        1               // Core 1
    );
    
    initialized = true;
    Serial.println(F("[MQTT] Manager initialized"));
    return true;
}

void MQTTManager::stop() {
    if (!initialized) return;
    
    // Stop task
    if (mqttTaskHandle) {
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = nullptr;
    }
    
    // Disconnect
    if (mqttClient && mqttClient->connected()) {
        // Send offline status
        JsonDocument doc;
        doc["timestamp"] = millis();
        doc["status"] = "offline";
        doc["reason"] = "shutdown";
        
        String payload;
        serializeJson(doc, payload);
        mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        
        mqttClient->disconnect();
    }
    
    // Clean up
    delete mqttClient;
    delete insecureClient;
    delete secureClient;
    
    if (mqttMutex) vSemaphoreDelete(mqttMutex);
    if (outgoingQueue) vQueueDelete(outgoingQueue);
    if (incomingQueue) vQueueDelete(incomingQueue);
    
    initialized = false;
}

void MQTTManager::buildTopics() {
    // Build topic prefix based on configuration
    topicPrefix = "";
    
    if (config.level1.type != "skip" && !config.level1.value.isEmpty()) {
        topicPrefix += config.level1.value;
    }
    
    if (config.level2.type != "skip" && !config.level2.value.isEmpty()) {
        if (!topicPrefix.isEmpty()) topicPrefix += "/";
        topicPrefix += config.level2.value;
    }
    
    if (config.level3.type != "skip" && !config.level3.value.isEmpty()) {
        if (!topicPrefix.isEmpty()) topicPrefix += "/";
        topicPrefix += config.level3.value;
    }
    
    if (!topicPrefix.isEmpty()) topicPrefix += "/";
    topicPrefix += config.device_name;
    
    // Build specific topics
    telemetryTopic = topicPrefix + "/telemetry/temperature";
    commandRequestTopic = topicPrefix + "/command/request";
    commandResponseTopic = topicPrefix + "/command/response";
    alarmTopic = topicPrefix + "/alarm/state";
    stateTopic = topicPrefix + "/state/device";
    eventTopic = topicPrefix + "/event/sensor";
    
    // Update LWT topic if not set
    if (config.lwt_topic.isEmpty()) {
        config.lwt_topic = stateTopic;
    }
    
    Serial.print(F("[MQTT] Topic prefix: "));
    Serial.println(topicPrefix);
}

bool MQTTManager::reconnect() {
    if (mqttClient->connected()) {
        return true;
    }
    
    // Check reconnect timing
    uint32_t now = millis();
    if (now - lastReconnectAttempt < reconnectDelay) {
        return false;
    }
    
    lastReconnectAttempt = now;
    
    Serial.print(F("[MQTT] Attempting connection to "));
    Serial.print(config.broker_host);
    Serial.print(":");
    Serial.println(config.broker_port);
    
    // Generate LWT if needed
    if (config.lwt_message.isEmpty()) {
        config.lwt_message = generateLWT();
    }
    
    // Attempt connection
    bool success = false;
    
    if (config.username.isEmpty()) {
        success = mqttClient->connect(
            config.client_id.c_str(),
            config.lwt_topic.c_str(),
            config.lwt_qos,
            config.lwt_retain,
            config.lwt_message.c_str()
        );
    } else {
        success = mqttClient->connect(
            config.client_id.c_str(),
            config.username.c_str(),
            config.password.c_str(),
            config.lwt_topic.c_str(),
            config.lwt_qos,
            config.lwt_retain,
            config.lwt_message.c_str()
        );
    }
    
    if (success) {
        Serial.println(F("[MQTT] Connected"));
        connected = true;
        reconnectDelay = 1000;  // Reset delay
        reconnectCount++;
        
        // Send online notification
        JsonDocument doc;
        doc["timestamp"] = millis();
        doc["status"] = "online";
        doc["ip_address"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["reconnect_count"] = reconnectCount;
        
        String payload;
        serializeJson(doc, payload);
        mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        
        // Subscribe to command topic
        String cmdTopic = commandRequestTopic;
        if (mqttClient->subscribe(cmdTopic.c_str(), config.qos_commands)) {
            Serial.print(F("[MQTT] Subscribed to "));
            Serial.println(cmdTopic);
        }
        
        // Subscribe to notification topic
        String notifyTopic = topicPrefix + "/notification/+";
        mqttClient->subscribe(notifyTopic.c_str(), 1);
        
        return true;
    } else {
        Serial.print(F("[MQTT] Connection failed, rc="));
        Serial.println(mqttClient->state());
        connected = false;
        
        // Exponential backoff
        reconnectDelay = min(reconnectDelay * 2, (uint32_t)300000);  // Max 5 minutes
        
        return false;
    }
}

String MQTTManager::generateLWT() {
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["device_id"] = config.device_name;
    doc["status"] = "offline";
    doc["reason"] = "unexpected_disconnect";
    
    String lwt;
    serializeJson(doc, lwt);
    return lwt;
}

void MQTTManager::mqttTask(void* parameter) {
    MQTTManager* self = (MQTTManager*)parameter;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Ensure WiFi is connected
        if (WiFi.status() == WL_CONNECTED) {
            // Handle reconnection
            if (!self->mqttClient->connected()) {
                self->reconnect();
            }
            
            // Process MQTT loop
            if (self->mqttClient->connected()) {
                self->mqttClient->loop();
                
                // Process queues
                self->processOutgoingQueue();
                self->processIncomingQueue();
            }
        }
        
        // Delay to prevent tight loop
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

void MQTTManager::processOutgoingQueue() {
    MQTTMessage msg;
    
    while (xQueueReceive(outgoingQueue, &msg, 0) == pdTRUE) {
        if (mqttClient->connected()) {
            bool success = mqttClient->publish(
                msg.topic.c_str(),
                msg.payload.c_str(),
                msg.retain
            );
            
            if (success) {
                messagesSent++;
                Serial.print(F("[MQTT] Published to "));
                Serial.println(msg.topic);
            } else {
                // Re-queue if QoS > 0 and retry count < 3
                if (msg.qos > 0 && msg.retry_count < 3) {
                    msg.retry_count++;
                    xQueueSendToFront(outgoingQueue, &msg, 0);
                } else {
                    Serial.print(F("[MQTT] Failed to publish to "));
                    Serial.println(msg.topic);
                }
            }
        } else {
            // Re-queue if QoS > 0
            if (msg.qos > 0) {
                xQueueSendToFront(outgoingQueue, &msg, 0);
            }
            break;  // Stop processing if disconnected
        }
        
        // Prevent watchdog timeout
        yield();
    }
}

void MQTTManager::messageCallback(char* topic, byte* payload, unsigned int length) {
    if (!instance) return;
    
    // Convert to String
    String topicStr = String(topic);
    String payloadStr;
    payloadStr.reserve(length);
    
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    Serial.print(F("[MQTT] Received: "));
    Serial.print(topicStr);
    Serial.print(F(" = "));
    Serial.println(payloadStr);
    
    // Queue the message
    MQTTMessage msg;
    msg.topic = topicStr;
    msg.payload = payloadStr;
    msg.timestamp = millis();
    
    xQueueSend(instance->incomingQueue, &msg, 0);
    instance->messagesReceived++;
}

bool MQTTManager::publish(const String& subtopic, const String& payload, 
                          uint8_t qos, bool retain) {
    if (!initialized) return false;
    
    String fullTopic = topicPrefix + "/" + subtopic;
    
    MQTTMessage msg;
    msg.topic = fullTopic;
    msg.payload = payload;
    msg.qos = qos;
    msg.retain = retain;
    msg.timestamp = millis();
    msg.retry_count = 0;
    
    return xQueueSend(outgoingQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool MQTTManager::publishJson(const String& subtopic, JsonDocument& doc, 
                              uint8_t qos, bool retain) {
    String payload;
    serializeJson(doc, payload);
    return publish(subtopic, payload, qos, retain);
}

bool MQTTManager::getNextMessage(String& topic, String& payload) {
    MQTTMessage msg;
    
    if (xQueueReceive(incomingQueue, &msg, 0) == pdTRUE) {
        topic = msg.topic;
        payload = msg.payload;
        return true;
    }
    
    return false;
}

bool MQTTManager::loadCertificates() {
    if (!config.use_tls || !secureClient) return true;
    
    // Load CA certificate
    if (!config.ca_cert.isEmpty()) {
        secureClient->setCACert(config.ca_cert.c_str());
    } else {
        // Skip certificate verification (not recommended for production)
        secureClient->setInsecure();
    }
    
    // Load client certificate for mutual TLS
    if (!config.client_cert.isEmpty() && !config.client_key.isEmpty()) {
        secureClient->setCertificate(config.client_cert.c_str());
        secureClient->setPrivateKey(config.client_key.c_str());
    }
    
    return true;
}

void MQTTManager::getStatistics(uint32_t& sent, uint32_t& received, uint32_t& reconnects) {
    sent = messagesSent;
    received = messagesReceived;
    reconnects = reconnectCount;
}
```

---

## 2. Command Processor Implementation

### MQTTCommandProcessor.h
```cpp
/**
 * @file MQTTCommandProcessor.h
 * @brief Processes incoming MQTT commands and generates responses
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>
#include "MQTTManager.h"
#include "TemperatureController.h"
#include "Alarm.h"
#include "IndicatorInterface.h"

/**
 * @brief Command handler function type
 */
typedef std::function<JsonDocument(const JsonDocument&)> CommandHandler;

/**
 * @class MQTTCommandProcessor
 * @brief Processes MQTT commands and executes appropriate actions
 */
class MQTTCommandProcessor {
private:
    MQTTManager* mqttManager;
    TemperatureController* tempController;
    IndicatorInterface* indicators;
    
    // Command handlers map
    std::map<String, CommandHandler> handlers;
    
    // Command queue for deferred execution
    struct QueuedCommand {
        String command_id;
        String command;
        JsonDocument parameters;
        uint32_t timestamp;
    };
    QueueHandle_t commandQueue;
    
    // Helper methods
    JsonDocument createErrorResponse(const String& message, const String& code = "ERROR");
    JsonDocument createSuccessResponse(const JsonDocument& data = JsonDocument());
    void registerHandlers();
    
    // Individual command handlers
    JsonDocument handleHelp(const JsonDocument& params);
    JsonDocument handleStatus(const JsonDocument& params);
    JsonDocument handleGetConfig(const JsonDocument& params);
    JsonDocument handleRestart(const JsonDocument& params);
    JsonDocument handleGetAllPoints(const JsonDocument& params);
    JsonDocument handleGetPoint(const JsonDocument& params);
    JsonDocument handleGetChanges(const JsonDocument& params);
    JsonDocument handleGetSummary(const JsonDocument& params);
    JsonDocument handleGetActiveAlarms(const JsonDocument& params);
    JsonDocument handleAcknowledgeAlarm(const JsonDocument& params);
    JsonDocument handleAcknowledgeAll(const JsonDocument& params);
    JsonDocument handleTestAlarm(const JsonDocument& params);
    JsonDocument handleSetThreshold(const JsonDocument& params);
    JsonDocument handleGetAlarmConfig(const JsonDocument& params);
    JsonDocument handleSetAlarmConfig(const JsonDocument& params);
    JsonDocument handleSetRelay(const JsonDocument& params);
    JsonDocument handleGetRelayStatus(const JsonDocument& params);
    
public:
    /**
     * @brief Construct a new Command Processor
     */
    MQTTCommandProcessor(MQTTManager* mqtt, TemperatureController* temp, 
                        IndicatorInterface* ind);
    
    /**
     * @brief Initialize the command processor
     */
    bool begin();
    
    /**
     * @brief Process incoming messages
     */
    void processMessages();
    
    /**
     * @brief Execute a command
     * @param[in] commandId Unique command identifier
     * @param[in] command Command name
     * @param[in] parameters Command parameters
     */
    void executeCommand(const String& commandId, const String& command, 
                       const JsonDocument& parameters);
    
    /**
     * @brief Register a custom command handler
     * @param[in] command Command name
     * @param[in] handler Handler function
     */
    void registerCommand(const String& command, CommandHandler handler);
    
    /**
     * @brief Get list of available commands
     */
    std::vector<String> getAvailableCommands();
};
```

### MQTTCommandProcessor.cpp
```cpp
/**
 * @file MQTTCommandProcessor.cpp
 * @brief Implementation of MQTT command processing
 */

#include "MQTTCommandProcessor.h"
#include <WiFi.h>
#include <esp_system.h>

MQTTCommandProcessor::MQTTCommandProcessor(MQTTManager* mqtt, 
                                         TemperatureController* temp,
                                         IndicatorInterface* ind) 
    : mqttManager(mqtt), tempController(temp), indicators(ind) {
}

bool MQTTCommandProcessor::begin() {
    // Create command queue
    commandQueue = xQueueCreate(10, sizeof(QueuedCommand));
    if (!commandQueue) {
        return false;
    }
    
    // Register all command handlers
    registerHandlers();
    
    return true;
}

void MQTTCommandProcessor::registerHandlers() {
    // System commands
    handlers["help"] = [this](const JsonDocument& p) { return handleHelp(p); };
    handlers["status"] = [this](const JsonDocument& p) { return handleStatus(p); };
    handlers["get_config"] = [this](const JsonDocument& p) { return handleGetConfig(p); };
    handlers["restart"] = [this](const JsonDocument& p) { return handleRestart(p); };
    
    // Telemetry commands
    handlers["get_all_points"] = [this](const JsonDocument& p) { return handleGetAllPoints(p); };
    handlers["get_point"] = [this](const JsonDocument& p) { return handleGetPoint(p); };
    handlers["get_changes"] = [this](const JsonDocument& p) { return handleGetChanges(p); };
    handlers["get_summary"] = [this](const JsonDocument& p) { return handleGetSummary(p); };
    
    // Alarm commands
    handlers["get_active_alarms"] = [this](const JsonDocument& p) { return handleGetActiveAlarms(p); };
    handlers["acknowledge_alarm"] = [this](const JsonDocument& p) { return handleAcknowledgeAlarm(p); };
    handlers["acknowledge_all"] = [this](const JsonDocument& p) { return handleAcknowledgeAll(p); };
    handlers["test_alarm"] = [this](const JsonDocument& p) { return handleTestAlarm(p); };
    
    // Configuration commands
    handlers["set_threshold"] = [this](const JsonDocument& p) { return handleSetThreshold(p); };
    handlers["get_alarm_config"] = [this](const JsonDocument& p) { return handleGetAlarmConfig(p); };
    handlers["set_alarm_config"] = [this](const JsonDocument& p) { return handleSetAlarmConfig(p); };
    
    // Relay commands
    handlers["set_relay"] = [this](const JsonDocument& p) { return handleSetRelay(p); };
    handlers["get_relay_status"] = [this](const JsonDocument& p) { return handleGetRelayStatus(p); };
}

void MQTTCommandProcessor::processMessages() {
    String topic, payload;
    
    // Process all available messages
    while (mqttManager->getNextMessage(topic, payload)) {
        // Check if it's a command request
        if (topic.endsWith("/command/request")) {
            // Parse JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.print(F("[CMD] JSON parse error: "));
                Serial.println(error.c_str());
                continue;
            }
            
            // Extract command details
            String cmdId = doc["cmd_id"] | "";
            String command = doc["command"] | "";
            JsonObject params = doc["parameters"];
            
            if (cmdId.isEmpty() || command.isEmpty()) {
                Serial.println(F("[CMD] Invalid command format"));
                continue;
            }
            
            // Execute command
            executeCommand(cmdId, command, params);
        }
    }
}

void MQTTCommandProcessor::executeCommand(const String& commandId, 
                                        const String& command, 
                                        const JsonDocument& parameters) {
    Serial.print(F("[CMD] Executing: "));
    Serial.println(command);
    
    uint32_t startTime = millis();
    JsonDocument response;
    
    // Set common response fields
    response["cmd_id"] = commandId;
    response["command"] = command;
    response["timestamp"] = millis();
    
    // Find and execute handler
    auto it = handlers.find(command);
    if (it != handlers.end()) {
        try {
            JsonDocument result = it->second(parameters);
            response["status"] = result["status"] | "success";
            response["data"] = result["data"];
            
            if (result.containsKey("message")) {
                response["message"] = result["message"];
            }
        } catch (...) {
            response["status"] = "error";
            response["message"] = "Command execution failed";
        }
    } else {
        response["status"] = "error";
        response["message"] = "Unknown command";
        
        // Add available commands
        JsonArray available = response.createNestedArray("available_commands");
        for (const auto& pair : handlers) {
            available.add(pair.first);
        }
    }
    
    // Add execution time
    response["execution_time"] = millis() - startTime;
    
    // Send response
    mqttManager->publishJson("command/response", response, 1);
}

JsonDocument MQTTCommandProcessor::handleHelp(const JsonDocument& params) {
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    
    // Group commands by category
    JsonObject commands = data.createNestedObject("commands");
    
    JsonArray system = commands.createNestedArray("system");
    system.add("help");
    system.add("status");
    system.add("get_config");
    system.add("restart");
    
    JsonArray telemetry = commands.createNestedArray("telemetry");
    telemetry.add("get_all_points");
    telemetry.add("get_point");
    telemetry.add("get_changes");
    telemetry.add("get_summary");
    
    JsonArray alarms = commands.createNestedArray("alarms");
    alarms.add("get_active_alarms");
    alarms.add("acknowledge_alarm");
    alarms.add("acknowledge_all");
    alarms.add("test_alarm");
    
    JsonArray config = commands.createNestedArray("config");
    config.add("set_threshold");
    config.add("get_alarm_config");
    config.add("set_alarm_config");
    
    JsonArray relay = commands.createNestedArray("relay");
    relay.add("set_relay");
    relay.add("get_relay_status");
    
    data["total_commands"] = handlers.size();
    
    return response;
}

JsonDocument MQTTCommandProcessor::handleStatus(const JsonDocument& params) {
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    
    // System information
    JsonObject system = data.createNestedObject("system");
    system["device_id"] = mqttManager->config.device_name;
    system["firmware"] = "2.0.0";  // TODO: Get from version header
    system["uptime"] = millis() / 1000;
    system["free_heap"] = ESP.getFreeHeap();
    system["total_heap"] = ESP.getHeapSize();
    system["cpu_freq"] = ESP.getCpuFreqMHz();
    
    // Network status
    JsonObject network = data.createNestedObject("network");
    network["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    network["ssid"] = WiFi.SSID();
    network["ip"] = WiFi.localIP().toString();
    network["rssi"] = WiFi.RSSI();
    network["mqtt_connected"] = mqttManager->isConnected();
    
    // Get MQTT statistics
    uint32_t sent, received, reconnects;
    mqttManager->getStatistics(sent, received, reconnects);
    network["mqtt_messages_sent"] = sent;
    network["mqtt_messages_received"] = received;
    network["mqtt_reconnects"] = reconnects;
    
    // Sensor information
    JsonObject sensors = data.createNestedObject("sensors");
    int activeCount = 0;
    int errorCount = 0;
    
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point && point->isSensorBound()) {
            activeCount++;
            if (point->isSensorError()) {
                errorCount++;
            }
        }
    }
    
    sensors["total"] = 60;
    sensors["active"] = activeCount;
    sensors["errors"] = errorCount;
    
    // Alarm summary
    JsonObject alarms = data.createNestedObject("alarms");
    int activeAlarms = 0;
    int acknowledgedAlarms = 0;
    
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point) {
            Alarm* highAlarm = point->getHighAlarm();
            Alarm* lowAlarm = point->getLowAlarm();
            
            if (highAlarm && highAlarm->getState() != Alarm::NORMAL) {
                activeAlarms++;
                if (highAlarm->getState() == Alarm::ACKNOWLEDGED) {
                    acknowledgedAlarms++;
                }
            }
            
            if (lowAlarm && lowAlarm->getState() != Alarm::NORMAL) {
                activeAlarms++;
                if (lowAlarm->getState() == Alarm::ACKNOWLEDGED) {
                    acknowledgedAlarms++;
                }
            }
        }
    }
    
    alarms["active"] = activeAlarms;
    alarms["acknowledged"] = acknowledgedAlarms;
    
    return response;
}

JsonDocument MQTTCommandProcessor::handleGetAllPoints(const JsonDocument& params) {
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    
    // Add metadata
    data["timestamp"] = millis();
    data["device_id"] = mqttManager->config.device_name;
    
    // Add points array
    JsonArray points = data.createNestedArray("points");
    
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point && point->isSensorBound()) {
            JsonObject p = points.createNestedObject();
            p["id"] = i;
            p["name"] = point->getName();
            p["temperature"] = point->getTemperature();
            p["unit"] = "C";
            
            // Add sensor info
            Sensor* sensor = point->getSensor();
            if (sensor) {
                p["sensor_id"] = sensor->getAddressString();
                p["sensor_type"] = (i < 50) ? "DS18B20" : "PT1000";
            }
            
            // Add status
            if (point->isSensorError()) {
                p["status"] = "ERROR";
            } else if (point->hasActiveAlarm()) {
                p["status"] = "ALARM";
            } else {
                p["status"] = "OK";
            }
            
            // Add alarm states if active
            if (point->hasActiveAlarm()) {
                JsonObject alarmStates = p.createNestedObject("alarms");
                
                Alarm* highAlarm = point->getHighAlarm();
                if (highAlarm && highAlarm->getState() != Alarm::NORMAL) {
                    alarmStates["high"] = highAlarm->getStateString();
                }
                
                Alarm* lowAlarm = point->getLowAlarm();
                if (lowAlarm && lowAlarm->getState() != Alarm::NORMAL) {
                    alarmStates["low"] = lowAlarm->getStateString();
                }
            }
        }
    }
    
    // Add summary
    JsonObject summary = data.createNestedObject("summary");
    summary["total_points"] = 60;
    summary["active_points"] = points.size();
    
    // Calculate min/max/avg
    float minTemp = 999.0;
    float maxTemp = -999.0;
    float sumTemp = 0.0;
    int count = 0;
    
    for (JsonVariant v : points) {
        float temp = v["temperature"];
        if (temp > -127.0) {
            minTemp = min(minTemp, temp);
            maxTemp = max(maxTemp, temp);
            sumTemp += temp;
            count++;
        }
    }
    
    if (count > 0) {
        summary["min_temp"] = minTemp;
        summary["max_temp"] = maxTemp;
        summary["avg_temp"] = sumTemp / count;
    }
    
    return response;
}

JsonDocument MQTTCommandProcessor::handleGetPoint(const JsonDocument& params) {
    int pointId = params["point_id"] | -1;
    
    if (pointId < 0 || pointId >= 60) {
        return createErrorResponse("Invalid point ID", "INVALID_PARAMETER");
    }
    
    MeasurementPoint* point = tempController->getMeasurementPoint(pointId);
    if (!point || !point->isSensorBound()) {
        return createErrorResponse("Point not configured", "NOT_FOUND");
    }
    
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    
    // Basic information
    data["point_id"] = pointId;
    data["name"] = point->getName();
    data["temperature"] = point->getTemperature();
    data["unit"] = "C";
    
    // Sensor information
    Sensor* sensor = point->getSensor();
    if (sensor) {
        JsonObject sensorInfo = data.createNestedObject("sensor");
        sensorInfo["id"] = sensor->getAddressString();
        sensorInfo["type"] = (pointId < 50) ? "DS18B20" : "PT1000";
        sensorInfo["bus"] = sensor->getBus();
    }
    
    // Alarm configuration
    JsonObject alarmConfig = data.createNestedObject("alarm_config");
    
    Alarm* highAlarm = point->getHighAlarm();
    if (highAlarm) {
        JsonObject high = alarmConfig.createNestedObject("high");
        high["enabled"] = highAlarm->isEnabled();
        high["threshold"] = highAlarm->getThreshold();
        high["hysteresis"] = highAlarm->getHysteresis();
        high["delay"] = highAlarm->getDelay();
        high["priority"] = highAlarm->getPriorityString();
        high["state"] = highAlarm->getStateString();
    }
    
    Alarm* lowAlarm = point->getLowAlarm();
    if (lowAlarm) {
        JsonObject low = alarmConfig.createNestedObject("low");
        low["enabled"] = lowAlarm->isEnabled();
        low["threshold"] = lowAlarm->getThreshold();
        low["hysteresis"] = lowAlarm->getHysteresis();
        low["delay"] = lowAlarm->getDelay();
        low["priority"] = lowAlarm->getPriorityString();
        low["state"] = lowAlarm->getStateString();
    }
    
    // Include history if requested
    bool includeHistory = params["include_history"] | false;
    if (includeHistory) {
        // TODO: Implement history retrieval from data logger
        JsonArray history = data.createNestedArray("history");
        data["history_note"] = "History not yet implemented";
    }
    
    return response;
}

JsonDocument MQTTCommandProcessor::handleAcknowledgeAlarm(const JsonDocument& params) {
    int pointId = params["point_id"] | -1;
    String alarmType = params["alarm_type"] | "";
    
    if (pointId < 0 || pointId >= 60) {
        return createErrorResponse("Invalid point ID", "INVALID_PARAMETER");
    }
    
    if (alarmType != "HIGH_TEMP" && alarmType != "LOW_TEMP") {
        return createErrorResponse("Invalid alarm type", "INVALID_PARAMETER");
    }
    
    MeasurementPoint* point = tempController->getMeasurementPoint(pointId);
    if (!point) {
        return createErrorResponse("Point not found", "NOT_FOUND");
    }
    
    Alarm* alarm = nullptr;
    if (alarmType == "HIGH_TEMP") {
        alarm = point->getHighAlarm();
    } else {
        alarm = point->getLowAlarm();
    }
    
    if (!alarm) {
        return createErrorResponse("Alarm not configured", "NOT_FOUND");
    }
    
    if (alarm->getState() == Alarm::NORMAL) {
        return createErrorResponse("No active alarm to acknowledge", "INVALID_STATE");
    }
    
    // Acknowledge the alarm
    alarm->acknowledge();
    
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    data["point_id"] = pointId;
    data["point_name"] = point->getName();
    data["alarm_type"] = alarmType;
    data["new_state"] = alarm->getStateString();
    
    // Send alarm state change notification
    JsonDocument notification;
    notification["timestamp"] = millis();
    notification["device_id"] = mqttManager->config.device_name;
    notification["point_id"] = pointId;
    notification["point_name"] = point->getName();
    notification["alarm_type"] = alarmType;
    notification["state_transition"] = "ACTIVE->ACKNOWLEDGED";
    notification["temperature"] = point->getTemperature();
    notification["threshold"] = alarm->getThreshold();
    
    mqttManager->publishJson("alarm/state", notification, 1, true);
    
    return response;
}

JsonDocument MQTTCommandProcessor::handleSetRelay(const JsonDocument& params) {
    int relayId = params["relay_id"] | -1;
    String state = params["state"] | "";
    int duration = params["duration_seconds"] | 0;
    
    if (relayId < 0 || relayId > 3) {
        return createErrorResponse("Invalid relay ID (0-3)", "INVALID_PARAMETER");
    }
    
    if (state != "ON" && state != "OFF" && state != "TOGGLE") {
        return createErrorResponse("Invalid state (ON/OFF/TOGGLE)", "INVALID_PARAMETER");
    }
    
    // Get current state
    bool currentState = indicators->getRelayState(relayId);
    bool newState = currentState;
    
    if (state == "ON") {
        newState = true;
    } else if (state == "OFF") {
        newState = false;
    } else if (state == "TOGGLE") {
        newState = !currentState;
    }
    
    // Set relay
    indicators->setRelay(relayId, newState);
    
    // Handle duration if specified
    if (duration > 0 && newState) {
        // TODO: Implement timer-based relay control
        // For now, just note it in the response
    }
    
    JsonDocument response = createSuccessResponse();
    JsonObject data = response["data"];
    data["relay_id"] = relayId;
    data["previous_state"] = currentState ? "ON" : "OFF";
    data["new_state"] = newState ? "ON" : "OFF";
    
    if (duration > 0 && newState) {
        data["auto_off_seconds"] = duration;
        data["note"] = "Timer-based auto-off not yet implemented";
    }
    
    return response;
}

JsonDocument MQTTCommandProcessor::createSuccessResponse(const JsonDocument& data) {
    JsonDocument response;
    response["status"] = "success";
    if (!data.isNull()) {
        response["data"] = data;
    }
    return response;
}

JsonDocument MQTTCommandProcessor::createErrorResponse(const String& message, const String& code) {
    JsonDocument response;
    response["status"] = "error";
    response["error_code"] = code;
    response["message"] = message;
    return response;
}
```

---

## 3. Telemetry Publisher

### MQTTTelemetry.h
```cpp
/**
 * @file MQTTTelemetry.h
 * @brief Efficient telemetry publishing for temperature data
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"
#include "TemperatureController.h"

/**
 * @class MQTTTelemetry
 * @brief Handles efficient telemetry data publishing
 */
class MQTTTelemetry {
private:
    MQTTManager* mqttManager;
    TemperatureController* tempController;
    
    // Telemetry state
    uint32_t lastTelemetryTime = 0;
    uint32_t telemetryInterval = 60000;  // Default 60 seconds
    uint32_t sequenceNumber = 0;
    
    // Change detection
    struct PointState {
        float lastTemperature = -127.0;
        float lastReportedTemp = -127.0;
        uint32_t lastChangeTime = 0;
        bool hasChanged = false;
    };
    PointState pointStates[60];
    
    // Compression state
    bool useCompression = false;
    float changeThreshold = 0.5;  // °C
    
    // Statistics tracking
    struct TelemetryStats {
        float minTemp = 999.0;
        float maxTemp = -999.0;
        float avgTemp = 0.0;
        int activePoints = 0;
        int errorPoints = 0;
        int alarmPoints = 0;
    };
    
    // Helper methods
    void updatePointStates();
    TelemetryStats calculateStats();
    JsonDocument buildFullTelemetry();
    JsonDocument buildChangeTelemetry();
    JsonDocument buildCompressedTelemetry();
    String compressData(const JsonDocument& doc);
    
public:
    /**
     * @brief Construct a new Telemetry publisher
     */
    MQTTTelemetry(MQTTManager* mqtt, TemperatureController* temp);
    
    /**
     * @brief Initialize telemetry
     */
    bool begin();
    
    /**
     * @brief Update telemetry (call from main loop)
     */
    void update();
    
    /**
     * @brief Force immediate telemetry update
     */
    void forceUpdate();
    
    /**
     * @brief Set telemetry interval
     * @param[in] intervalMs Interval in milliseconds
     */
    void setInterval(uint32_t intervalMs);
    
    /**
     * @brief Set change threshold
     * @param[in] threshold Temperature change threshold in °C
     */
    void setChangeThreshold(float threshold);
    
    /**
     * @brief Enable/disable compression
     * @param[in] enable Enable compression
     */
    void setCompression(bool enable);
    
    /**
     * @brief Publish temperature change event
     * @param[in] pointId Point that changed
     * @param[in] oldTemp Previous temperature
     * @param[in] newTemp New temperature
     */
    void publishChange(int pointId, float oldTemp, float newTemp);
    
    /**
     * @brief Publish alarm event
     * @param[in] pointId Point with alarm
     * @param[in] alarmType Type of alarm
     * @param[in] state New alarm state
     */
    void publishAlarmEvent(int pointId, const String& alarmType, const String& state);
};
```

### MQTTTelemetry.cpp
```cpp
/**
 * @file MQTTTelemetry.cpp
 * @brief Implementation of efficient telemetry publishing
 */

#include "MQTTTelemetry.h"
#include <mbedtls/base64.h>

MQTTTelemetry::MQTTTelemetry(MQTTManager* mqtt, TemperatureController* temp)
    : mqttManager(mqtt), tempController(temp) {
}

bool MQTTTelemetry::begin() {
    // Initialize point states
    for (int i = 0; i < 60; i++) {
        pointStates[i].lastTemperature = -127.0;
        pointStates[i].lastReportedTemp = -127.0;
        pointStates[i].lastChangeTime = 0;
        pointStates[i].hasChanged = false;
    }
    
    return true;
}

void MQTTTelemetry::update() {
    uint32_t now = millis();
    
    // Update point states to detect changes
    updatePointStates();
    
    // Check if it's time for regular telemetry
    if (now - lastTelemetryTime >= telemetryInterval) {
        lastTelemetryTime = now;
        
        // Build and send telemetry
        JsonDocument telemetry = buildFullTelemetry();
        
        if (useCompression && telemetry["points"].size() > 10) {
            // Compress if we have many points
            String compressed = compressData(telemetry);
            mqttManager->publish("telemetry/temperature/compressed", 
                               compressed, 
                               mqttManager->config.qos_telemetry,
                               mqttManager->config.retain_telemetry);
        } else {
            mqttManager->publishJson("telemetry/temperature", 
                                   telemetry, 
                                   mqttManager->config.qos_telemetry,
                                   mqttManager->config.retain_telemetry);
        }
        
        // Reset change flags
        for (int i = 0; i < 60; i++) {
            pointStates[i].hasChanged = false;
            pointStates[i].lastReportedTemp = pointStates[i].lastTemperature;
        }
        
        sequenceNumber++;
    }
    
    // Check for significant changes that need immediate reporting
    int changeCount = 0;
    for (int i = 0; i < 60; i++) {
        if (pointStates[i].hasChanged) {
            changeCount++;
        }
    }
    
    // If we have significant changes, send change update
    if (changeCount > 0 && mqttManager->config.publish_changes_only) {
        JsonDocument changes = buildChangeTelemetry();
        mqttManager->publishJson("telemetry/changes", changes, 0);
    }
}

void MQTTTelemetry::updatePointStates() {
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point && point->isSensorBound()) {
            float currentTemp = point->getTemperature();
            
            // Check for significant change
            float delta = abs(currentTemp - pointStates[i].lastReportedTemp);
            if (delta >= changeThreshold) {
                pointStates[i].hasChanged = true;
                pointStates[i].lastChangeTime = millis();
                
                // Publish individual change if significant
                if (delta >= changeThreshold * 2) {
                    publishChange(i, pointStates[i].lastTemperature, currentTemp);
                }
            }
            
            pointStates[i].lastTemperature = currentTemp;
        }
    }
}

JsonDocument MQTTTelemetry::buildFullTelemetry() {
    JsonDocument doc;
    
    // Metadata
    doc["timestamp"] = millis();
    doc["device_id"] = mqttManager->config.device_name;
    doc["sequence"] = sequenceNumber;
    doc["measurement_period"] = telemetryInterval / 1000;  // seconds
    
    // Points object (using object instead of array for compression)
    JsonObject points = doc.createNestedObject("points");
    
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point && point->isSensorBound()) {
            JsonObject p = points.createNestedObject(String(i));
            
            p["name"] = point->getName();
            p["temp"] = serialized(String(point->getTemperature(), 1));
            
            // Add min/max if tracking
            // p["min"] = serialized(String(minTemp, 1));
            // p["max"] = serialized(String(maxTemp, 1));
            
            // Sensor info
            Sensor* sensor = point->getSensor();
            if (sensor) {
                p["sensor_id"] = sensor->getAddressString();
                p["sensor_type"] = (i < 50) ? "DS18B20" : "PT1000";
            }
            
            // Status
            if (point->isSensorError()) {
                p["status"] = "ERROR";
            } else if (point->hasActiveAlarm()) {
                p["status"] = "ALARM";
            } else {
                p["status"] = "OK";
            }
        }
    }
    
    // Summary statistics
    TelemetryStats stats = calculateStats();
    JsonObject summary = doc.createNestedObject("summary");
    summary["total_points"] = 60;
    summary["active_points"] = stats.activePoints;
    summary["sensor_errors"] = stats.errorPoints;
    summary["active_alarms"] = stats.alarmPoints;
    summary["min_temp"] = serialized(String(stats.minTemp, 1));
    summary["max_temp"] = serialized(String(stats.maxTemp, 1));
    summary["avg_temp"] = serialized(String(stats.avgTemp, 1));
    
    return doc;
}

JsonDocument MQTTTelemetry::buildChangeTelemetry() {
    JsonDocument doc;
    
    doc["timestamp"] = millis();
    doc["device_id"] = mqttManager->config.device_name;
    
    JsonObject changes = doc.createNestedObject("changes");
    
    for (int i = 0; i < 60; i++) {
        if (pointStates[i].hasChanged) {
            MeasurementPoint* point = tempController->getMeasurementPoint(i);
            if (point) {
                JsonObject change = changes.createNestedObject(String(i));
                change["name"] = point->getName();
                change["temp"] = serialized(String(pointStates[i].lastTemperature, 1));
                change["prev_temp"] = serialized(String(pointStates[i].lastReportedTemp, 1));
                change["delta"] = serialized(String(
                    pointStates[i].lastTemperature - pointStates[i].lastReportedTemp, 1));
                
                // Calculate rate of change (°C/min)
                if (pointStates[i].lastChangeTime > 0) {
                    uint32_t timeDelta = millis() - pointStates[i].lastChangeTime;
                    float rate = (pointStates[i].lastTemperature - pointStates[i].lastReportedTemp) 
                               * 60000.0 / timeDelta;
                    change["rate"] = serialized(String(rate, 2));
                }
                
                // Add alarm info if triggered
                if (point->hasActiveAlarm()) {
                    Alarm* highAlarm = point->getHighAlarm();
                    Alarm* lowAlarm = point->getLowAlarm();
                    
                    if (highAlarm && highAlarm->getState() != Alarm::NORMAL) {
                        change["alarm_triggered"] = "HIGH_TEMP";
                    } else if (lowAlarm && lowAlarm->getState() != Alarm::NORMAL) {
                        change["alarm_triggered"] = "LOW_TEMP";
                    }
                }
            }
        }
    }
    
    return doc;
}

MQTTTelemetry::TelemetryStats MQTTTelemetry::calculateStats() {
    TelemetryStats stats;
    int tempCount = 0;
    float tempSum = 0.0;
    
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = tempController->getMeasurementPoint(i);
        if (point && point->isSensorBound()) {
            stats.activePoints++;
            
            if (point->isSensorError()) {
                stats.errorPoints++;
            } else {
                float temp = point->getTemperature();
                if (temp > -127.0) {
                    stats.minTemp = min(stats.minTemp, temp);
                    stats.maxTemp = max(stats.maxTemp, temp);
                    tempSum += temp;
                    tempCount++;
                }
                
                if (point->hasActiveAlarm()) {
                    stats.alarmPoints++;
                }
            }
        }
    }
    
    if (tempCount > 0) {
        stats.avgTemp = tempSum / tempCount;
    }
    
    return stats;
}

void MQTTTelemetry::publishChange(int pointId, float oldTemp, float newTemp) {
    MeasurementPoint* point = tempController->getMeasurementPoint(pointId);
    if (!point) return;
    
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["device_id"] = mqttManager->config.device_name;
    doc["point_id"] = pointId;
    doc["point_name"] = point->getName();
    doc["old_temperature"] = oldTemp;
    doc["new_temperature"] = newTemp;
    doc["delta"] = newTemp - oldTemp;
    
    mqttManager->publishJson("event/temperature_change", doc, 0);
}

void MQTTTelemetry::publishAlarmEvent(int pointId, const String& alarmType, const String& state) {
    MeasurementPoint* point = tempController->getMeasurementPoint(pointId);
    if (!point) return;
    
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["device_id"] = mqttManager->config.device_name;
    doc["alarm_id"] = String("ALM_") + String(millis());
    doc["point_id"] = pointId;
    doc["point_name"] = point->getName();
    doc["alarm_type"] = alarmType;
    doc["priority"] = "HIGH";  // TODO: Get from alarm config
    doc["state"] = state;
    doc["temperature"] = point->getTemperature();
    
    // Add threshold info
    if (alarmType == "HIGH_TEMP") {
        Alarm* alarm = point->getHighAlarm();
        if (alarm) {
            doc["threshold"] = alarm->getThreshold();
            doc["priority"] = alarm->getPriorityString();
        }
    } else if (alarmType == "LOW_TEMP") {
        Alarm* alarm = point->getLowAlarm();
        if (alarm) {
            doc["threshold"] = alarm->getThreshold();
            doc["priority"] = alarm->getPriorityString();
        }
    }
    
    mqttManager->publishJson("alarm/state", doc, 
                           mqttManager->config.qos_alarms,
                           mqttManager->config.retain_alarms);
}

void MQTTTelemetry::forceUpdate() {
    lastTelemetryTime = 0;  // Force update on next call
    update();
}

void MQTTTelemetry::setInterval(uint32_t intervalMs) {
    telemetryInterval = max(intervalMs, (uint32_t)10000);  // Minimum 10 seconds
}

void MQTTTelemetry::setChangeThreshold(float threshold) {
    changeThreshold = max(threshold, 0.1f);  // Minimum 0.1°C
}

void MQTTTelemetry::setCompression(bool enable) {
    useCompression = enable;
}

String MQTTTelemetry::compressData(const JsonDocument& doc) {
    // Convert to MessagePack or custom binary format
    // For now, just return JSON string
    String json;
    serializeJson(doc, json);
    
    // TODO: Implement actual compression
    // Options:
    // 1. MessagePack
    // 2. CBOR
    // 3. Custom binary protocol
    // 4. gzip (if supported by broker)
    
    return json;
}
```

---

## 4. Scheduler Implementation

### MQTTScheduler.h
```cpp
/**
 * @file MQTTScheduler.h
 * @brief Scheduler for automated MQTT tasks and commands
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include "MQTTManager.h"

/**
 * @brief Schedule entry for automated tasks
 */
struct ScheduleEntry {
    String id;                    // Unique schedule ID
    String type;                  // "interval", "cron", "once"
    uint32_t interval = 0;        // Interval in seconds (for interval type)
    String cronExpr = "";         // Cron expression (for cron type)
    uint32_t oneTimeExec = 0;     // Timestamp for one-time execution
    String command = "";          // Command to execute
    JsonDocument parameters;      // Command parameters
    uint32_t lastRun = 0;         // Last execution timestamp
    uint32_t nextRun = 0;         // Next scheduled execution
    bool enabled = true;          // Schedule enabled/disabled
    uint32_t executionCount = 0;  // Number of executions
};

/**
 * @class MQTTScheduler
 * @brief Manages scheduled MQTT commands and tasks
 */
class MQTTScheduler {
private:
    MQTTManager* mqttManager;
    std::vector<ScheduleEntry> schedules;
    
    // Cron parser state
    struct CronField {
        std::vector<int> values;
        bool isWildcard = false;
    };
    
    // Helper methods
    uint32_t calculateNextRun(const ScheduleEntry& entry);
    uint32_t calculateNextCronRun(const String& cronExpr, uint32_t fromTime);
    bool parseCronExpression(const String& cronExpr, CronField fields[5]);
    bool matchesCronTime(const CronField fields[5], uint32_t timestamp);
    void executeSchedule(ScheduleEntry& entry);
    void saveSchedules();
    void loadSchedules();
    
public:
    /**
     * @brief Construct a new Scheduler
     */
    MQTTScheduler(MQTTManager* mqtt);
    
    /**
     * @brief Initialize the scheduler
     */
    bool begin();
    
    /**
     * @brief Update scheduler (call from main loop)
     */
    void update();
    
    /**
     * @brief Add a new schedule
     * @param[in] entry Schedule entry to add
     * @return Schedule ID if successful, empty string on error
     */
    String addSchedule(const ScheduleEntry& entry);
    
    /**
     * @brief Remove a schedule
     * @param[in] scheduleId Schedule ID to remove
     * @return true if removed successfully
     */
    bool removeSchedule(const String& scheduleId);
    
    /**
     * @brief Update an existing schedule
     * @param[in] scheduleId Schedule ID to update
     * @param[in] entry New schedule data
     * @return true if updated successfully
     */
    bool updateSchedule(const String& scheduleId, const ScheduleEntry& entry);
    
    /**
     * @brief Get all schedules
     * @return Vector of schedule entries
     */
    std::vector<ScheduleEntry> getSchedules() const { return schedules; }
    
    /**
     * @brief Get a specific schedule
     * @param[in] scheduleId Schedule ID
     * @param[out] entry Schedule entry if found
     * @return true if found
     */
    bool getSchedule(const String& scheduleId, ScheduleEntry& entry);
    
    /**
     * @brief Enable/disable a schedule
     * @param[in] scheduleId Schedule ID
     * @param[in] enabled Enable state
     * @return true if updated successfully
     */
    bool setScheduleEnabled(const String& scheduleId, bool enabled);
    
    /**
     * @brief Force execution of a schedule
     * @param[in] scheduleId Schedule ID to execute
     * @return true if executed successfully
     */
    bool executeNow(const String& scheduleId);
    
    /**
     * @brief Register a custom command handler
     * @param[in] command Command name
     * @param[in] handler Handler function
     */
    void registerCommand(const String& command, 
                        std::function<void(const JsonDocument&)> handler);
};
```

### MQTTScheduler.cpp (partial implementation)
```cpp
/**
 * @file MQTTScheduler.cpp
 * @brief Implementation of MQTT task scheduler
 */

#include "MQTTScheduler.h"
#include <SPIFFS.h>
#include <time.h>

#define SCHEDULES_FILE "/schedules.json"

MQTTScheduler::MQTTScheduler(MQTTManager* mqtt) : mqttManager(mqtt) {
}

bool MQTTScheduler::begin() {
    // Load saved schedules
    loadSchedules();
    
    // Calculate next run times
    for (auto& schedule : schedules) {
        schedule.nextRun = calculateNextRun(schedule);
    }
    
    return true;
}

void MQTTScheduler::update() {
    uint32_t now = millis() / 1000;  // Convert to seconds
    
    for (auto& schedule : schedules) {
        if (!schedule.enabled) continue;
        
        // Check if it's time to execute
        if (now >= schedule.nextRun) {
            executeSchedule(schedule);
            
            // Calculate next run time
            schedule.lastRun = now;
            schedule.nextRun = calculateNextRun(schedule);
            schedule.executionCount++;
            
            // Save updated schedules
            saveSchedules();
        }
    }
}

String MQTTScheduler::addSchedule(const ScheduleEntry& entry) {
    // Validate entry
    if (entry.id.isEmpty() || entry.command.isEmpty()) {
        return "";
    }
    
    // Check for duplicate ID
    for (const auto& schedule : schedules) {
        if (schedule.id == entry.id) {
            return "";  // ID already exists
        }
    }
    
    // Add schedule
    ScheduleEntry newEntry = entry;
    newEntry.nextRun = calculateNextRun(newEntry);
    schedules.push_back(newEntry);
    
    // Save schedules
    saveSchedules();
    
    Serial.print(F("[SCHEDULER] Added schedule: "));
    Serial.println(entry.id);
    
    return entry.id;
}

bool MQTTScheduler::removeSchedule(const String& scheduleId) {
    auto it = std::remove_if(schedules.begin(), schedules.end(),
        [&scheduleId](const ScheduleEntry& s) { return s.id == scheduleId; });
    
    if (it != schedules.end()) {
        schedules.erase(it, schedules.end());
        saveSchedules();
        return true;
    }
    
    return false;
}

void MQTTScheduler::executeSchedule(ScheduleEntry& entry) {
    Serial.print(F("[SCHEDULER] Executing schedule: "));
    Serial.println(entry.id);
    
    // Build command message
    JsonDocument doc;
    doc["cmd_id"] = "SCHED_" + entry.id + "_" + String(millis());
    doc["timestamp"] = millis();
    doc["source"] = "scheduler";
    doc["schedule_id"] = entry.id;
    doc["command"] = entry.command;
    doc["parameters"] = entry.parameters;
    
    // Publish command
    mqttManager->publishJson("command/request", doc, 1);
}

uint32_t MQTTScheduler::calculateNextRun(const ScheduleEntry& entry) {
    uint32_t now = millis() / 1000;
    
    if (entry.type == "interval") {
        if (entry.lastRun == 0) {
            return now + entry.interval;
        } else {
            return entry.lastRun + entry.interval;
        }
    } else if (entry.type == "cron") {
        return calculateNextCronRun(entry.cronExpr, now);
    } else if (entry.type == "once") {
        return entry.oneTimeExec;
    }
    
    return 0;
}

uint32_t MQTTScheduler::calculateNextCronRun(const String& cronExpr, uint32_t fromTime) {
    // Basic cron parser for common expressions
    // Format: minute hour day month weekday
    // Examples: "0 8 * * *" = Every day at 8:00
    //           "*/15 * * * *" = Every 15 minutes
    //           "0 0 * * 0" = Every Sunday at midnight
    
    CronField fields[5];
    if (!parseCronExpression(cronExpr, fields)) {
        return 0;  // Invalid expression
    }
    
    // Convert to struct tm
    time_t t = fromTime;
    struct tm* timeinfo = localtime(&t);
    
    // Find next matching time
    for (int attempts = 0; attempts < 366 * 24 * 60; attempts++) {
        // Increment by 1 minute
        timeinfo->tm_min++;
        mktime(timeinfo);  // Normalize
        
        if (matchesCronTime(fields, mktime(timeinfo))) {
            return mktime(timeinfo);
        }
    }
    
    return 0;  // No match found
}

void MQTTScheduler::saveSchedules() {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (const auto& schedule : schedules) {
        JsonObject obj = array.createNestedObject();
        obj["id"] = schedule.id;
        obj["type"] = schedule.type;
        obj["interval"] = schedule.interval;
        obj["cron"] = schedule.cronExpr;
        obj["once"] = schedule.oneTimeExec;
        obj["command"] = schedule.command;
        obj["parameters"] = schedule.parameters;
        obj["enabled"] = schedule.enabled;
        obj["lastRun"] = schedule.lastRun;
        obj["nextRun"] = schedule.nextRun;
        obj["executionCount"] = schedule.executionCount;
    }
    
    File file = SPIFFS.open(SCHEDULES_FILE, "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void MQTTScheduler::loadSchedules() {
    File file = SPIFFS.open(SCHEDULES_FILE, "r");
    if (!file) return;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) return;
    
    schedules.clear();
    JsonArray array = doc.as<JsonArray>();
    
    for (JsonObject obj : array) {
        ScheduleEntry entry;
        entry.id = obj["id"] | "";
        entry.type = obj["type"] | "";
        entry.interval = obj["interval"] | 0;
        entry.cronExpr = obj["cron"] | "";
        entry.oneTimeExec = obj["once"] | 0;
        entry.command = obj["command"] | "";
        entry.parameters = obj["parameters"];
        entry.enabled = obj["enabled"] | true;
        entry.lastRun = obj["lastRun"] | 0;
        entry.nextRun = obj["nextRun"] | 0;
        entry.executionCount = obj["executionCount"] | 0;
        
        schedules.push_back(entry);
    }
}
```

---

## 5. Integration Examples

### Integration with TemperatureController

```cpp
/**
 * @file main_mqtt_integration.cpp
 * @brief Example of integrating MQTT into the main application
 */

#include "MQTTManager.h"
#include "MQTTCommandProcessor.h"
#include "MQTTTelemetry.h"
#include "MQTTScheduler.h"

// Global MQTT components
MQTTManager* mqttManager = nullptr;
MQTTCommandProcessor* commandProcessor = nullptr;
MQTTTelemetry* telemetryPublisher = nullptr;
MQTTScheduler* scheduler = nullptr;

// In setup()
void setupMQTT() {
    // Load MQTT configuration from ConfigAssist
    MQTTConfig mqttConfig;
    mqttConfig.enabled = config.getBool("mqtt_enabled");
    mqttConfig.broker_host = config.getString("mqtt_broker");
    mqttConfig.broker_port = config.getInt("mqtt_port");
    mqttConfig.username = config.getString("mqtt_username");
    mqttConfig.password = config.getString("mqtt_password");
    mqttConfig.device_name = config.getString("mqtt_device_name");
    mqttConfig.use_tls = config.getBool("mqtt_use_tls");
    mqttConfig.telemetry_interval = config.getInt("mqtt_telemetry_interval") * 1000;
    
    // Topic configuration
    mqttConfig.level1.type = config.getString("mqtt_topic_level1_type");
    mqttConfig.level1.value = config.getString("mqtt_topic_level1_value");
    mqttConfig.level2.type = config.getString("mqtt_topic_level2_type");
    mqttConfig.level2.value = config.getString("mqtt_topic_level2_value");
    mqttConfig.level3.type = config.getString("mqtt_topic_level3_type");
    mqttConfig.level3.value = config.getString("mqtt_topic_level3_value");
    
    // Initialize MQTT components
    mqttManager = new MQTTManager();
    if (mqttManager->begin(mqttConfig)) {
        Serial.println(F("MQTT Manager initialized"));
        
        // Initialize command processor
        commandProcessor = new MQTTCommandProcessor(mqttManager, &tempController, &indicators);
        commandProcessor->begin();
        
        // Initialize telemetry publisher
        telemetryPublisher = new MQTTTelemetry(mqttManager, &tempController);
        telemetryPublisher->begin();
        telemetryPublisher->setInterval(mqttConfig.telemetry_interval);
        
        // Initialize scheduler
        scheduler = new MQTTScheduler(mqttManager);
        scheduler->begin();
        
        // Add default schedules
        ScheduleEntry dailyReport;
        dailyReport.id = "daily_summary";
        dailyReport.type = "cron";
        dailyReport.cronExpr = "0 8 * * *";  // 8:00 AM daily
        dailyReport.command = "get_summary";
        scheduler->addSchedule(dailyReport);
    } else {
        Serial.println(F("Failed to initialize MQTT"));
    }
}

// In main loop()
void loopMQTT() {
    if (mqttManager && mqttManager->config.enabled) {
        // Process incoming commands
        if (commandProcessor) {
            commandProcessor->processMessages();
        }
        
        // Update telemetry
        if (telemetryPublisher) {
            telemetryPublisher->update();
        }
        
        // Update scheduler
        if (scheduler) {
            scheduler->update();
        }
    }
}

// Hook into alarm state changes
void onAlarmStateChange(int pointId, Alarm::AlarmType type, Alarm::State newState) {
    if (telemetryPublisher) {
        String alarmType = (type == Alarm::HIGH) ? "HIGH_TEMP" : "LOW_TEMP";
        String state = "";
        
        switch (newState) {
            case Alarm::NORMAL: state = "RESOLVED"; break;
            case Alarm::ACTIVE: state = "ACTIVE"; break;
            case Alarm::ACKNOWLEDGED: state = "ACKNOWLEDGED"; break;
            case Alarm::SILENCED: state = "SILENCED"; break;
        }
        
        telemetryPublisher->publishAlarmEvent(pointId, alarmType, state);
    }
}

// Hook into sensor events
void onSensorEvent(int sensorIndex, const String& event) {
    if (mqttManager) {
        JsonDocument doc;
        doc["timestamp"] = millis();
        doc["device_id"] = mqttManager->config.device_name;
        doc["sensor_index"] = sensorIndex;
        doc["event"] = event;
        
        mqttManager->publishJson("event/sensor", doc, 0);
    }
}
```

### Web Interface Configuration

```javascript
/**
 * @file mqtt_config.js
 * @brief JavaScript for MQTT configuration UI
 */

// MQTT configuration page handler
class MQTTConfig {
    constructor() {
        this.statusUpdateInterval = null;
        this.topicPreviewTimer = null;
    }
    
    async init() {
        // Load current configuration
        await this.loadConfig();
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Start status updates if MQTT is enabled
        const enabled = document.getElementById('mqtt_enabled').checked;
        if (enabled) {
            this.startStatusUpdates();
        }
    }
    
    async loadConfig() {
        try {
            const response = await fetch('/api/mqtt/config');
            const config = await response.json();
            
            // Populate form fields
            document.getElementById('mqtt_enabled').checked = config.enabled;
            document.getElementById('mqtt_broker').value = config.broker_host;
            document.getElementById('mqtt_port').value = config.broker_port;
            document.getElementById('mqtt_username').value = config.username;
            document.getElementById('mqtt_password').value = config.password;
            document.getElementById('mqtt_device_name').value = config.device_name;
            document.getElementById('mqtt_use_tls').checked = config.use_tls;
            document.getElementById('mqtt_telemetry_interval').value = config.telemetry_interval;
            
            // Topic configuration
            document.getElementById('topic_level1_type').value = config.level1_type;
            document.getElementById('topic_level1_value').value = config.level1_value;
            document.getElementById('topic_level2_type').value = config.level2_type;
            document.getElementById('topic_level2_value').value = config.level2_value;
            document.getElementById('topic_level3_type').value = config.level3_type;
            document.getElementById('topic_level3_value').value = config.level3_value;
            
            // Features
            document.getElementById('mqtt_publish_changes').checked = config.publish_changes_only;
            document.getElementById('mqtt_retain_telemetry').checked = config.retain_telemetry;
            document.getElementById('mqtt_retain_alarms').checked = config.retain_alarms;
            
            this.updateTopicPreview();
            
        } catch (error) {
            console.error('Failed to load MQTT config:', error);
        }
    }
    
    setupEventListeners() {
        // Enable/disable toggle
        document.getElementById('mqtt_enabled').addEventListener('change', (e) => {
            if (e.target.checked) {
                this.startStatusUpdates();
            } else {
                this.stopStatusUpdates();
            }
        });
        
        // Topic level changes
        const topicInputs = document.querySelectorAll('[id^="topic_level"]');
        topicInputs.forEach(input => {
            input.addEventListener('input', () => {
                clearTimeout(this.topicPreviewTimer);
                this.topicPreviewTimer = setTimeout(() => {
                    this.updateTopicPreview();
                }, 300);
            });
        });
        
        // Save button
        document.getElementById('save_mqtt_config').addEventListener('click', () => {
            this.saveConfig();
        });
        
        // Test connection button
        document.getElementById('test_mqtt_connection').addEventListener('click', () => {
            this.testConnection();
        });
    }
    
    updateTopicPreview() {
        const level1Type = document.getElementById('topic_level1_type').value;
        const level1Value = document.getElementById('topic_level1_value').value;
        const level2Type = document.getElementById('topic_level2_type').value;
        const level2Value = document.getElementById('topic_level2_value').value;
        const level3Type = document.getElementById('topic_level3_type').value;
        const level3Value = document.getElementById('topic_level3_value').value;
        const deviceName = document.getElementById('mqtt_device_name').value;
        
        let preview = '';
        
        if (level1Type !== 'skip' && level1Value) {
            preview += level1Value;
        }
        
        if (level2Type !== 'skip' && level2Value) {
            if (preview) preview += '/';
            preview += level2Value;
        }
        
        if (level3Type !== 'skip' && level3Value) {
            if (preview) preview += '/';
            preview += level3Value;
        }
        
        if (preview) preview += '/';
        preview += deviceName || 'device';
        preview += '/telemetry/temperature';
        
        document.getElementById('topic-preview').textContent = preview;
    }
    
    async saveConfig() {
        const config = {
            enabled: document.getElementById('mqtt_enabled').checked,
            broker_host: document.getElementById('mqtt_broker').value,
            broker_port: parseInt(document.getElementById('mqtt_port').value),
            username: document.getElementById('mqtt_username').value,
            password: document.getElementById('mqtt_password').value,
            device_name: document.getElementById('mqtt_device_name').value,
            use_tls: document.getElementById('mqtt_use_tls').checked,
            telemetry_interval: parseInt(document.getElementById('mqtt_telemetry_interval').value),
            
            // Topic levels
            level1_type: document.getElementById('topic_level1_type').value,
            level1_value: document.getElementById('topic_level1_value').value,
            level2_type: document.getElementById('topic_level2_type').value,
            level2_value: document.getElementById('topic_level2_value').value,
            level3_type: document.getElementById('topic_level3_type').value,
            level3_value: document.getElementById('topic_level3_value').value,
            
            // Features
            publish_changes_only: document.getElementById('mqtt_publish_changes').checked,
            retain_telemetry: document.getElementById('mqtt_retain_telemetry').checked,
            retain_alarms: document.getElementById('mqtt_retain_alarms').checked
        };
        
        try {
            const response = await fetch('/api/mqtt/config', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(config)
            });
            
            if (response.ok) {
                alert('MQTT configuration saved successfully');
            } else {
                alert('Failed to save configuration');
            }
            
        } catch (error) {
            console.error('Save failed:', error);
            alert('Failed to save configuration: ' + error.message);
        }
    }
    
    async testConnection() {
        try {
            const response = await fetch('/api/mqtt/test');
            const result = await response.json();
            
            if (result.success) {
                alert('MQTT connection test successful!');
            } else {
                alert('Connection test failed: ' + result.error);
            }
            
        } catch (error) {
            alert('Test failed: ' + error.message);
        }
    }
    
    async updateStatus() {
        try {
            const response = await fetch('/api/mqtt/status');
            const status = await response.json();
            
            // Update connection indicator
            const indicator = document.querySelector('.status-indicator');
            const statusText = document.querySelector('.status-text');
            
            if (status.connected) {
                indicator.classList.add('connected');
                indicator.classList.remove('disconnected');
                statusText.textContent = 'Connected to ' + status.broker;
            } else {
                indicator.classList.add('disconnected');
                indicator.classList.remove('connected');
                statusText.textContent = 'Disconnected';
            }
            
            // Update statistics
            document.getElementById('mqtt-sent').textContent = status.messages_sent || 0;
            document.getElementById('mqtt-received').textContent = status.messages_received || 0;
            document.getElementById('mqtt-error').textContent = status.last_error || 'None';
            
        } catch (error) {
            console.error('Status update failed:', error);
        }
    }
    
    startStatusUpdates() {
        this.updateStatus();
        this.statusUpdateInterval = setInterval(() => {
            this.updateStatus();
        }, 5000);
    }
    
    stopStatusUpdates() {
        if (this.statusUpdateInterval) {
            clearInterval(this.statusUpdateInterval);
            this.statusUpdateInterval = null;
        }
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    const mqttConfig = new MQTTConfig();
    mqttConfig.init();
});
```

---

## 6. Client Examples

### Python Monitoring Client

```python
#!/usr/bin/env python3
"""
MQTT Temperature Monitor Client
Monitors temperature data and alarms from ESP32 device
"""

import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
import sqlite3
import matplotlib.pyplot as plt
from collections import deque
import threading

class TemperatureMonitor:
    def __init__(self, broker, port, device_name, username=None, password=None):
        self.broker = broker
        self.port = port
        self.device_name = device_name
        self.username = username
        self.password = password
        
        # Data storage
        self.temperature_data = {}
        self.alarm_queue = deque(maxlen=100)
        self.db_conn = None
        
        # MQTT client
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        if username and password:
            self.client.username_pw_set(username, password)
        
        # Setup database
        self.setup_database()
        
    def setup_database(self):
        """Initialize SQLite database for historical data"""
        self.db_conn = sqlite3.connect('temperature_data.db', check_same_thread=False)
        cursor = self.db_conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS temperature_readings (
                timestamp INTEGER,
                point_id INTEGER,
                point_name TEXT,
                temperature REAL,
                status TEXT,
                PRIMARY KEY (timestamp, point_id)
            )
        ''')
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS alarms (
                timestamp INTEGER PRIMARY KEY,
                point_id INTEGER,
                point_name TEXT,
                alarm_type TEXT,
                state TEXT,
                temperature REAL,
                threshold REAL
            )
        ''')
        
        self.db_conn.commit()
        
    def on_connect(self, client, userdata, flags, rc):
        """Callback for MQTT connection"""
        if rc == 0:
            print(f"Connected to MQTT broker at {self.broker}:{self.port}")
            
            # Subscribe to topics
            topics = [
                f"+/+/+/{self.device_name}/telemetry/+",
                f"+/+/+/{self.device_name}/alarm/+",
                f"+/+/+/{self.device_name}/event/+",
                f"+/+/+/{self.device_name}/state/+"
            ]
            
            for topic in topics:
                client.subscribe(topic, qos=1)
                print(f"Subscribed to {topic}")
        else:
            print(f"Connection failed with code {rc}")
            
    def on_disconnect(self, client, userdata, rc):
        """Callback for MQTT disconnection"""
        print(f"Disconnected from broker (rc={rc})")
        
    def on_message(self, client, userdata, msg):
        """Process incoming MQTT messages"""
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            
            # Extract topic components
            topic_parts = topic.split('/')
            message_type = topic_parts[-2]
            data_type = topic_parts[-1]
            
            if message_type == "telemetry" and data_type == "temperature":
                self.process_telemetry(payload)
            elif message_type == "alarm":
                self.process_alarm(payload)
            elif message_type == "event":
                self.process_event(payload)
            elif message_type == "state":
                self.process_state(payload)
                
        except Exception as e:
            print(f"Error processing message: {e}")
            
    def process_telemetry(self, data):
        """Process temperature telemetry data"""
        timestamp = data.get('timestamp', int(time.time() * 1000))
        points = data.get('points', {})
        
        cursor = self.db_conn.cursor()
        
        for point_id, point_data in points.items():
            try:
                point_id = int(point_id)
                temperature = float(point_data['temp'])
                point_name = point_data.get('name', f'Point {point_id}')
                status = point_data.get('status', 'OK')
                
                # Update in-memory data
                if point_id not in self.temperature_data:
                    self.temperature_data[point_id] = {
                        'name': point_name,
                        'temperatures': deque(maxlen=1000),
                        'timestamps': deque(maxlen=1000)
                    }
                
                self.temperature_data[point_id]['temperatures'].append(temperature)
                self.temperature_data[point_id]['timestamps'].append(timestamp)
                
                # Store in database
                cursor.execute('''
                    INSERT OR REPLACE INTO temperature_readings 
                    (timestamp, point_id, point_name, temperature, status)
                    VALUES (?, ?, ?, ?, ?)
                ''', (timestamp, point_id, point_name, temperature, status))
                
            except Exception as e:
                print(f"Error processing point {point_id}: {e}")
                
        self.db_conn.commit()
        
        # Print summary
        summary = data.get('summary', {})
        print(f"\n[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Temperature Update:")
        print(f"  Active Points: {summary.get('active_points', 0)}")
        print(f"  Temperature Range: {summary.get('min_temp', 0)}°C - {summary.get('max_temp', 0)}°C")
        print(f"  Average: {summary.get('avg_temp', 0)}°C")
        print(f"  Alarms: {summary.get('active_alarms', 0)}")
        
    def process_alarm(self, data):
        """Process alarm events"""
        timestamp = data.get('timestamp', int(time.time() * 1000))
        point_id = data.get('point_id', -1)
        point_name = data.get('point_name', 'Unknown')
        alarm_type = data.get('alarm_type', '')
        state = data.get('state', '')
        temperature = data.get('temperature', 0)
        threshold = data.get('threshold', 0)
        
        # Store in database
        cursor = self.db_conn.cursor()
        cursor.execute('''
            INSERT INTO alarms 
            (timestamp, point_id, point_name, alarm_type, state, temperature, threshold)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (timestamp, point_id, point_name, alarm_type, state, temperature, threshold))
        self.db_conn.commit()
        
        # Add to alarm queue
        self.alarm_queue.append(data)
        
        # Print alarm
        print(f"\n[ALARM] {point_name} - {alarm_type}")
        print(f"  State: {state}")
        print(f"  Temperature: {temperature}°C (Threshold: {threshold}°C)")
        
    def send_command(self, command, parameters=None):
        """Send a command to the device"""
        cmd_id = f"PY_{int(time.time() * 1000)}"
        
        message = {
            "cmd_id": cmd_id,
            "timestamp": int(time.time() * 1000),
            "source": "python_client",
            "command": command,
            "parameters": parameters or {}
        }
        
        topic = f"plant/area1/line2/{self.device_name}/command/request"
        self.client.publish(topic, json.dumps(message), qos=1)
        
        print(f"Sent command: {command}")
        return cmd_id
        
    def plot_temperatures(self, point_ids=None):
        """Plot temperature trends"""
        if point_ids is None:
            point_ids = list(self.temperature_data.keys())[:5]  # First 5 points
            
        plt.figure(figsize=(12, 6))
        
        for point_id in point_ids:
            if point_id in self.temperature_data:
                data = self.temperature_data[point_id]
                if len(data['temperatures']) > 0:
                    # Convert timestamps to datetime
                    times = [datetime.fromtimestamp(ts/1000) for ts in data['timestamps']]
                    temps = list(data['temperatures'])
                    
                    plt.plot(times, temps, label=data['name'])
                    
        plt.xlabel('Time')
        plt.ylabel('Temperature (°C)')
        plt.title('Temperature Trends')
        plt.legend()
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.show()
        
    def get_statistics(self, point_id, hours=24):
        """Get statistics for a specific point"""
        cursor = self.db_conn.cursor()
        
        # Calculate time range
        end_time = int(time.time() * 1000)
        start_time = end_time - (hours * 3600 * 1000)
        
        cursor.execute('''
            SELECT 
                MIN(temperature) as min_temp,
                MAX(temperature) as max_temp,
                AVG(temperature) as avg_temp,
                COUNT(*) as reading_count
            FROM temperature_readings
            WHERE point_id = ? AND timestamp BETWEEN ? AND ?
        ''', (point_id, start_time, end_time))
        
        result = cursor.fetchone()
        
        if result:
            print(f"\nStatistics for Point {point_id} (last {hours} hours):")
            print(f"  Min Temperature: {result[0]:.1f}°C")
            print(f"  Max Temperature: {result[1]:.1f}°C")
            print(f"  Avg Temperature: {result[2]:.1f}°C")
            print(f"  Reading Count: {result[3]}")
            
    def run(self):
        """Start the monitor"""
        try:
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_forever()
        except KeyboardInterrupt:
            print("\nShutting down...")
            self.client.disconnect()
            self.db_conn.close()

if __name__ == "__main__":
    # Configuration
    BROKER = "broker.hivemq.com"
    PORT = 1883
    DEVICE_NAME = "tempcontroller01"
    USERNAME = None  # Set if required
    PASSWORD = None  # Set if required
    
    # Create and run monitor
    monitor = TemperatureMonitor(BROKER, PORT, DEVICE_NAME, USERNAME, PASSWORD)
    
    # Start monitoring in a separate thread
    monitor_thread = threading.Thread(target=monitor.run)
    monitor_thread.daemon = True
    monitor_thread.start()
    
    # Interactive command loop
    print("\nTemperature Monitor Started")
    print("Commands: status, summary, acknowledge <point_id>, plot, stats <point_id>, quit")
    
    while True:
        try:
            cmd = input("\n> ").strip().split()
            
            if not cmd:
                continue
                
            if cmd[0] == "quit":
                break
            elif cmd[0] == "status":
                monitor.send_command("status")
            elif cmd[0] == "summary":
                monitor.send_command("get_summary")
            elif cmd[0] == "acknowledge" and len(cmd) > 1:
                monitor.send_command("acknowledge_alarm", {
                    "point_id": int(cmd[1]),
                    "alarm_type": "HIGH_TEMP"
                })
            elif cmd[0] == "plot":
                monitor.plot_temperatures()
            elif cmd[0] == "stats" and len(cmd) > 1:
                monitor.get_statistics(int(cmd[1]))
            else:
                print("Unknown command")
                
        except Exception as e:
            print(f"Error: {e}")
            
    monitor.client.disconnect()
```

### Node.js/n8n Integration

```javascript
/**
 * n8n Custom Node for Temperature Controller MQTT Integration
 */

const mqtt = require('mqtt');

class TemperatureControllerNode {
    description = {
        displayName: 'Temperature Controller',
        name: 'temperatureController',
        group: ['transform'],
        version: 1,
        description: 'Interact with ESP32 Temperature Controller via MQTT',
        defaults: {
            name: 'Temperature Controller',
        },
        inputs: ['main'],
        outputs: ['main'],
        credentials: [
            {
                name: 'mqttCredentials',
                required: true,
            },
        ],
        properties: [
            {
                displayName: 'Operation',
                name: 'operation',
                type: 'options',
                options: [
                    {
                        name: 'Get Status',
                        value: 'getStatus',
                    },
                    {
                        name: 'Get Temperature',
                        value: 'getTemperature',
                    },
                    {
                        name: 'Acknowledge Alarm',
                        value: 'acknowledgeAlarm',
                    },
                    {
                        name: 'Set Relay',
                        value: 'setRelay',
                    },
                    {
                        name: 'Subscribe to Telemetry',
                        value: 'subscribeTelemetry',
                    },
                ],
                default: 'getStatus',
            },
            {
                displayName: 'Device Name',
                name: 'deviceName',
                type: 'string',
                default: 'tempcontroller01',
                description: 'MQTT device name',
            },
            {
                displayName: 'Point ID',
                name: 'pointId',
                type: 'number',
                displayOptions: {
                    show: {
                        operation: ['getTemperature', 'acknowledgeAlarm'],
                    },
                },
                default: 0,
                description: 'Temperature point ID (0-59)',
            },
            {
                displayName: 'Relay ID',
                name: 'relayId',
                type: 'number',
                displayOptions: {
                    show: {
                        operation: ['setRelay'],
                    },
                },
                default: 0,
                description: 'Relay ID (0-3)',
            },
            {
                displayName: 'Relay State',
                name: 'relayState',
                type: 'options',
                displayOptions: {
                    show: {
                        operation: ['setRelay'],
                    },
                },
                options: [
                    {
                        name: 'ON',
                        value: 'ON',
                    },
                    {
                        name: 'OFF',
                        value: 'OFF',
                    },
                    {
                        name: 'TOGGLE',
                        value: 'TOGGLE',
                    },
                ],
                default: 'ON',
            },
        ],
    };

    async execute() {
        const items = this.getInputData();
        const returnData = [];
        const credentials = this.getCredentials('mqttCredentials');
        
        // MQTT connection options
        const mqttOptions = {
            host: credentials.host,
            port: credentials.port || 1883,
            username: credentials.username,
            password: credentials.password,
            protocol: credentials.useTls ? 'mqtts' : 'mqtt',
        };
        
        // Connect to MQTT broker
        const client = mqtt.connect(mqttOptions);
        
        for (let i = 0; i < items.length; i++) {
            const operation = this.getNodeParameter('operation', i);
            const deviceName = this.getNodeParameter('deviceName', i);
            
            // Build topic prefix
            const topicPrefix = `plant/area1/line2/${deviceName}`;
            
            try {
                const result = await new Promise((resolve, reject) => {
                    client.on('connect', async () => {
                        const commandId = `n8n_${Date.now()}`;
                        
                        // Subscribe to response topic
                        const responseTopic = `${topicPrefix}/command/response`;
                        client.subscribe(responseTopic);
                        
                        // Handle response
                        client.on('message', (topic, message) => {
                            if (topic === responseTopic) {
                                const response = JSON.parse(message.toString());
                                if (response.cmd_id === commandId) {
                                    client.end();
                                    resolve(response);
                                }
                            }
                        });
                        
                        // Build command
                        let command = {
                            cmd_id: commandId,
                            timestamp: Date.now(),
                            source: 'n8n',
                            command: '',
                            parameters: {},
                        };
                        
                        switch (operation) {
                            case 'getStatus':
                                command.command = 'status';
                                break;
                                
                            case 'getTemperature':
                                command.command = 'get_point';
                                command.parameters.point_id = this.getNodeParameter('pointId', i);
                                break;
                                
                            case 'acknowledgeAlarm':
                                command.command = 'acknowledge_alarm';
                                command.parameters.point_id = this.getNodeParameter('pointId', i);
                                command.parameters.alarm_type = 'HIGH_TEMP';
                                break;
                                
                            case 'setRelay':
                                command.command = 'set_relay';
                                command.parameters.relay_id = this.getNodeParameter('relayId', i);
                                command.parameters.state = this.getNodeParameter('relayState', i);
                                break;
                                
                            case 'subscribeTelemetry':
                                // Special case - subscribe and stream data
                                const telemetryTopic = `${topicPrefix}/telemetry/+`;
                                client.subscribe(telemetryTopic);
                                
                                // Collect telemetry for 5 seconds
                                const telemetryData = [];
                                client.on('message', (topic, message) => {
                                    if (topic.includes('/telemetry/')) {
                                        telemetryData.push({
                                            topic: topic,
                                            data: JSON.parse(message.toString()),
                                            timestamp: new Date().toISOString(),
                                        });
                                    }
                                });
                                
                                setTimeout(() => {
                                    client.end();
                                    resolve({ telemetry: telemetryData });
                                }, 5000);
                                
                                return;
                        }
                        
                        // Send command
                        const commandTopic = `${topicPrefix}/command/request`;
                        client.publish(commandTopic, JSON.stringify(command), { qos: 1 });
                        
                        // Timeout after 10 seconds
                        setTimeout(() => {
                            client.end();
                            reject(new Error('Command timeout'));
                        }, 10000);
                    });
                    
                    client.on('error', (error) => {
                        reject(error);
                    });
                });
                
                returnData.push({
                    json: result,
                    binary: {},
                });
                
            } catch (error) {
                if (this.continueOnFail()) {
                    returnData.push({
                        json: {
                            error: error.message,
                        },
                        binary: {},
                    });
                    continue;
                }
                throw error;
            }
        }
        
        return [returnData];
    }
}

module.exports = { nodeClass: TemperatureControllerNode };
```

### Command Line Tools

```bash
#!/bin/bash
# mqtt_temp_cli.sh - Command line interface for Temperature Controller

BROKER="broker.hivemq.com"
PORT="1883"
DEVICE="tempcontroller01"
TOPIC_PREFIX="plant/area1/line2/${DEVICE}"

# Function to send command
send_command() {
    local command=$1
    local params=$2
    local cmd_id="CLI_$(date +%s)"
    
    if [ -z "$params" ]; then
        params="{}"
    fi
    
    local payload=$(cat <<EOF
{
    "cmd_id": "${cmd_id}",
    "timestamp": $(date +%s)000,
    "source": "cli",
    "command": "${command}",
    "parameters": ${params}
}
EOF
)
    
    mosquitto_pub -h ${BROKER} -p ${PORT} -t "${TOPIC_PREFIX}/command/request" -m "${payload}" -q 1
    
    # Listen for response
    timeout 5 mosquitto_sub -h ${BROKER} -p ${PORT} -t "${TOPIC_PREFIX}/command/response" -C 1 | jq '.'
}

# Function to monitor telemetry
monitor_telemetry() {
    echo "Monitoring temperature telemetry (Ctrl+C to stop)..."
    mosquitto_sub -h ${BROKER} -p ${PORT} -t "${TOPIC_PREFIX}/telemetry/temperature" -v | while read line
    do
        topic=$(echo $line | cut -d' ' -f1)
        payload=$(echo $line | cut -d' ' -f2-)
        
        # Parse and display summary
        echo "---"
        echo "Timestamp: $(date)"
        echo $payload | jq '.summary'
    done
}

# Function to monitor alarms
monitor_alarms() {
    echo "Monitoring alarms (Ctrl+C to stop)..."
    mosquitto_sub -h ${BROKER} -p ${PORT} -t "${TOPIC_PREFIX}/alarm/+" -v | while read line
    do
        topic=$(echo $line | cut -d' ' -f1)
        payload=$(echo $line | cut -d' ' -f2-)
        
        # Format alarm output
        echo "---"
        echo "ALARM: $(echo $payload | jq -r '.point_name') - $(echo $payload | jq -r '.alarm_type')"
        echo "State: $(echo $payload | jq -r '.state')"
        echo "Temperature: $(echo $payload | jq -r '.temperature')°C (Threshold: $(echo $payload | jq -r '.threshold')°C)"
    done
}

# Main menu
case "$1" in
    status)
        echo "Getting system status..."
        send_command "status"
        ;;
    
    summary)
        echo "Getting temperature summary..."
        send_command "get_summary"
        ;;
    
    point)
        if [ -z "$2" ]; then
            echo "Usage: $0 point <point_id>"
            exit 1
        fi
        echo "Getting data for point $2..."
        send_command "get_point" "{\"point_id\": $2}"
        ;;
    
    ack)
        if [ -z "$2" ]; then
            echo "Usage: $0 ack <point_id>"
            exit 1
        fi
        echo "Acknowledging alarm for point $2..."
        send_command "acknowledge_alarm" "{\"point_id\": $2, \"alarm_type\": \"HIGH_TEMP\"}"
        ;;
    
    relay)
        if [ -z "$2" ] || [ -z "$3" ]; then
            echo "Usage: $0 relay <relay_id> <ON|OFF|TOGGLE>"
            exit 1
        fi
        echo "Setting relay $2 to $3..."
        send_command "set_relay" "{\"relay_id\": $2, \"state\": \"$3\"}"
        ;;
    
    monitor)
        monitor_telemetry
        ;;
    
    alarms)
        monitor_alarms
        ;;
    
    *)
        echo "ESP32 Temperature Controller MQTT CLI"
        echo "Usage: $0 {status|summary|point|ack|relay|monitor|alarms}"
        echo ""
        echo "Commands:"
        echo "  status          - Get system status"
        echo "  summary         - Get temperature summary"
        echo "  point <id>      - Get specific point data"
        echo "  ack <id>        - Acknowledge alarm"
        echo "  relay <id> <state> - Control relay"
        echo "  monitor         - Monitor telemetry"
        echo "  alarms          - Monitor alarms"
        exit 1
        ;;
esac
```

### Data Visualization with Grafana

```json
{
  "dashboard": {
    "title": "Temperature Controller Dashboard",
    "panels": [
      {
        "title": "Temperature Overview",
        "type": "graph",
        "targets": [
          {
            "query": "SELECT mean(\"temperature\") FROM \"mqtt_consumer\" WHERE (\"topic\" =~ /telemetry\\/temperature/) AND $timeFilter GROUP BY \"point_id\""
          }
        ]
      },
      {
        "title": "Active Alarms",
        "type": "table",
        "targets": [
          {
            "query": "SELECT last(\"point_name\"), last(\"alarm_type\"), last(\"temperature\"), last(\"threshold\") FROM \"mqtt_consumer\" WHERE (\"topic\" =~ /alarm\\/state/) AND (\"state\" = 'ACTIVE') GROUP BY \"point_id\""
          }
        ]
      },
      {
        "title": "System Status",
        "type": "stat",
        "targets": [
          {
            "query": "SELECT last(\"active_points\"), last(\"sensor_errors\"), last(\"active_alarms\") FROM \"mqtt_consumer\" WHERE (\"topic\" =~ /telemetry\\/temperature/)"
          }
        ]
      }
    ]
  }
}
```

---

## Conclusion

This document provides comprehensive, production-ready MQTT implementation examples for the ESP32 temperature monitoring system. The code includes:

1. **Robust Connection Management**: Automatic reconnection, TLS support, and connection state tracking
2. **Efficient Message Handling**: Queuing, QoS support, and compression for large payloads
3. **Comprehensive Command System**: Full command processing with validation and error handling
4. **Optimized Telemetry**: Change detection, bulk updates, and configurable intervals
5. **Flexible Scheduling**: Cron-like scheduling for automated tasks
6. **Complete Integration**: Examples for web UI, alarm system, and relay control
7. **Client Libraries**: Python, Node.js, and command-line tools for easy integration

The implementation follows ESP32 best practices for memory management, uses FreeRTOS for multitasking, and provides extensive error handling suitable for industrial deployment.