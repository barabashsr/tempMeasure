# Component Specifications

## 1. MQTTManager (Partially Implemented)

### Current Implementation Status
- ✅ **Core Infrastructure**: Class structure, configuration, connection management
- ✅ **256dpi/MQTT Library**: Migrated from PubSubClient for better ESP32/TLS support
- ✅ **Web Configuration**: Complete UI with all settings
- ✅ **TLS Support**: Working with HiveMQ Cloud on port 8883
- ✅ **Test Publishing**: Counter publishing for connection testing
- ❌ **Temperature Publishing**: Methods exist but not integrated
- ❌ **Command Processing**: Not yet implemented
- ❌ **Alarm Notifications**: Not yet implemented

### Existing Class Structure
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

### Required Integration Points

#### 1. Temperature Publishing (Phase 1 Priority)
```cpp
// In MQTTManager::update() - needs activation
if (millis() - lastTelemetryPublish > publishIntervalMs) {
    publishTemperatureData(controller);  // Currently not called
    lastTelemetryPublish = millis();
}
```

#### 2. Alarm State Changes (Phase 1 Priority)
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

#### 3. Command Processing (Phase 1 Priority)
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

### Memory Management
- Static allocation where possible
- Pre-allocated message buffers (2KB)
- Command queue limit (10 commands)
- Telemetry buffer reuse

### Existing Topic Configuration
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

## 2. QR Code Display Enhancement

### Integration with IndicatorInterface

#### New Display Mode
```cpp
enum DisplayMode {
    // Existing modes...
    DISPLAY_STATUS_QR = 6  // New QR code page
};
```

#### QR Code Generation
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

#### Display Integration
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

## 3. Russian Translation Implementation

### Language Manager
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

### Web Interface Translation

#### JavaScript Integration
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

#### HTML Updates
```html
<!-- Before -->
<h1>Temperature Monitor</h1>

<!-- After -->
<h1 data-i18n="title.dashboard">Temperature Monitor</h1>
```

## 4. Modbus Register 899 Enhancement

### Command Processing
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

### Safety Validation
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
