# ConfigAssist Library Usage Guide

## Overview
The system uses ConfigAssist library for persistent configuration management with automatic web interface generation. ConfigAssist provides YAML-based configuration definition, automatic HTML form generation, and JSON/INI file storage.

## Library Integration
```cpp
// Platform configuration (platformio.ini)
lib_deps = https://github.com/gemi254/ConfigAssist.git
```

## Core Configuration Setup

### 1. YAML Configuration Definition
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

### 2. ConfigAssist Initialization
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

## Saving Settings to File

### 1. Automatic Saving
ConfigAssist automatically saves changes when values are modified through the web interface or programmatically:

```cpp
// Setting values automatically triggers save
conf["device_id"] = "1234";
conf["measurement_period"] = "30";
// File is automatically saved after changes
```

### 2. Manual Save
For explicit save operations:

```cpp
// Force save current configuration
conf.saveConfigFile();
```

### 3. Multiple Configuration Files
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

## Retrieving Settings from File

### 1. Using operator() for Read Access
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

### 2. Using operator[] for Read/Write Access
```cpp
// Read value
String currentSSID = conf["st_ssid"];

// Write value (triggers auto-save)
conf["st_ssid"] = "NewWiFiNetwork";
```

### 3. Loading from Secondary Files
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

## Declaring API Endpoints

### 1. Automatic ConfigAssist Endpoints
ConfigAssist automatically creates these endpoints:

```cpp
// Automatic endpoints created by conf.setup():
// GET /setup           - Configuration web interface
// GET /setup/values    - Get all values as JSON
// POST /setup/values   - Update values from JSON
// GET /setup/json      - Get configuration as JSON
// GET /setup/ini       - Download config.ini file
```

### 2. Custom API Endpoints Integration
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

### 3. Configuration Change Handling
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

## Advanced ConfigAssist Patterns

### 1. Dynamic Variable Substitution
ConfigAssist supports variable substitution in default values:

```cpp
// In YAML definition
default: 'temp-monitor-{mac}'  // {mac} will be replaced with MAC address
```

### 2. Value Validation
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

### 3. Conditional Configuration
```cpp
// Show/hide options based on other settings
if (conf("mqtt_enabled").toInt() == 1) {
    // MQTT is enabled, show MQTT settings in UI
    server->on("/api/mqtt/config", HTTP_GET, handleMQTTConfig);
}
```

### 4. Configuration Backup/Restore
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

## Best Practices

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
