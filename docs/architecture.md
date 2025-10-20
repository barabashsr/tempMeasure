# Technical Architecture Document
## Temperature Controller System Enhancement

**Version**: 1.0  
**Date**: 2024-10-18  
**Author**: Winston (System Architect)  
**Project Type**: Brownfield Enhancement

---

## Executive Summary

This architecture document details the technical implementation approach for enhancing the existing Temperature Controller system with MQTT integration, Russian translation, QR code display, and Modbus completion. The design maintains strict backward compatibility while adding new capabilities through modular components.

## Current System Architecture

### Core Components Overview
```
┌─────────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│ TemperatureController│────▶│  MeasurementPoint│────▶│      Alarm      │
│   (Orchestrator)    │     │  (60 instances)  │     │  (State Machine)│
└──────┬──────────────┘     └──────────────────┘     └─────────────────┘
       │
       ├──▶ SensorManager (DS18B20 & PT1000)
       ├──▶ ConfigManager (ConfigAssist/YAML)
       ├──▶ LoggerManager (SD Card)
       ├──▶ TempModbusServer (RTU)
       ├──▶ IndicatorInterface (OLED/LED/Button)
       └──▶ Web Server (HTML/JS/API)
```

### Key Design Patterns
- **Observer Pattern**: Alarm notifications
- **Strategy Pattern**: Sensor implementations
- **State Machine**: Alarm stage transitions
- **Manager Pattern**: Subsystem isolation

## Enhanced Architecture

### Component Integration Overview
```
┌─────────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│ TemperatureController│────▶│  MeasurementPoint│────▶│      Alarm      │
│   (Orchestrator)    │     │  (60 instances)  │     │  (State Machine)│
└──────┬──────────────┘     └──────────────────┘     └─────────────────┘
       │
       ├──▶ SensorManager
       ├──▶ ConfigManager ◄─── [Extended for MQTT config]
       ├──▶ LoggerManager
       ├──▶ TempModbusServer ◄─── [Enhanced with Reg 899]
       ├──▶ IndicatorInterface ◄─── [QR Code Display]
       ├──▶ Web Server ◄─── [Russian Translation]
       └──▶ MQTTManager ◄─── [NEW Component]
```

## Component Specifications

### 1. MQTTManager (Partially Implemented)

#### Current Implementation Status
- ✅ **Core Infrastructure**: Class structure, configuration, connection management
- ✅ **256dpi/MQTT Library**: Migrated from PubSubClient for better ESP32/TLS support
- ✅ **Web Configuration**: Complete UI with all settings
- ✅ **TLS Support**: Working with HiveMQ Cloud on port 8883
- ✅ **Test Publishing**: Counter publishing for connection testing
- ❌ **Temperature Publishing**: Methods exist but not integrated
- ❌ **Command Processing**: Not yet implemented
- ❌ **Alarm Notifications**: Not yet implemented

#### Existing Class Structure
```cpp
class MQTTManager {
private:
    static MQTTConfig config;           // JSON-based configuration
    static WiFiClientSecure wifiClientSecure;  // TLS support
    static MQTTClient mqttClient;       // 256dpi/MQTT client
    
    // Connection management
    static bool isConnected;
    static unsigned long lastReconnectAttempt;
    
    // Publishing timing
    static unsigned long lastTelemetryPublish;
    static unsigned long publishIntervalMs;
    
public:
    static bool begin();
    static void update(TemperatureController& controller);
    static bool publish(const char* topic, const char* payload, bool retain, int qos);
    static bool loadConfig();  // From /config/mqtt.json
    static bool saveConfig();
    static String buildTopic(const String& topicType, const String& subtopic);
    
    // Methods ready but not called:
    static bool publishTemperatureData(TemperatureController& controller);
    static bool publishSystemStatus(TemperatureController& controller);
};
```

#### Required Integration Points

##### 1. Temperature Publishing (Phase 1 Priority)
```cpp
// In MQTTManager::update() - needs activation
if (millis() - lastTelemetryPublish > publishIntervalMs) {
    publishTemperatureData(controller);  // Currently not called
    lastTelemetryPublish = millis();
}
```

##### 2. Alarm State Changes (Phase 1 Priority)
```cpp
// In Alarm::setState() - needs implementation
if (newState != currentState) {
    // Existing logic...
    
    // MQTT notification hook - TO BE ADDED
    if (MQTTManager::isEnabled() && MQTTManager::connected()) {
        MQTTManager::publishAlarmChange(pointIndex, newState);
    }
}
```

##### 3. Command Processing (Phase 1 Priority)
```cpp
// In MQTTManager::messageReceived() - needs expansion
void MQTTManager::messageReceived(String &topic, String &payload) {
    // Currently only logs messages
    // TO BE ADDED: Command parsing and response
    
    if (topic.endsWith("/command/request")) {
        JsonDocument doc;
        deserializeJson(doc, payload);
        
        const char* cmd = doc["command"];
        const char* cmdId = doc["cmd_id"];
        
        if (strcmp(cmd, "get_status") == 0) {
            // Build and send response
        }
        else if (strcmp(cmd, "acknowledge_alarm") == 0) {
            // Process acknowledgment
        }
        // ... other commands
    }
}
```

#### Memory Management
- Static allocation where possible
- Pre-allocated message buffers (2KB)
- Command queue limit (10 commands)
- Telemetry buffer reuse

#### Existing Topic Configuration
The MQTT configuration already supports ISA-95 hierarchical topics:
```cpp
struct MQTTConfig {
    // Topic configuration (ISA-95 hierarchy)
    String topic_level1_type;  // "plant", "enterprise", "skip", "custom"
    String topic_level1_value;
    String topic_level2_type;  // "area", "department", "skip", "custom"
    String topic_level2_value;
    String topic_level3_type;  // "line", "cell", "skip", "custom"
    String topic_level3_value;
    String device_name;
    
    // The buildTopic() method constructs topics like:
    // plant/area1/line2/tempcontroller01/telemetry/temperature
};
```

### 2. QR Code Display Enhancement

#### Integration with IndicatorInterface

##### New Display Mode
```cpp
enum DisplayMode {
    // Existing modes...
    DISPLAY_STATUS_QR = 6  // New QR code page
};
```

##### QR Code Generation
```cpp
class QRCodeDisplay {
private:
    uint8_t qrcode[qrcode_getBufferSize(3)];  // Version 3 (29x29)
    
public:
    void generateWiFiQR(const char* ssid, const char* password, const char* ip);
    void generateURLQR(const char* url);
    void drawQR(OLEDDisplay* display, int offsetX, int offsetY);
};
```

##### Display Integration
```cpp
// In IndicatorInterface::displaySystemStatus()
case DISPLAY_STATUS_QR:
    if (WiFi.getMode() == WIFI_AP) {
        // AP mode - WiFi credentials + URL
        qrDisplay.generateWiFiQR(
            config->getAPSSID(),
            config->getAPPassword(),
            WiFi.softAPIP().toString().c_str()
        );
    } else {
        // Station mode - just URL
        qrDisplay.generateURLQR(WiFi.localIP().toString().c_str());
    }
    qrDisplay.drawQR(&display, 34, 0);  // Center on 128x64
    break;
```

