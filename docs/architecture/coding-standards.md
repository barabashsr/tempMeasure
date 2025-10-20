# Coding Standards

## Overview

This document defines the coding standards for the Temperature Controller System Enhancement project. All code contributions must adhere to these standards to ensure consistency, maintainability, and quality.

## Documentation Standards

### Doxygen Comments (MANDATORY)

**ALL code MUST be thoroughly documented using Doxygen-style comments.** This is a core requirement for the project.

#### File Headers
Every source file must begin with a Doxygen file header:

```cpp
/**
 * @file ConfigManager.cpp
 * @brief Central configuration management using ConfigAssist library
 * @details Manages web interface, WiFi settings, and coordinates all 
 *          configuration-related operations through ConfigAssist
 * @author Your Name
 * @date 2024-10-20
 * @version 2.1.0
 */
```

#### Class Documentation
```cpp
/**
 * @class MQTTManager
 * @brief Static class for MQTT connectivity and message handling
 * @details Implements MQTT client functionality with TLS support, automatic
 *          reconnection, and ISA-95 compliant topic hierarchy. Uses the
 *          256dpi/MQTT library for ESP32 compatibility.
 * 
 * @note This is a static class - do not attempt to instantiate
 * @warning Requires WiFi connection before initialization
 * 
 * @see TemperatureController for integration points
 * @see LoggerManager for MQTT history logging
 */
class MQTTManager {
```

#### Method Documentation
```cpp
/**
 * @brief Publishes temperature data for all configured measurement points
 * @details Collects temperature readings from all active points and publishes
 *          them as a JSON message to the telemetry topic. Only includes
 *          configured points with bound sensors.
 * 
 * @param controller Reference to the temperature controller
 * @return true if publish was successful
 * @return false if publish failed or MQTT not connected
 * 
 * @note Only publishes configured points, not all 60 slots
 * @warning Large payloads may exceed MQTT broker limits
 * 
 * Example published JSON:
 * @code{.json}
 * {
 *   "timestamp": 1706543210123,
 *   "device_id": "tempcontroller01",
 *   "points": {
 *     "0": {"name": "Tank 1", "temp": 65.5, "status": "OK"}
 *   }
 * }
 * @endcode
 */
static bool publishTemperatureData(TemperatureController& controller);
```

#### Variable Documentation
```cpp
/**
 * @brief MQTT client configuration loaded from /config/mqtt.json
 * @details Contains broker connection settings, authentication credentials,
 *          and topic hierarchy configuration
 */
static MQTTConfig config;

/**
 * @brief Temperature telemetry publish interval in milliseconds
 * @note Configurable range: 10000-3600000 ms (10s to 1 hour)
 */
static unsigned long publishIntervalMs;
```

#### Enum Documentation
```cpp
/**
 * @enum AlarmStage
 * @brief Defines the stages of the alarm state machine
 */
enum AlarmStage {
    ALARM_STAGE_NORMAL,      ///< Temperature within acceptable range
    ALARM_STAGE_WARNING,     ///< Temperature approaching threshold
    ALARM_STAGE_ALARM,       ///< Temperature exceeded threshold
    ALARM_STAGE_CRITICAL     ///< Temperature critically high/low
};
```

### Code Comments

#### Implementation Comments
- Use `//` for single-line implementation comments
- Place comments above the code they describe
- Explain WHY, not WHAT

```cpp
// Use QoS 1 for alarm messages to ensure delivery
// Critical alarms must not be lost due to network issues
if (isAlarmMessage) {
    qos = 1;
}

// Batch temperature updates to reduce network traffic
// Publishing 60 individual messages would overload the broker
JsonDocument doc;
for (int i = 0; i < 60; i++) {
    if (points[i].isConfigured()) {
        doc["points"][String(i)] = points[i].toJson();
    }
}
```

## C++ Coding Standards

### Naming Conventions

#### Classes and Structs
- **PascalCase** for class names: `ConfigManager`, `MQTTManager`
- Descriptive names that indicate purpose

#### Methods and Functions
- **camelCase** for methods: `publishTemperatureData()`, `validateConfiguration()`
- Verb-based names that describe action
- Boolean methods should ask a question: `isConnected()`, `hasValidSensor()`

#### Variables
- **camelCase** for variables: `lastPublishTime`, `sensorCount`
- **SCREAMING_SNAKE_CASE** for constants: `MAX_SENSORS`, `DEFAULT_TIMEOUT`
- Member variables with descriptive names (no `m_` prefix in this codebase)

#### File Names
- **PascalCase** for C++ files: `ConfigManager.cpp`, `MQTTManager.h`
- **lowercase** for web files: `index.html`, `settings-mqtt.html`

### Code Organization

#### Header Files
```cpp
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

// System includes first
#include <Arduino.h>
#include <WebServer.h>

// Library includes
#include <ConfigAssist.h>
#include <ArduinoJson.h>

// Project includes
#include "TemperatureController.h"

// Forward declarations
class LoggerManager;

class ConfigManager {
    // Public interface first
public:
    ConfigManager(TemperatureController& controller);
    void begin();
    
    // Grouped by functionality
    // Configuration methods
    String getWifiSSID();
    bool isModbusEnabled();
    
    // API endpoints
    void handleStatusRequest();
    void handleConfigUpdate();
    
private:
    // Private members last
    ConfigAssist conf;
    WebServer* server;
};

#endif // CONFIG_MANAGER_H
```

