# Comprehensive Class Descriptions and Enhancement Requirements

## Core System Classes

### 1. TemperatureController (src/TemperatureController.cpp)
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

### 2. ConfigManager (src/ConfigManager.cpp)
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

### 3. LoggerManager (src/LoggerManager.cpp)
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

### 4. CSVConfigManager (src/CSVConfigManager.cpp)
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

### 5. MQTTManager (src/MQTTManager.cpp)
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

### 6. IndicatorInterface (src/IndicatorInterface.cpp)
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

### 7. Alarm (src/Alarm.cpp)
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

### 8. ModbusRegistersMap (src/ModbusRegistersMap.cpp)
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

## Support Classes

### 9. SensorManager
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

### 10. MeasurementPoint
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

## New Classes Required

### 11. LanguageManager (New)
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

### 12. QRCodeDisplay (New)
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

### 13. MQTTHistoryViewer (New Web Component)
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

## Integration Patterns

### Event-Driven Architecture
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

## Memory Optimization Strategies

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