### 3. Russian Translation Implementation

#### Language Manager
```cpp
class LanguageManager {
private:
    enum Language { LANG_EN, LANG_RU };
    Language currentLanguage;
    
    // String tables stored in PROGMEM
    static const char* const strings_en[] PROGMEM;
    static const char* const strings_ru[] PROGMEM;
    
public:
    const char* getString(StringID id);
    void setLanguage(Language lang);
    Language getLanguage();
};
```

#### Web Interface Translation

##### JavaScript Integration
```javascript
// language.js
const translations = {
    en: {
        "title.dashboard": "Temperature Monitor",
        "label.point": "Point",
        "label.temperature": "Temperature",
        "label.status": "Status",
        // ... all strings
    },
    ru: {
        "title.dashboard": "Монитор температуры",
        "label.point": "Точка",
        "label.temperature": "Температура", 
        "label.status": "Статус",
        // ... all strings
    }
};

function t(key) {
    return translations[currentLanguage][key] || key;
}
```

##### HTML Updates
```html
<!-- Before -->
<h1>Temperature Monitor</h1>

<!-- After -->
<h1 data-i18n="title.dashboard">Temperature Monitor</h1>
```

### 4. Modbus Register 899 Enhancement

#### Command Processing
```cpp
// In TempModbusServer::processWriteRegister()
case 899:  // Command register
    switch (value) {
        case 0x0001:  // Apply alarm configuration
            if (validatePendingConfig()) {
                applyAlarmConfiguration();
                logModbusChange("Alarm config applied");
            }
            break;
        case 0x0002:  // Reset min/max
            controller->resetMinMaxValues();
            logModbusChange("Min/max values reset");
            break;
        case 0x0003:  // Clear alarm history
            controller->clearAlarmHistory();
            logModbusChange("Alarm history cleared");
            break;
        case 0x00FF:  // Factory reset
            if (confirmationSequence == 0xDEAD) {
                performFactoryReset();
            }
            break;
    }
    break;
```

#### Safety Validation
```cpp
bool validatePendingConfig() {
    for (int i = 0; i < 60; i++) {
        float lowThreshold = pendingRegisters[600 + i] / 10.0;
        float highThreshold = pendingRegisters[700 + i] / 10.0;
        
        // Range check
        if (lowThreshold < -50.0 || highThreshold > 150.0) {
            return false;
        }
        
        // Logic check
        if (lowThreshold >= highThreshold) {
            return false;
        }
        
        // Hysteresis check
        float hysteresis = pendingRegisters[870 + i/3] / 10.0;
        if (hysteresis < 0.1 || hysteresis > 10.0) {
            return false;
        }
    }
    return true;
}
```

## Data Flow Architecture

### Temperature Data Flow
```
Sensors ──▶ MeasurementPoint ──▶ TemperatureController
                                          │
                    ┌─────────────────────┼─────────────────────┐
                    ▼                     ▼                     ▼
               RegisterMap          MQTTManager            WebServer
               (Modbus)             (Remote)               (Local)
```

### Alarm Flow
```
MeasurementPoint ──▶ Alarm (State Change) ──▶ TemperatureController
                                                       │
                  ┌────────────────────────────────────┼────────────────────────┐
                  ▼                                    ▼                        ▼
          IndicatorInterface                    MQTTManager              LoggerManager
          (LED/Display/Relay)                   (Publish)                (SD Card)
```

### Command Flow
```
MQTT Broker ──▶ MQTTManager ──▶ Command Parser ──▶ TemperatureController
                     │                                      │
                     └──────────── Response ◀──────────────┘
```

## Configuration Architecture

### MQTT Configuration Storage
```cpp
struct MQTTConfig {
    char broker[128];
    uint16_t port;
    char username[64];
    char password[64];
    char clientId[64];
    uint16_t telemetryInterval;
    uint8_t qosSettings;
    bool useTLS;
    char topicPrefix[64];
};
```

### Web Interface Configuration
```html
<!-- New settings-mqtt.html -->
<div class="config-section">
    <h2 data-i18n="mqtt.title">MQTT Configuration</h2>
    <form id="mqttForm">
        <label data-i18n="mqtt.broker">Broker:</label>
        <input type="text" id="mqttBroker" maxlength="128">
        
        <label data-i18n="mqtt.port">Port:</label>
        <input type="number" id="mqttPort" min="1" max="65535">
        
        <!-- ... other fields ... -->
    </form>
</div>
```

## Memory Architecture

### RAM Usage Analysis
```
Current System:        ~110KB
├── Core System:        80KB
├── Web Server:         20KB
└── Buffers:            10KB

Enhanced System:       ~140KB (+30KB)
├── MQTT Client:        15KB
├── Message Buffers:     8KB
├── Command Queue:       3KB
├── QR Code Buffer:      2KB
└── Translation Cache:   2KB
```

### Flash Usage
```
Current:              1.2MB
├── Application:      800KB
├── Web Files:        300KB
└── Libraries:        100KB

Enhanced:             1.4MB (+200KB)
├── MQTT Library:      50KB
├── QR Library:        20KB
├── Translations:      30KB
└── New Code:         100KB
```

## Performance Architecture

### Task Scheduling
```cpp
void TemperatureController::loop() {
    unsigned long currentMillis = millis();
    
    // High Priority (every loop)
    buttonHandler();
    alarmProcessor();
    
    // Medium Priority (100ms)
    if (currentMillis - lastDisplayUpdate > 100) {
        indicatorInterface->update();
        lastDisplayUpdate = currentMillis;
    }
    
    // Low Priority (1s)
    if (currentMillis - lastSensorRead > 1000) {
        readAllSensors();
        lastSensorRead = currentMillis;
    }
    
    // MQTT Tasks (non-blocking)
    if (mqttManager) {
        mqttManager->loop();  // Handles its own timing
    }
}
```

### MQTT Performance Optimization
- Batch temperature updates (60 points in one message)
- Delta reporting for changes only
- Command queue prevents blocking
- Async publish with callbacks
- Connection pooling for TLS

## Security Architecture

### MQTT Security Layers
1. **Authentication**: Username/password required
2. **Encryption**: Optional TLS/SSL support
3. **Authorization**: Read-only telemetry topics
4. **Rate Limiting**: 10 commands/minute
5. **Input Validation**: All commands validated

### Modbus Security
1. **Explicit Commands**: Register 899 triggers only
2. **Value Validation**: Range and logic checks
3. **Confirmation Sequence**: Critical operations
4. **Audit Logging**: All changes tracked

## Testing Architecture

### Unit Testing Strategy
```cpp
class MQTTManagerTest : public TestCase {
    void testConnectionHandling();
    void testMessagePublishing();
    void testCommandProcessing();
    void testMemoryLeaks();
    void testReconnection();
};
```

### Integration Testing
1. **MQTT + Alarms**: Verify notifications on state changes
2. **MQTT + Modbus**: Ensure no conflicts in data access
3. **QR + Display**: Test all display modes cycle correctly
4. **Translation + Web**: Verify all strings translated