### Memory Management

#### Static Allocation Preferred
```cpp
// GOOD: Static allocation
StaticJsonDocument<2048> doc;

// AVOID: Dynamic allocation
DynamicJsonDocument doc(2048);
```

#### RAII Pattern
```cpp
// Use RAII for resource management
class FileHandler {
    File file;
public:
    FileHandler(const char* path) : file(SD.open(path)) {}
    ~FileHandler() { if (file) file.close(); }
};
```

### Error Handling

#### Return Status Codes
```cpp
enum class MQTTStatus {
    SUCCESS,
    CONNECTION_FAILED,
    PUBLISH_FAILED,
    INVALID_CONFIG
};

MQTTStatus publishMessage(const String& topic, const String& payload) {
    if (!isConnected()) {
        return MQTTStatus::CONNECTION_FAILED;
    }
    // ...
}
```

#### Defensive Programming
```cpp
bool ConfigManager::updateSetting(const String& key, const String& value) {
    // Validate inputs
    if (key.isEmpty() || value.isEmpty()) {
        Serial.println("[CONFIG] Error: Empty key or value");
        return false;
    }
    
    // Range check
    if (key == "device_id") {
        int id = value.toInt();
        if (id < 1 || id > 9999) {
            Serial.println("[CONFIG] Error: Device ID out of range");
            return false;
        }
    }
    
    // Update configuration
    conf[key] = value;
    return true;
}
```

## Web Development Standards

### HTML Structure
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title data-i18n="page.title">Temperature Monitor</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <!-- Semantic HTML5 elements -->
    <header>
        <h1 data-i18n="header.title">Temperature Controller</h1>
    </header>
    
    <main>
        <!-- Content sections with clear IDs -->
        <section id="status-panel">
            <!-- ... -->
        </section>
    </main>
    
    <footer>
        <p data-i18n="footer.copyright">© 2024</p>
    </footer>
    
    <!-- Scripts at end of body -->
    <script src="app.js"></script>
</body>
</html>
```

### JavaScript Standards
```javascript
/**
 * @class MQTTHistoryViewer
 * @description Handles client-side parsing and display of MQTT log files
 */
class MQTTHistoryViewer {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.currentFilter = {};
    }
    
    /**
     * Loads available log files from the server
     * @returns {Promise<void>}
     */
    async loadLogFiles() {
        try {
            const response = await fetch('/api/mqtt/logs');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            this.logFiles = await response.json();
            this.renderFileList();
        } catch (error) {
            console.error('Failed to load log files:', error);
            this.showError('Failed to load log files');
        }
    }
}
```

### CSS Organization
```css
/* Component-based organization */
/* Base styles */
* {
    box-sizing: border-box;
}

body {
    font-family: Arial, sans-serif;
    line-height: 1.6;
    color: #333;
}

/* Layout components */
.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 0 20px;
}

/* Feature-specific styles */
.mqtt-status {
    /* MQTT status indicator styles */
}

.mqtt-status--connected {
    color: #28a745;
}

.mqtt-status--disconnected {
    color: #dc3545;
}
```

## Testing Standards

### Serial Debug Output
```cpp
// Use consistent prefix for module identification
Serial.println("[MQTT] Connected to broker");
Serial.printf("[MQTT] Publishing to topic: %s\n", topic.c_str());
Serial.printf("[CONFIG] Device ID changed to: %d\n", deviceId);
```

### Performance Monitoring
```cpp
// Measure critical operations
unsigned long startTime = millis();
publishTemperatureData(controller);
unsigned long elapsed = millis() - startTime;

if (elapsed > 100) {
    Serial.printf("[MQTT] Warning: Publish took %lu ms\n", elapsed);
}
```

## Git Commit Standards

### Commit Message Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types
- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Code style changes
- **refactor**: Code refactoring
- **test**: Test additions/changes
- **chore**: Build process or auxiliary tool changes

### Examples
```
feat(mqtt): Add temperature telemetry publishing

- Implement publishTemperatureData() method
- Add 60-second publish interval
- Include only configured points in payload
- Integrate with MQTT history logging

Closes #123

feat(mqtt): Add alarm state change notifications

- Hook into Alarm::setState() method
- Publish to alarm/state_change topic
- Use QoS 1 for reliable delivery
- Include all alarm context in message

Part of MQTT integration epic
```

## Code Review Checklist

Before submitting code for review:

1. **Documentation**
   - [ ] All new files have Doxygen file headers
   - [ ] All new classes have comprehensive Doxygen comments
   - [ ] All public methods have Doxygen documentation
   - [ ] Complex logic has explanatory comments

2. **Code Quality**
   - [ ] No compiler warnings
   - [ ] Consistent naming conventions
   - [ ] Error handling for all external operations
   - [ ] Memory usage is optimized

3. **Testing**
   - [ ] Manual testing completed per story requirements
   - [ ] Serial debug output is informative
   - [ ] Performance impact measured

4. **Integration**
   - [ ] Changes don't break existing functionality
   - [ ] New features can be disabled/enabled
   - [ ] Configuration changes are backward compatible

Remember: **Well-commented code is a requirement, not a nice-to-have!**