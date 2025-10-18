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

## Conclusion

This architecture enhances the Temperature Controller system with modern connectivity and usability features while maintaining its industrial reliability. The modular design ensures each enhancement can be developed, tested, and deployed independently with minimal risk to existing functionality.

The MQTT infrastructure is substantially complete, requiring only the final integration hooks to activate the already-implemented publishing and command features. This significantly reduces the implementation risk and timeline.

---

*End of Architecture Document*