### System Testing
1. **24-hour stability test** with all features active
2. **Network failure recovery** testing
3. **Memory leak detection** over extended periods
4. **Performance benchmarking** under load

## Deployment Architecture

### Feature Flags
```cpp
struct FeatureFlags {
    bool mqttEnabled = false;
    bool qrCodeEnabled = true;
    bool russianEnabled = true;
    bool modbus899Enabled = false;
};
```

### Phased Rollout
1. **Phase 1**: QR code and Russian translation (low risk)
2. **Phase 2**: MQTT telemetry only (read-only)
3. **Phase 3**: MQTT commands (with monitoring)
4. **Phase 4**: Modbus 899 (after validation)

### Rollback Strategy
- Feature flags allow instant disable
- Previous firmware kept for emergency rollback
- Configuration backup before changes
- Gradual rollout to test sites first

## Maintenance Architecture

### Logging Strategy
```cpp
enum LogLevel {
    LOG_ERROR,    // Always logged
    LOG_WARNING,  // Production logging
    LOG_INFO,     // Normal operations
    LOG_DEBUG     // Development only
};

// MQTT specific logging
mqttManager->setLogLevel(LOG_INFO);
Logger.log(LOG_INFO, "MQTT", "Published telemetry: %d points", pointCount);
```

### Monitoring Points
1. **MQTT Metrics**: Messages sent/received, reconnections
2. **Memory Usage**: Heap fragmentation, high water mark
3. **Performance**: Loop execution time, message latency
4. **Error Rates**: Failed publishes, command errors

## Architecture Decisions and Rationale

### Decision 1: Separate MQTTManager Class
**Rationale**: Maintains single responsibility principle, allows MQTT to be completely disabled without affecting core functionality.

### Decision 2: Event-Driven Integration
**Rationale**: Minimizes changes to existing code, reduces coupling, easier to test.

### Decision 3: Static Memory Allocation
**Rationale**: Predictable memory usage, no fragmentation, suitable for embedded systems.

### Decision 4: Phased Feature Deployment
**Rationale**: Reduces risk, allows monitoring of each feature's impact, easier rollback.

### Decision 5: Configuration via Web UI
**Rationale**: Consistent with existing system, no need for recompilation, user-friendly.

## Implementation Status Summary

### Already Implemented
1. **MQTT Core Infrastructure** (90% complete)
   - MQTTManager class with static architecture
   - 256dpi/MQTT library integration
   - TLS/SSL support working
   - Web configuration UI complete
   - JSON configuration persistence
   - ISA-95 topic structure
   - Connection management and reconnection

2. **Partial Implementations**
   - Temperature publishing methods exist but not called
   - System status methods exist but not called
   - Basic message receiving callback exists

### Required Implementations

#### Phase 1: MQTT Completion (1-2 days)
1. **Enable Temperature Publishing**
   - Call `publishTemperatureData()` in update loop
   - Set proper telemetry interval handling
   - Format JSON according to simplified architecture

2. **Implement Alarm Notifications**
   - Add hook in `Alarm::setState()`
   - Create `publishAlarmChange()` method
   - Format alarm JSON messages

3. **Basic Command Processing**
   - Expand `messageReceived()` callback
   - Implement command parser
   - Add handlers for: get_status, get_points_data, acknowledge_alarm, send_message

#### Phase 2: Russian Translation (2-3 days)
1. **Create Language Manager**
   - String tables in PROGMEM
   - Language switching logic

2. **Translate Web Interface**
   - Extract all strings to translation files
   - Update HTML with data-i18n attributes
   - Implement JavaScript translation system

3. **Translate System Messages**
   - Error messages
   - Alarm descriptions
   - Configuration tooltips

#### Phase 3: QR Code Display (1-2 days)
1. **Integrate QR Library**
   - Add QRCode library to platformio.ini
   - Create QRCodeDisplay class

2. **Add Display Mode**
   - New status page (mode 6)
   - WiFi QR for AP mode
   - URL QR for normal mode

3. **Button Navigation**
   - Long press entry
   - Page cycling

#### Phase 4: Modbus Completion (1 day)
1. **Register 899 Handler**
   - Command code processing
   - Validation logic
   - Safety confirmation

2. **Audit Logging**
   - Log all Modbus changes
   - Include timestamps and values

## MQTT Implementation Guide

### Overview
The MQTT implementation uses the **static class pattern** (singleton-like) to provide global access to MQTT functionality throughout the codebase. The `MQTTManager` class is implemented entirely with static members and methods, making it accessible from any component without passing instances.

### Static Class Design
```cpp
class MQTTManager {
private:
    static MQTTConfig config;
    static MQTTClient mqttClient;
    // All members are static
    MQTTManager() = delete;  // Prevent instantiation

public:
    // All methods are static - accessible anywhere
    static bool begin();
    static void update(TemperatureController& controller);
    static bool publish(const char* topic, const char* payload, bool retain, int qos);
    static bool isEnabled() { return configLoaded && config.enabled; }
    static bool connected() { return mqttClient.connected(); }
};
```

### Integration Points

#### 1. Main Application (main.cpp)
```cpp
void setup() {
    // Initialize MQTT after WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        MQTTManager::begin();
    }
}

void loop() {
    // Update MQTT in main loop
    if (MQTTManager::isEnabled()) {
        MQTTManager::update(temperatureController);
    }
}
```

#### 2. Alarm State Changes (Alarm.cpp)
Add MQTT notification when alarm state changes:
```cpp
void Alarm::setState(AlarmStage newStage) {
    if (stage != newStage) {
        AlarmStage oldStage = stage;
        stage = newStage;
        
        // Log state change
        Serial.printf("Alarm state changed: Point %d from %s to %s\n", 
                     pointIndex, getStageString(oldStage), getStageString(newStage));
        
        // MQTT notification - ADD THIS
        if (MQTTManager::isEnabled() && MQTTManager::connected()) {
            publishAlarmStateChange(oldStage, newStage);
        }
        
        // Update timestamps
        updateTimestamp(newStage);
    }
}

void Alarm::publishAlarmStateChange(AlarmStage oldStage, AlarmStage newStage) {
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["point_index"] = pointIndex;
    doc["point_name"] = measurementPoint->getName();
    doc["alarm_type"] = (alarmType == ALARM_LOW) ? "LOW" : "HIGH";
    doc["old_state"] = getStageString(oldStage);
    doc["new_state"] = getStageString(newStage);
    doc["current_temp"] = measurementPoint->getCurrentTemp();
    doc["threshold"] = (alarmType == ALARM_LOW) ? 
                       measurementPoint->getLowAlarmThreshold() : 
                       measurementPoint->getHighAlarmThreshold();
    
    String topic = MQTTManager::buildTopic("alarms", "state_change");
    String payload;
    serializeJson(doc, payload);
    
    MQTTManager::publish(topic, payload, true, 1);  // Retain=true, QoS=1
}
```

#### 3. Temperature Updates (TemperatureController.cpp)
Enable periodic temperature publishing in the update method:
```cpp
void MQTTManager::update(TemperatureController& controller) {
    if (!config.enabled) return;
    
    // Handle connection maintenance
    loop();
    
    // Publish temperature data periodically - ENABLE THIS
    if (connected()) {
        unsigned long now = millis();
        if (now - lastTelemetryPublish >= publishIntervalMs) {
            publishTemperatureData(controller);
            publishSystemStatus(controller);
            lastTelemetryPublish = now;
        }
    }
}
```

#### 4. Command Processing (MQTTManager.cpp)
Implement command handling in the message callback:
```cpp
void MQTTManager::messageReceived(String &topic, String &payload) {
    Serial.printf("[MQTT] Received: Topic=%s, Payload=%s\n", 
                  topic.c_str(), payload.c_str());
    
    // Check if this is a command request
    if (topic.endsWith("/command/request")) {
        processCommand(topic, payload);
    }
}

void MQTTManager::processCommand(const String& topic, const String& payload) {
    JsonDocument request;
    DeserializationError error = deserializeJson(request, payload);
    
    if (error) {
        sendCommandError("Invalid JSON", "");
        return;
    }
    
    const char* cmd = request["command"];
    const char* cmdId = request["cmd_id"] | "";
    
    if (!cmd) {
        sendCommandError("Missing command field", cmdId);
        return;
    }
    
    // Process commands
    if (strcmp(cmd, "get_status") == 0) {
        handleGetStatus(cmdId);
    } else if (strcmp(cmd, "get_points_data") == 0) {
        handleGetPointsData(cmdId, request["params"]);
    } else if (strcmp(cmd, "acknowledge_alarm") == 0) {
        handleAcknowledgeAlarm(cmdId, request["params"]);
    } else if (strcmp(cmd, "reset_min_max") == 0) {
        handleResetMinMax(cmdId, request["params"]);
    } else if (strcmp(cmd, "send_message") == 0) {
        handleSendMessage(cmdId, request["params"]);
    } else {
        sendCommandError("Unknown command", cmdId);
    }
}
```

#### 5. Web Interface Integration (ConfigManager.cpp)
Update MQTT settings when changed via web:
```cpp
void ConfigManager::onConfigChanged(String key) {
    if (key == "mqtt_enabled") {
        bool enabled = conf(key).toInt() == 1;
        if (enabled && !MQTTManager::isEnabled()) {
            MQTTManager::begin();
        } else if (!enabled && MQTTManager::isEnabled()) {
            MQTTManager::disconnect();
        }
    }
}
```

### MQTT Message Formats

#### 1. Temperature Telemetry
**Topic**: `{prefix}/{device_name}/telemetry/temperature`  
**Publish Interval**: Configurable (default 60 seconds)  
**Retain**: false  
**QoS**: 0  

**Message Example**:
```json
{
  "timestamp": 1706543210123,
  "device_name": "temp_controller_01",
  "measurement_points": [
    {
      "address": "28:FF:12:34:56:78:90:AB",
      "name": "Tank 1 Bottom",
      "type": "DS18B20",
      "value": 65.5,
      "min": 64.2,
      "max": 67.8,
      "alarm_status": "NORMAL",
      "error_status": false
    },
    {
      "address": "28:FF:AB:CD:EF:12:34:56",
      "name": "Tank 1 Top",
      "type": "DS18B20",
      "value": 66.2,
      "min": 65.0,
      "max": 68.1,
      "alarm_status": "HIGH_WARNING",
      "error_status": false
    },
    {
      "address": "PT1000_0",
      "name": "Reactor Core",
      "type": "PT1000",
      "value": 125.3,
      "min": 120.1,
      "max": 128.5,
      "alarm_status": "NORMAL",
      "error_status": false
    }
  ]
}
```

#### 2. System Status
**Topic**: `{prefix}/{device_name}/telemetry/status`  
**Publish Interval**: Same as temperature  
**Retain**: true  
**QoS**: 0  

**Message Example**:
```json
{
  "timestamp": 1706543210123,
  "device_name": "temp_controller_01",
  "device_id": 1001,
  "firmware_version": "2.1.0",
  "uptime": 3600,
  "alarms": {
    "active_count": 2,
    "acknowledged_count": 1,
    "total_count": 5
  },
  "network": {
    "wifi_connected": true,
    "wifi_ssid": "PlantNetwork",
    "wifi_rssi": -65,
    "ip_address": "192.168.1.100"
  },
  "memory": {
    "free_heap": 145632,
    "min_free_heap": 125000,
    "heap_size": 327680
  }
}
```

#### 3. Alarm State Change
**Topic**: `{prefix}/{device_name}/alarms/state_change`  
**Publish**: On state change  
**Retain**: true  
**QoS**: 1  

**Message Example**:
```json
{
  "timestamp": 1706543215456,
  "point_index": 5,
  "point_name": "Tank 2 Middle",
  "alarm_type": "HIGH",
  "old_state": "NORMAL",
  "new_state": "HIGH_WARNING",
  "current_temp": 68.5,
  "threshold": 68.0
}
```

#### 4. Command Request (Subscribe)
**Topic**: `{prefix}/{device_name}/command/request`  
**Direction**: Device subscribes  
**QoS**: 2  

**Command Examples**:

**4.1 Get Status Command**:
```json
{
  "command": "get_status",
  "cmd_id": "cmd_123456",
  "timestamp": 1706543220000
}
```

**4.2 Get Points Data Command**:
```json
{
  "command": "get_points_data",
  "cmd_id": "cmd_123457",
  "params": {
    "points": [0, 5, 10, 15],  // Optional: specific points
    "include_history": false
  }
}
```

**4.3 Acknowledge Alarm Command**:
```json
{
  "command": "acknowledge_alarm",
  "cmd_id": "cmd_123458",
  "params": {
    "point_index": 5,
    "alarm_type": "HIGH",
    "acknowledgment_message": "Operator aware, cooling initiated"
  }
}
```

**4.4 Reset Min/Max Command**:
```json
{
  "command": "reset_min_max",
  "cmd_id": "cmd_123459",
  "params": {
    "points": "all"  // or array of point indices
  }
}
```

**4.5 Send Message Command**:
```json
{
  "command": "send_message",
  "cmd_id": "cmd_123460",
  "params": {
    "message": "Maintenance scheduled for 14:00",
    "display_duration": 30
  }
}
```

#### 5. Command Response (Publish)
**Topic**: `{prefix}/{device_name}/command/response`  
**Publish**: In response to commands  
**Retain**: false  
**QoS**: 2  

**Response Examples**:

**5.1 Success Response**:
```json
{
  "cmd_id": "cmd_123456",
  "status": "success",
  "timestamp": 1706543221000,
  "data": {
    // Command-specific response data
  }
}
```

**5.2 Get Status Response**:
```json
{
  "cmd_id": "cmd_123456",
  "status": "success",
  "timestamp": 1706543221000,
  "data": {
    "device_id": 1001,
    "active_alarms": [
      {
        "point_index": 5,
        "point_name": "Tank 2 Middle",
        "alarm_type": "HIGH",
        "state": "HIGH_WARNING",
        "duration": 300
      }
    ],
    "sensor_count": {
      "ds18b20": 45,
      "pt1000": 8,
      "total": 53
    }
  }
}
```

**5.3 Error Response**:
```json
{
  "cmd_id": "cmd_123458",
  "status": "error",
  "timestamp": 1706543222000,
  "error": {
    "code": "INVALID_POINT",
    "message": "Point index 65 does not exist"
  }
}
```

#### 6. Last Will and Testament (LWT)
**Topic**: `{prefix}/{device_name}/state/connection`  
**Retain**: true  
**QoS**: 1  

**Online Message** (sent on connect):
```json
{
  "status": "online",
  "timestamp": 1706543200000
}
```

**Offline Message** (LWT):
```json
{
  "status": "offline"
}
```

### Topic Structure

The topic structure follows ISA-95 hierarchy pattern:
```
{level1}/{level2}/{level3}/{device_name}/{category}/{subcategory}
```

Example configurations:
- `plant/area1/line2/temp_controller_01/telemetry/temperature`
- `factory/building_a/zone3/tc_1001/alarms/state_change`
- `site/process/reactor/temp_mon_05/command/request`

Topic building example:
```cpp
// Configure hierarchy in settings
config.topic_level1_type = "plant";
config.topic_level1_value = "chemical_plant_1";
config.topic_level2_type = "area";
config.topic_level2_value = "reactor_area";
config.topic_level3_type = "line";
config.topic_level3_value = "reactor_1";
config.device_name = "temp_ctrl_r1";

// Results in topics like:
// chemical_plant_1/reactor_area/reactor_1/temp_ctrl_r1/telemetry/temperature
```

### Implementation Checklist

To fully integrate MQTT functionality:

1. **In TemperatureController::loop()**:
   ```cpp
   // Add MQTT update call
   if (MQTTManager::isEnabled()) {
       MQTTManager::update(*this);
   }
   ```

2. **In Alarm::setState()**:
   - Add alarm state change publishing

3. **In MQTTManager::update()**:
   - Uncomment temperature publishing calls
   - Remove test counter publishing

4. **In MQTTManager::messageReceived()**:
   - Implement command processing logic

5. **Subscribe to command topic** in connect():
   ```cpp
   String cmdTopic = buildTopic("command", "request");
   mqttClient.subscribe(cmdTopic, config.qos_commands);
   ```

6. **Add command handlers**:
   - Implement each command processing function
   - Send appropriate responses

### Memory Considerations

- Message buffer: 4KB allocated for MQTT client
- JSON documents: Use StaticJsonDocument where possible
- Topic strings: Build dynamically, don't store
- Payload optimization: Only send changed values when possible

### Error Handling

```cpp
// Connection error handling
if (!MQTTManager::connected()) {
    // MQTT operations will fail gracefully
    // Automatic reconnection handled internally
}

// Publish error handling
if (!MQTTManager::publish(topic, payload, retain, qos)) {
    Serial.printf("MQTT publish failed for topic: %s\n", topic);
    // Log error but don't block operation
}
```

### Testing MQTT Integration

1. **Test connection**: Check test topic publishing
2. **Verify telemetry**: Monitor temperature/status topics
3. **Test alarms**: Trigger alarm conditions
4. **Command testing**: Send each command type
5. **Disconnection handling**: Test network interruptions
6. **Load testing**: Verify with all 60 points active

## ConfigAssist Library Usage Guide

### Overview
The system uses ConfigAssist library for persistent configuration management with automatic web interface generation. ConfigAssist provides YAML-based configuration definition, automatic HTML form generation, and JSON/INI file storage.

### Library Integration
```cpp
// Platform configuration (platformio.ini)
lib_deps = https://github.com/gemi254/ConfigAssist.git
```

### Core Configuration Setup

#### 1. YAML Configuration Definition
Define configuration structure using YAML format with validation rules:

```cpp
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
    Wifi settings:
      - st_ssid:
          label: WiFi SSID
          default: ""
          max: 32
      - st_pass:
          label: WiFi Password  
          default: ""
          type: password
          max: 64
      - host_name:
          label: Device Hostname
          default: 'temp-monitor-{mac}'
          max: 32
    
    Device settings:
      - device_id:
          label: Device ID
          type: number
          min: 1
          max: 9999
          default: 1000
      - measurement_period:
          label: Measurement Period (seconds)
          type: number
          min: 1
          max: 3600
          default: 10
      - modbus_enabled:
          label: Enable Modbus
          type: checkbox
          default: 1
    
    MQTT settings:
      - mqtt_enabled:
          label: Enable MQTT
          type: checkbox
          default: 0
      - mqtt_broker:
          label: MQTT Broker
          default: ""
          max: 128
)~";
```

#### 2. ConfigAssist Initialization
```cpp
class ConfigManager {
private:
    ConfigAssist conf;
    WebServer* server;
    
public:
    ConfigManager(TemperatureController& tempController) 
        : conf("/config.ini", VARIABLES_DEF_YAML),  // Primary config file
          controller(tempController) {
        
        server = new WebServer(80);
    }
    
    void begin() {
        // Mount filesystem
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount LittleFS");
            return;
        }
        
        // Setup ConfigAssist with web server
        bool startAP = shouldStartAP();
        conf.setup(*server, startAP);
        
        // Set configuration change callback
        conf.setRemotUpdateCallback(onConfigChanged);
        
        // Start web server
        server->begin();
    }
};
```

### Saving Settings to File

#### 1. Automatic Saving
ConfigAssist automatically saves changes when values are modified through the web interface or programmatically:

```cpp
// Setting values automatically triggers save
conf["device_id"] = "1234";
conf["measurement_period"] = "30";
// File is automatically saved after changes
```

#### 2. Manual Save
For explicit save operations:

```cpp
// Force save current configuration
conf.saveConfigFile();
```

#### 3. Multiple Configuration Files
For secondary configuration files (e.g., measurement points):

```cpp
// Create secondary config without YAML definition
ConfigAssist pointsConf("/points2.ini", false);

// Set values
for (int i = 0; i < 60; i++) {
    String key = "P" + String(i);
    MeasurementPoint* point = controller.getPoint(i);
    
    pointsConf[key + "_name"] = point->getName();
    pointsConf[key + "_enabled"] = String(point->isEnabled() ? 1 : 0);
    pointsConf[key + "_low_alarm"] = String(point->getLowAlarmThreshold());
    pointsConf[key + "_high_alarm"] = String(point->getHighAlarmThreshold());
}

// Manually save the file
pointsConf.saveConfigFile();
```

### Retrieving Settings from File

#### 1. Using operator() for Read Access
The operator() provides read-only access to configuration values:

```cpp
// Get string values
String getWifiSSID() { return conf("st_ssid"); }
String getWifiPassword() { return conf("st_pass"); }
String getHostname() { return conf("host_name"); }

// Get numeric values with conversion
uint16_t getDeviceId() { return conf("device_id").toInt(); }
uint16_t getMeasurementPeriod() { return conf("measurement_period").toInt(); }

// Get boolean values
bool isModbusEnabled() { return conf("modbus_enabled").toInt() == 1; }
bool isMQTTEnabled() { return conf("mqtt_enabled").toInt() == 1; }
```

#### 2. Using operator[] for Read/Write Access
```cpp
// Read value
String currentSSID = conf["st_ssid"];

// Write value (triggers auto-save)
conf["st_ssid"] = "NewWiFiNetwork";
```

#### 3. Loading from Secondary Files
```cpp
// Load existing configuration file
ConfigAssist alarmsConf("/alarms.ini", false);

// Read values
for (int i = 0; i < 20; i++) {
    String key = "A" + String(i);
    String name = alarmsConf[key + "_name"];
    float threshold = alarmsConf[key + "_threshold"].toFloat();
    bool enabled = alarmsConf[key + "_enabled"].toInt() == 1;
}
```

### Declaring API Endpoints

#### 1. Automatic ConfigAssist Endpoints
ConfigAssist automatically creates these endpoints:

```cpp
// Automatic endpoints created by conf.setup():
// GET /setup           - Configuration web interface
// GET /setup/values    - Get all values as JSON
// POST /setup/values   - Update values from JSON
// GET /setup/json      - Get configuration as JSON
// GET /setup/ini       - Download config.ini file
```

#### 2. Custom API Endpoints Integration
Add custom endpoints that work with ConfigAssist:

```cpp
void ConfigManager::setupAPIEndpoints() {
    // Status endpoint using ConfigAssist values
    server->on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc;
        
        // Add ConfigAssist values to response
        doc["device_id"] = conf("device_id");
        doc["hostname"] = conf("host_name");
        doc["wifi_ssid"] = conf("st_ssid");
        doc["modbus_enabled"] = conf("modbus_enabled").toInt() == 1;
        doc["mqtt_enabled"] = conf("mqtt_enabled").toInt() == 1;
        doc["measurement_period"] = conf("measurement_period").toInt();
        
        // Add runtime values
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
    });
    
    // Export settings as CSV
    server->on("/api/settings/export", HTTP_GET, [this]() {
        String csv = "Setting,Value\n";
        
        // WiFi Settings
        csv += "st_ssid," + _escapeCSVField(conf("st_ssid")) + "\n";
        csv += "st_pass," + _escapeCSVField(conf("st_pass")) + "\n";
        csv += "host_name," + _escapeCSVField(conf("host_name")) + "\n";
        
        // Device Settings
        csv += "device_id," + conf("device_id") + "\n";
        csv += "measurement_period," + conf("measurement_period") + "\n";
        csv += "modbus_enabled," + conf("modbus_enabled") + "\n";
        
        server->send(200, "text/csv", csv);
    });
    
    // Import settings from CSV
    server->on("/api/settings/import", HTTP_POST, []() {
        server->send(200);
    }, [this]() {
        HTTPUpload& upload = server->upload();
        if (upload.status == UPLOAD_FILE_END) {
            String csvData = upload.buf;
            
            // Parse CSV and update ConfigAssist
            // ... CSV parsing logic ...
            
            // Update values (auto-saves)
            conf["device_id"] = parsedDeviceId;
            conf["measurement_period"] = parsedPeriod;
        }
    });
}
```

#### 3. Configuration Change Handling
React to configuration changes in real-time:

```cpp
// Static callback function
void ConfigManager::onConfigChanged(String key) {
    if (instance == nullptr) return;
    
    // Handle specific key changes
    if (key == "device_id") {
        uint16_t newId = instance->conf(key).toInt();
        instance->controller.setDeviceId(newId);
        Serial.printf("Device ID changed to: %d\n", newId);
        
    } else if (key == "measurement_period") {
        uint16_t newPeriod = instance->conf(key).toInt();
        instance->controller.setMeasurementPeriod(newPeriod);
        Serial.printf("Measurement period changed to: %d seconds\n", newPeriod);
        
    } else if (key == "modbus_enabled") {
        bool enabled = instance->conf(key).toInt() == 1;
        instance->controller.setModbusEnabled(enabled);
        
    } else if (key == "mqtt_enabled") {
        bool enabled = instance->conf(key).toInt() == 1;
        if (enabled) {
            MQTTManager::begin();
        } else {
            MQTTManager::disconnect();
        }
        
    } else if (key == "reset_min_max") {
        if (instance->conf(key).toInt() == 1) {
            instance->resetMinMaxValues();
            instance->conf[key] = "0";  // Reset the flag
        }
    }
}
```

### Advanced ConfigAssist Patterns

#### 1. Dynamic Variable Substitution
ConfigAssist supports variable substitution in default values:

```cpp
// In YAML definition
default: 'temp-monitor-{mac}'  // {mac} will be replaced with MAC address
```

#### 2. Value Validation
Define validation rules in YAML:

```cpp
- temperature_offset:
    label: Temperature Offset
    type: number
    min: -10.0
    max: 10.0
    step: 0.1
    default: 0.0
```

#### 3. Conditional Configuration
```cpp
// Show/hide options based on other settings
if (conf("mqtt_enabled").toInt() == 1) {
    // MQTT is enabled, show MQTT settings in UI
    server->on("/api/mqtt/config", HTTP_GET, handleMQTTConfig);
}
```

#### 4. Configuration Backup/Restore
```cpp
// Backup configuration
void backupConfiguration() {
    File configFile = LittleFS.open("/config.ini", "r");
    File backupFile = LittleFS.open("/config.bak", "w");
    
    while (configFile.available()) {
        backupFile.write(configFile.read());
    }
    
    configFile.close();
    backupFile.close();
}

// Restore configuration
void restoreConfiguration() {
    LittleFS.remove("/config.ini");
    LittleFS.rename("/config.bak", "/config.ini");
    
    // Reload ConfigAssist
    conf.loadConfigFile();
}
```

### Best Practices

1. **Use PROGMEM for YAML Definitions**
   - Saves RAM by storing configuration structure in flash

2. **Implement Change Callbacks**
   - React to configuration changes without polling

3. **Validate Input Ranges**
   - Define min/max values in YAML to prevent invalid configurations

4. **Use Separate Files for Different Configs**
   - Main settings in `/config.ini`
   - Feature-specific settings in separate files

5. **Provide Default Values**
   - Ensure system can start with factory defaults

6. **Escape Special Characters**
   - Use proper escaping for CSV export/import

7. **Thread Safety**
   - ConfigAssist is not thread-safe; use from main loop only

## Comprehensive Class Descriptions and Enhancement Requirements

### Core System Classes

#### 1. TemperatureController (src/TemperatureController.cpp)
**Primary Responsibility**: Main orchestrator that coordinates all system components, manages measurement points, and handles the main control loop.

**Current Key Methods**:
- `begin()` - Initialize all subsystems
- `update()` - Main control loop
- `getMeasurementPoint(int index)` - Access specific measurement point
- `getAllMeasurementPoints()` - Get all points for bulk operations
- `getActiveAlarms()` - Retrieve current alarm states
- `resetMinMaxValues()` - Reset temperature statistics

**Additional Methods Needed for New Features**:
```cpp
// MQTT Integration Support
void setMQTTCallback(std::function<void(int, AlarmStage, AlarmStage)> callback);
String getSystemStatusJSON();  // For MQTT status publishing
bool acknowledgeAlarmRemote(int pointIndex, AlarmType type, String message);
void displayOLEDMessage(String message, int duration);  // For MQTT send_message command

// Bulk Operations for MQTT Commands
JsonDocument getPointsDataJSON(std::vector<int> pointIndices);
bool resetMinMaxForPoints(std::vector<int> pointIndices);

// Russian Translation Support
void setLanguage(Language lang);
Language getCurrentLanguage();
```

#### 2. ConfigManager (src/ConfigManager.cpp)
**Primary Responsibility**: Central configuration hub managing web interface, WiFi settings, and coordinating all configuration-related operations through ConfigAssist.

**Current Key Methods**:
- `begin()` - Initialize ConfigAssist and web server
- `setupAPIEndpoints()` - Configure all REST API endpoints
- `handleCSVUpload()` - Process CSV configuration imports
- `generateStatusJSON()` - Create system status responses

**Additional Methods Needed for New Features**:
```cpp
// MQTT Configuration Management
void handleMQTTConfigUpdate(JsonDocument& mqttConfig);
bool validateMQTTSettings(const MQTTConfig& config);
void onMQTTSettingChanged(String key, String value);

// MQTT History Viewer Support
void handleMQTTHistoryRequest();
void handleMQTTLogFileList();
void handleMQTTLogDownload(String filename);
bool isMQTTHistoryEnabled() { return conf("mqtt_history_enabled").toInt() == 1; }

// Language Management
void handleLanguageChange(String languageCode);
String getCurrentLanguageCode();
JsonDocument getTranslationJSON(String languageCode);

// QR Code Configuration
String getQRCodeContent();  // Generate appropriate QR content based on WiFi mode
bool isQRCodeEnabled() { return conf("qr_code_enabled").toInt() == 1; }
```

#### 3. LoggerManager (src/LoggerManager.cpp)
**Primary Responsibility**: Comprehensive logging system managing three subsystems - measurement logging, event logging, and alarm logging to SD card.

**Current Key Methods**:
- `logMeasurement()` - Log temperature data
- `logEvent()` - Log system events with severity
- `logAlarm()` - Log alarm state changes
- `getMeasurementLogs()` - Retrieve historical data
- File management with daily rotation

**Additional Methods Needed for New Features**:
```cpp
// MQTT History Logging
bool logMQTTMessage(MQTTLogDirection direction, const String& topic, 
                    size_t messageSize, const String& preview, 
                    MQTTLogPriority priority = MQTT_NORMAL);
bool isMQTTLoggingEnabled();
void setMQTTLoggingEnabled(bool enabled);
std::vector<String> getMQTTLogFiles();
String getMQTTLogContent(String filename, int maxLines = 1000);
bool rotateMQTTLogs(int retentionDays);

// Enhanced CSV Operations
bool exportLogsAsCSV(LogType type, String startDate, String endDate);
CSVExportResult exportFilteredLogs(LogFilter filter);

// Modbus Audit Logging
bool logModbusCommand(uint16_t registerNum, uint16_t oldValue, 
                      uint16_t newValue, String source);

// Performance Monitoring
void enablePerformanceLogging(bool enable);
void logPerformanceMetric(String metric, float value);
```

#### 4. CSVConfigManager (src/CSVConfigManager.cpp)
**Primary Responsibility**: Handles measurement point and alarm configuration import/export in CSV format.

**Current Key Methods**:
- `parseAlarmConfig()` - Import alarm settings from CSV
- `exportMeasurementPoints()` - Export current configuration
- `validateCSVData()` - Basic validation

**Additional Methods Needed for New Features**:
```cpp
// Template Generation for User Guidance
String generateCSVTemplate(CSVTemplateType type);
String generateRussianCSVTemplate(CSVTemplateType type);

// Enhanced Validation
CSVValidationResult validateCSVWithDetailedErrors(String csvData);
bool validateThresholdLogic(float low, float high, float hysteresis);
std::vector<String> getValidationErrors();

// Bulk Operations Support
bool importBulkConfiguration(String csvData, bool validateOnly = false);
String exportConfigurationWithMetadata();

// Russian Language Support
void setCSVLanguage(Language lang);
String getLocalizedCSVHeader(CSVColumnType column);
```

#### 5. MQTTManager (src/MQTTManager.cpp)
**Primary Responsibility**: Static class managing MQTT connectivity, publishing, and command processing with TLS support.

**Current Implementation Status**:
- ✅ Static class design with 256dpi/MQTT library
- ✅ TLS/SSL support working
- ✅ Configuration persistence
- ✅ Test publishing functional
- ❌ Temperature publishing not integrated
- ❌ Command processing not implemented

**Additional Methods Needed for Completion**:
```cpp
// Alarm Integration
static bool publishAlarmStateChange(int pointIndex, const String& pointName,
                                   AlarmType type, AlarmStage oldStage, 
                                   AlarmStage newStage, float currentTemp,
                                   float threshold);

// Command Handlers
static void handleGetStatus(const String& cmdId);
static void handleGetPointsData(const String& cmdId, JsonObject params);
static void handleAcknowledgeAlarm(const String& cmdId, JsonObject params);
static void handleResetMinMax(const String& cmdId, JsonObject params);
static void handleSendMessage(const String& cmdId, JsonObject params);
static void handleSetAlarmThreshold(const String& cmdId, JsonObject params);

// History/Logging Integration
static void setLoggingCallback(std::function<bool(MQTTLogDirection, 
                               const String&, size_t, const String&, 
                               MQTTLogPriority)> callback);

// Enhanced Publishing
static bool publishChangedValues(TemperatureController& controller);
static bool publishBulkAlarms(const std::vector<AlarmInfo>& alarms);

// Offline Queue Management  
static bool queueMessageForLater(const String& topic, const String& payload);
static void processOfflineQueue();
static size_t getQueuedMessageCount();
```

#### 6. IndicatorInterface (src/IndicatorInterface.cpp)
**Primary Responsibility**: Manages OLED display, LED indicators, relay outputs, and button interface.

**Current Key Methods**:
- `displaySystemStatus()` - Show various status screens
- `updateLEDs()` - Control indicator LEDs
- `handleButton()` - Process button inputs
- Display rotation through multiple screens

**Additional Methods Needed for New Features**:
```cpp
// QR Code Display
void displayQRCode();
void generateWiFiQR(const char* ssid, const char* password);
void generateURLQR(const char* url);
bool enterQRMode(uint32_t timeout = 30000);

// Russian Display Support
void setDisplayLanguage(Language lang);
String getLocalizedStatus(StatusType status);
String getLocalizedAlarmText(AlarmType type, AlarmStage stage);

// MQTT Status Display
void displayMQTTStatus(bool connected, int messageCount);
void showMQTTMessage(String message, uint32_t duration);

// Enhanced Navigation
bool isInQRMode();
void addCustomStatusPage(std::function<void(OLEDDisplay*)> renderer);
```

#### 7. Alarm (src/Alarm.cpp)
**Primary Responsibility**: Implements state machine for alarm management with hysteresis, acknowledgment, and stage transitions.

**Current Key Methods**:
- `check()` - Evaluate alarm conditions
- `acknowledge()` - Local acknowledgment
- `setState()` - State transitions
- `getStageString()` - Status text

**Additional Methods Needed for New Features**:
```cpp
// MQTT Integration
void setStateChangeCallback(std::function<void(AlarmStage, AlarmStage)> callback);
bool acknowledgeRemote(String acknowledgedBy, String message);

// Russian Language Support  
String getLocalizedStageString(AlarmStage stage, Language lang);
String getLocalizedDescription(Language lang);

// Enhanced State Information
JsonDocument getAlarmStateJSON();
uint32_t getTimeInCurrentStage();
String getAcknowledgmentInfo();
```

#### 8. ModbusRegistersMap (src/ModbusRegistersMap.cpp)
**Primary Responsibility**: Maps Modbus registers to system variables and handles register read/write operations.

**Current Implementation**:
- Complete register mapping
- Read handlers for all registers
- Basic write handlers
- ❌ Register 899 command processing missing

**Additional Methods Needed for New Features**:
```cpp
// Register 899 Command Processing
ModbusCommandResult processCommand899(uint16_t commandCode);
bool validatePendingConfiguration();
void applyPendingConfiguration();
void clearPendingConfiguration();

// Safety and Validation
bool requiresConfirmation(uint16_t registerNum, uint16_t value);
bool validateRegisterWrite(uint16_t registerNum, uint16_t value);
void setCommandTimeout(uint32_t timeoutMs);

// Audit Support
void setAuditCallback(std::function<void(uint16_t, uint16_t, uint16_t)> callback);
String getLastCommandError();
```

### Support Classes

#### 9. SensorManager
**Primary Responsibility**: Manages DS18B20 and PT1000 sensor interfaces with error handling.

**Additional Methods Needed**:
```cpp
// Bulk Operations
std::vector<SensorReading> readAllSensorsOptimized();
bool detectNewSensors();

// Diagnostics
JsonDocument getSensorDiagnostics();
int getSensorErrorCount();
```

#### 10. MeasurementPoint
**Primary Responsibility**: Represents a single measurement point with temperature data and alarm configuration.

**Additional Methods Needed**:
```cpp
// MQTT Support
JsonDocument toJSON();
bool updateFromJSON(JsonObject data);

// History
std::vector<float> getRecentValues(int count);
float getAverageTemp(uint32_t periodMs);
```

### New Classes Required

#### 11. LanguageManager (New)
```cpp
class LanguageManager {
public:
    enum Language { LANG_EN, LANG_RU };
    
    static void setLanguage(Language lang);
    static Language getCurrentLanguage();
    static const char* getString(StringID id);
    static String getFormattedString(StringID id, ...);
    
    // Web interface support
    static JsonDocument getWebTranslations(Language lang);
    static bool loadTranslationFile(String filename);
    
private:
    static Language currentLanguage;
    static const char* const strings_en[] PROGMEM;
    static const char* const strings_ru[] PROGMEM;
};
```

#### 12. QRCodeDisplay (New)
```cpp
class QRCodeDisplay {
public:
    void begin();
    bool generateWiFiQR(const String& ssid, const String& password);
    bool generateURLQR(const String& url);
    void drawOnOLED(OLEDDisplay* display, int x, int y);
    
    // Configuration
    void setErrorCorrection(qrcode_ecc_t ecc);
    void setModuleSize(uint8_t size);
    
private:
    uint8_t qrcode[qrcode_getBufferSize(3)];
    bool qrValid;
    QRCodeConfig config;
};
```

#### 13. MQTTHistoryViewer (New Web Component)
```javascript
// JavaScript class for MQTT history viewing
class MQTTHistoryViewer {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.currentFilter = {};
    }
    
    async loadLogFiles() {
        const response = await fetch('/api/mqtt/logs');
        this.logFiles = await response.json();
        this.renderFileList();
    }
    
    async loadLogContent(filename) {
        const response = await fetch(`/api/mqtt/logs/${filename}`);
        const csvData = await response.text();
        this.parseAndRenderCSV(csvData);
    }
    
    parseAndRenderCSV(csvData) {
        // Parse CSV and render table with filtering
    }
    
    applyFilter(direction, topic, priority) {
        // Client-side filtering
    }
    
    downloadLog(filename) {
        window.location.href = `/api/mqtt/logs/${filename}/download`;
    }
}
```

### Integration Patterns

#### Event-Driven Architecture
To minimize coupling and support the new features, implement event-driven patterns:

```cpp
// Global Event Manager
class EventManager {
public:
    // Alarm Events
    static std::function<void(int, AlarmStage, AlarmStage)> onAlarmStateChange;
    
    // Configuration Events  
    static std::function<void(String, String)> onConfigChange;
    
    // Modbus Events
    static std::function<void(uint16_t, uint16_t, uint16_t)> onModbusWrite;
    
    // System Events
    static std::function<void(SystemEvent)> onSystemEvent;
};

// Usage Example
EventManager::onAlarmStateChange = [](int point, AlarmStage oldS, AlarmStage newS) {
    if (MQTTManager::isEnabled()) {
        MQTTManager::publishAlarmStateChange(point, /* ... */);
    }
    if (LoggerManager::isEnabled()) {
        LoggerManager::logAlarmTransition(point, oldS, newS);
    }
};
```

### Memory Optimization Strategies

Given the ESP32's memory constraints, implement these patterns:

1. **Shared Buffers**:
```cpp
class BufferPool {
    static char mqttBuffer[4096];
    static char csvBuffer[2048];
    static char jsonBuffer[2048];
    
public:
    static char* getMQTTBuffer() { return mqttBuffer; }
    static char* getCSVBuffer() { return csvBuffer; }
    static char* getJSONBuffer() { return jsonBuffer; }
};
```

2. **Conditional Compilation**:
```cpp
#ifdef ENABLE_MQTT_HISTORY
    void LoggerManager::logMQTTMessage(...) { /* ... */ }
#endif
```

3. **Dynamic Feature Loading**:
```cpp
if (conf("mqtt_enabled").toInt() == 1) {
    MQTTManager::begin();
} else {
    // MQTT code not loaded, saving ~30KB RAM
}
```

## Conclusion

This architecture enhances the Temperature Controller system with modern connectivity and usability features while maintaining its industrial reliability. The modular design ensures each enhancement can be developed, tested, and deployed independently with minimal risk to existing functionality.

The comprehensive class descriptions and additional method specifications provide a clear roadmap for developers to implement the new features without reinventing existing functionality. The event-driven patterns and memory optimization strategies ensure the system remains responsive and stable.

Key architectural benefits:
- **Reusability**: Existing classes are extended rather than replaced
- **Modularity**: Each feature can be enabled/disabled independently  
- **Maintainability**: Clear separation of concerns and minimal coupling
- **Performance**: Optimized memory usage and non-blocking operations
- **Extensibility**: Event system allows future features without core changes

The MQTT infrastructure is substantially complete, requiring only the final integration hooks to activate the already-implemented publishing and command features. This significantly reduces the implementation risk and timeline.

---

*End of Architecture Document*