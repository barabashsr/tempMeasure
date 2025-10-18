# Temperature Controller Development Planning Results

## Overview

This document provides a comprehensive step-by-step implementation plan for enhancing the Temperature Controller system based on requirements analysis from `main_requirements.md` and `DEVELOPMENT_BRIEF.md`.

## Priority Matrix

### Critical Issues (Must Fix)
1. **Hysteresis Implementation** - Critical for alarm stability
2. **Indicator Interface Logic** - Display/LED inconsistencies causing user confusion
3. **Alarm Configuration UX** - Current modal approach needs complete redesign

### High Priority Features  
4. **Modbus Safety Mechanisms** - Explicit triggers for configuration writes
5. **Single Button Navigation** - Improve user interaction consistency
6. **Circular Display Scrolling** - Better text display behavior

### Medium Priority Enhancements
7. **Temperature Trend Charts** - Modal-based charts with log data
8. **Relay Modbus Control** - RTU-accessible relay management
9. **Russian Translation** - Complete UI localization

## Detailed Implementation Plan

### Phase 1: Core Alarm System Fixes (Priority: Critical)

#### Step 1.1: Implement Hysteresis in Alarm Class
**Target File**: `include/Alarm.h`, `src/Alarm.cpp`

**Implementation Details**:
```cpp
// In Alarm class, add hysteresis support
private:
    int16_t _hysteresis;  // Already exists
    
// Modify _checkCondition() method
bool Alarm::_checkCondition() {
    if (!_source) return false;
    
    float currentTemp = _source->getCurrentTemperature();
    float threshold = (_type == AlarmType::HIGH_TEMPERATURE) ? 
        _source->getHighThreshold() : _source->getLowThreshold();
    
    bool conditionMet = false;
    
    if (_type == AlarmType::HIGH_TEMPERATURE) {
        if (_stage == AlarmStage::NEW || _stage == AlarmStage::CLEARED) {
            conditionMet = (currentTemp > threshold + _hysteresis);
        } else if (_stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED) {
            conditionMet = (currentTemp > threshold);
        }
    } else if (_type == AlarmType::LOW_TEMPERATURE) {
        if (_stage == AlarmStage::NEW || _stage == AlarmStage::CLEARED) {
            conditionMet = (currentTemp < threshold - _hysteresis);
        } else if (_stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED) {
            conditionMet = (currentTemp < threshold);
        }
    }
    
    return conditionMet;
}
```

**Testing**:
- Unit test with temperature values around thresholds
- Verify alarm doesn't oscillate with ±1°C temperature variations
- Test with different hysteresis values (1°C, 2°C, 5°C)

**Debug Output**:
```cpp
Serial.printf("HYSTERESIS: Point %d, Temp: %.1f, Threshold: %.1f, Hysteresis: %d, Condition: %s\n",
    _source->getAddress(), currentTemp, threshold, _hysteresis, conditionMet ? "MET" : "NOT_MET");
```

#### Step 1.2: Fix Indicator Interface Logic  
**Target File**: `src/IndicatorInterface.cpp`

**Issues to Fix**:
1. Inconsistent LED blinking patterns
2. Display state management problems  
3. Button handling reliability

**Implementation**:
```cpp
// Add state machine for display management
enum class DisplayState {
    NORMAL,           // Normal operation display
    ALARM_ACTIVE,     // Showing active alarms
    ALARM_ACK,        // Showing acknowledged alarms  
    SYSTEM_STATUS,    // System status mode (long press)
    OK_DISPLAY        // Showing OK after acknowledgment
};

// In IndicatorInterface class
private:
    DisplayState _currentDisplayState;
    unsigned long _displayStateTime;
    
// Centralized LED control method
void IndicatorInterface::updateLEDStates(AlarmPriority highestPriority, bool hasAcknowledged) {
    // Clear all LEDs first
    writePort("RED_LED", false);
    writePort("YELLOW_LED", false);
    writePort("BLUE_LED", false);
    
    switch(highestPriority) {
        case AlarmPriority::PRIORITY_CRITICAL:
            writePort("RED_LED", true);
            writePort("YELLOW_LED", true);
            break;
        case AlarmPriority::PRIORITY_HIGH:
            writePort("RED_LED", !hasAcknowledged);
            writePort("YELLOW_LED", true);
            break;
        case AlarmPriority::PRIORITY_MEDIUM:
            if (!hasAcknowledged) {
                startBlinking("YELLOW_LED", 2000, 30000);
            } else {
                stopBlinking("YELLOW_LED");
                writePort("YELLOW_LED", false);
            }
            break;
        case AlarmPriority::PRIORITY_LOW:
            // No LED indication for low priority
            break;
    }
}
```

**Testing**:
- Test all priority levels with different alarm states
- Verify LED states match requirements table
- Test button press handling reliability

#### Step 1.3: Redesign Alarm Configuration Web Interface
**Target File**: `data/alarms.html`

**Current Problem**: Modal-based approach shows one alarm at a time
**Solution**: Single comprehensive table showing all points

**Implementation**:
```html
<!-- Replace modal with comprehensive table -->
<div class="alarm-config-container">
    <table class="alarm-table">
        <thead>
            <tr>
                <th>Point</th>
                <th>Name</th>
                <th>Current Temp</th>
                <th>Low Threshold</th>
                <th>Low Priority</th>
                <th>High Threshold</th>
                <th>High Priority</th>
                <th>Error Priority</th>
                <th>Hysteresis</th>
                <th>Actions</th>
            </tr>
        </thead>
        <tbody id="alarmConfigTable">
            <!-- Dynamic rows populated by JavaScript -->
        </tbody>
    </table>
    <button id="applyAllChanges" class="btn-primary">Apply All Changes</button>
</div>
```

**JavaScript Implementation**:
```javascript
function loadAlarmConfiguration() {
    fetch('/api/alarm-config')
        .then(response => response.json())
        .then(data => {
            const tbody = document.getElementById('alarmConfigTable');
            tbody.innerHTML = '';
            
            data.points.forEach(point => {
                const row = createAlarmConfigRow(point);
                tbody.appendChild(row);
            });
        });
}

function createAlarmConfigRow(point) {
    const row = document.createElement('tr');
    row.innerHTML = `
        <td>${point.address}</td>
        <td><input type="text" value="${point.name}" data-field="name" data-point="${point.address}"></td>
        <td class="temp-display">${point.currentTemp}°C</td>
        <td><input type="number" value="${point.lowThreshold}" data-field="lowThreshold" data-point="${point.address}"></td>
        <td>
            <select data-field="lowPriority" data-point="${point.address}">
                <option value="0">Disabled</option>
                <option value="1">Low</option>
                <option value="2">Medium</option>
                <option value="3">High</option>
                <option value="4">Critical</option>
            </select>
        </td>
        <!-- Similar structure for other fields -->
    `;
    return row;
}
```

**API Endpoint Extensions Required**:
```cpp
// New/Modified API endpoints needed in main.cpp or separate API handler

// 1. Enhanced alarm configuration endpoint
server.on("/api/alarm-config", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Return comprehensive alarm config for all points
    JsonDocument doc;
    JsonArray points = doc["points"].to<JsonArray>();
    
    for (int i = 0; i < 60; i++) {
        JsonObject point = points.add<JsonObject>();
        MeasurementPoint* mp = tempController.getMeasurementPoint(i);
        if (mp) {
            point["address"] = i;
            point["name"] = mp->getName();
            point["currentTemp"] = mp->getCurrentTemperature();
            point["lowThreshold"] = mp->getLowThreshold();
            point["highThreshold"] = mp->getHighThreshold();
            point["lowPriority"] = mp->getLowAlarmPriority();
            point["highPriority"] = mp->getHighAlarmPriority();
            point["errorPriority"] = mp->getErrorAlarmPriority();
            point["hysteresis"] = mp->getHysteresis();
            point["sensorBound"] = mp->getBoundSensor() != nullptr;
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});

// 2. Bulk alarm configuration update endpoint
server.on("/api/alarm-config", HTTP_POST, [](AsyncWebServerRequest *request) {}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Parse JSON body with all configuration changes
    JsonDocument doc;
    deserializeJson(doc, (char*)data);
    
    JsonArray changes = doc["changes"];
    for (JsonObject change : changes) {
        uint8_t pointAddress = change["address"];
        MeasurementPoint* mp = tempController.getMeasurementPoint(pointAddress);
        if (mp) {
            if (change.containsKey("name")) mp->setName(change["name"]);
            if (change.containsKey("lowThreshold")) mp->setLowThreshold(change["lowThreshold"]);
            if (change.containsKey("highThreshold")) mp->setHighThreshold(change["highThreshold"]);
            if (change.containsKey("lowPriority")) mp->setLowAlarmPriority(change["lowPriority"]);
            if (change.containsKey("highPriority")) mp->setHighAlarmPriority(change["highPriority"]);
            if (change.containsKey("errorPriority")) mp->setErrorAlarmPriority(change["errorPriority"]);
            if (change.containsKey("hysteresis")) mp->setHysteresis(change["hysteresis"]);
        }
    }
    
    // Save configuration and update register map
    tempController.saveConfiguration();
    tempController.applyConfigToRegisterMap();
    
    request->send(200, "application/json", "{\"status\":\"success\"}");
});

// 3. Temperature trend data endpoint
server.on("/api/trend-data/{pointAddress}", HTTP_GET, [](AsyncWebServerRequest *request) {
    String pointAddressStr = request->pathArg(0);
    uint8_t pointAddress = pointAddressStr.toInt();
    
    // Read last 24 hours from log file
    JsonDocument doc;
    JsonArray timestamps = doc["timestamps"].to<JsonArray>();
    JsonArray temperatures = doc["temperatures"].to<JsonArray>();
    JsonArray alarmEvents = doc["alarmEvents"].to<JsonArray>();
    JsonArray highThresholds = doc["highThresholds"].to<JsonArray>();
    JsonArray lowThresholds = doc["lowThresholds"].to<JsonArray>();
    
    // Read from temperature log file (last 288 entries = 24 hours at 5-min intervals)
    File logFile = LittleFS.open("/data/temperatures.csv", "r");
    if (logFile) {
        // Parse CSV and extract data for specific point
        // Implementation details in Step 3.1
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});

// 4. Enhanced sensor binding endpoint with dropdown data
server.on("/api/sensor-binding-options", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    
    // Available sensors
    JsonArray sensors = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < tempController.getSensorCount(); i++) {
        Sensor* sensor = tempController.getSensorByIndex(i);
        if (sensor) {
            JsonObject sensorObj = sensors.add<JsonObject>();
            sensorObj["rom"] = sensor->getRomString();
            sensorObj["type"] = sensor->getType();
            sensorObj["bus"] = tempController.getSensorBus(sensor);
            sensorObj["bound"] = sensor->getBoundPointAddress() != 255;
        }
    }
    
    // Available measurement points with names
    JsonArray points = doc["points"].to<JsonArray>();
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* mp = tempController.getMeasurementPoint(i);
        if (mp) {
            JsonObject pointObj = points.add<JsonObject>();
            pointObj["address"] = i;
            pointObj["name"] = mp->getName();
            pointObj["type"] = i < 50 ? "DS18B20" : "PT1000";
            pointObj["hasSensor"] = mp->getBoundSensor() != nullptr;
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});

// 5. Relay control endpoint
server.on("/api/relay-control", HTTP_POST, [](AsyncWebServerRequest *request) {}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, (char*)data);
    
    uint8_t relayNumber = doc["relay"];
    bool state = doc["state"];
    
    if (relayNumber >= 1 && relayNumber <= 3) {
        String relayPort = "RELAY" + String(relayNumber);
        indicator.writePort(relayPort, state);
        
        // Update register map for Modbus access
        tempController.getRegisterMap().setRelayState(relayNumber, state);
        
        LoggerManager::getInstance().logEvent("Manual relay control: Relay" + String(relayNumber) + " = " + (state ? "ON" : "OFF"));
    }
    
    request->send(200, "application/json", "{\"status\":\"success\"}");
});

// 6. Enhanced system status endpoint
server.on("/api/system-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    
    // Existing system info
    doc["deviceId"] = tempController.getDeviceId();
    doc["firmwareVersion"] = tempController.getFirmwareVersion();
    doc["activeDS18B20"] = tempController.getDS18B20Count();
    doc["activePT1000"] = tempController.getPT1000Count();
    
    // Enhanced alarm statistics
    doc["totalAlarms"] = tempController.getAlarmCount(AlarmStage::ACTIVE);
    doc["criticalAlarms"] = tempController.getAlarmCount(AlarmPriority::PRIORITY_CRITICAL, AlarmStage::ACTIVE);
    doc["highAlarms"] = tempController.getAlarmCount(AlarmPriority::PRIORITY_HIGH, AlarmStage::ACTIVE);
    doc["mediumAlarms"] = tempController.getAlarmCount(AlarmPriority::PRIORITY_MEDIUM, AlarmStage::ACTIVE);
    doc["lowAlarms"] = tempController.getAlarmCount(AlarmPriority::PRIORITY_LOW, AlarmStage::ACTIVE);
    doc["acknowledgedAlarms"] = tempController.getAlarmCount(AlarmStage::ACKNOWLEDGED);
    
    // Unbound sensor count
    int unboundSensors = 0;
    for (int i = 0; i < tempController.getSensorCount(); i++) {
        Sensor* sensor = tempController.getSensorByIndex(i);
        if (sensor && sensor->getBoundPointAddress() == 255) {
            unboundSensors++;
        }
    }
    doc["unboundSensors"] = unboundSensors;
    
    // Relay states
    JsonObject relays = doc["relays"].to<JsonObject>();
    relays["relay1"] = indicator.readPort("RELAY1");
    relays["relay2"] = indicator.readPort("RELAY2"); 
    relays["relay3"] = indicator.readPort("RELAY3");
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});

// 7. Language preference endpoint
server.on("/api/language", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Read saved language preference from configuration
    String language = configManager.getLanguage(); // Default: "en"
    request->send(200, "application/json", "{\"language\":\"" + language + "\"}");
});

server.on("/api/language", HTTP_POST, [](AsyncWebServerRequest *request) {}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, (char*)data);
    
    String language = doc["language"];
    if (language == "en" || language == "ru") {
        configManager.setLanguage(language);
        configManager.saveConfiguration();
        request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        request->send(400, "application/json", "{\"error\":\"Invalid language\"}");
    }
});
```

**Required Model Extensions**:
```cpp
// Extensions needed in MeasurementPoint class
class MeasurementPoint {
public:
    // Add these methods if not present
    void setLowAlarmPriority(AlarmPriority priority);
    void setHighAlarmPriority(AlarmPriority priority);
    void setErrorAlarmPriority(AlarmPriority priority);
    AlarmPriority getLowAlarmPriority() const;
    AlarmPriority getHighAlarmPriority() const;
    AlarmPriority getErrorAlarmPriority() const;
    
    void setHysteresis(int16_t hysteresis);
    int16_t getHysteresis() const;
    
    uint8_t getBoundPointAddress() const; // For sensors
};

// Extensions needed in Sensor class
class Sensor {
public:
    uint8_t getBoundPointAddress() const; // Returns 255 if unbound
    String getType() const; // "DS18B20" or "PT1000"
};
```

**Testing**:
- Load configuration for all 60 measurement points
- Test dropdown functionality for priority selection
- Verify changes are applied correctly via API
- Test input validation (threshold ranges, hysteresis values)
- Test trend data API with real log files
- Verify sensor binding dropdown population
- Test relay control API integration

### Phase 2: Modbus Safety and Button Enhancement (Priority: High)

#### Step 2.1: Implement Modbus Configuration Safety
**Target File**: `src/TempModbusServer.cpp`, `include/RegisterMap.h`

**Requirements**: 
- Configuration changes only apply when trigger register is written
- Prevent accidental configuration from startup values

**Implementation**:
```cpp
// Add trigger registers to RegisterMap
class RegisterMap {
private:
    static const uint16_t ALARM_CONFIG_TRIGGER = 899;
    static const uint16_t RELAY_CONFIG_TRIGGER = 999;
    bool _alarmConfigPending = false;
    bool _relayConfigPending = false;
    
public:
    bool isAlarmConfigTriggered() {
        return _alarmConfigPending;
    }
    
    void setAlarmConfigTrigger() {
        _alarmConfigPending = true;
    }
    
    void clearAlarmConfigTrigger() {
        _alarmConfigPending = false;
    }
};

// In TempModbusServer handling
void TempModbusServer::handleWriteRegister(uint16_t address, uint16_t value) {
    if (address == RegisterMap::ALARM_CONFIG_TRIGGER && value == 0xA5A5) {
        // Apply pending alarm configuration
        _controller->applyAlarmConfigFromRegisterMap();
        _registerMap->clearAlarmConfigTrigger();
        LoggerManager::getInstance().logEvent("Modbus alarm config applied");
    }
    // Handle other registers...
}
```

**Testing**:
- Write alarm configuration to registers 800-898
- Verify changes don't apply until trigger register is written
- Test with invalid trigger values (should be ignored)
- Test configuration rollback if trigger not sent

#### Step 2.2: Enhance Single Button Navigation
**Target File**: `src/IndicatorInterface.cpp`

**Requirements**:
- Short press: Acknowledge alarms, navigate pages
- Long press: Enter/exit system status mode
- Improve debouncing and state management

**Implementation**:
```cpp
// Button state machine
enum class ButtonState {
    IDLE,
    PRESSED,
    SHORT_PRESS_DETECTED,
    LONG_PRESS_DETECTED
};

class ButtonHandler {
private:
    ButtonState _state = ButtonState::IDLE;
    unsigned long _pressStartTime = 0;
    unsigned long _lastPressTime = 0;
    const unsigned long LONG_PRESS_THRESHOLD = 2000;  // 2 seconds
    const unsigned long DEBOUNCE_DELAY = 50;          // 50ms
    
public:
    void update(bool buttonPressed) {
        unsigned long currentTime = millis();
        
        switch(_state) {
            case ButtonState::IDLE:
                if (buttonPressed && (currentTime - _lastPressTime > DEBOUNCE_DELAY)) {
                    _state = ButtonState::PRESSED;
                    _pressStartTime = currentTime;
                }
                break;
                
            case ButtonState::PRESSED:
                if (!buttonPressed) {
                    unsigned long pressDuration = currentTime - _pressStartTime;
                    if (pressDuration < LONG_PRESS_THRESHOLD) {
                        _state = ButtonState::SHORT_PRESS_DETECTED;
                    }
                    _lastPressTime = currentTime;
                } else if (currentTime - _pressStartTime >= LONG_PRESS_THRESHOLD) {
                    _state = ButtonState::LONG_PRESS_DETECTED;
                }
                break;
        }
    }
    
    bool isShortPressDetected() {
        if (_state == ButtonState::SHORT_PRESS_DETECTED) {
            _state = ButtonState::IDLE;
            return true;
        }
        return false;
    }
    
    bool isLongPressDetected() {
        if (_state == ButtonState::LONG_PRESS_DETECTED) {
            _state = ButtonState::IDLE;
            return true;
        }
        return false;
    }
};
```

**Testing**:
- Test short press detection (< 2 seconds)
- Test long press detection (≥ 2 seconds)
- Verify debouncing works with rapid button presses
- Test navigation through different display modes

#### Step 2.3: Implement Circular Display Scrolling
**Target File**: `src/IndicatorInterface.cpp`

**Current Problem**: Text jumps back to beginning instead of circular scrolling
**Solution**: Implement circular scrolling with spacing

**Implementation**:
```cpp
void IndicatorInterface::_handleScrolling() {
    unsigned long currentTime = millis();
    
    if (currentTime - _lastScrollTime < _scrollDelay) {
        return;
    }
    
    for (int i = 0; i < _textBufferSize; i++) {
        String& line = _textBuffer[i];
        if (line.length() > _maxCharsPerLine) {
            // Calculate total scroll length (text + spacing)
            int totalLength = line.length() + 3; // 3 spaces after text
            
            // Increment scroll offset
            _scrollOffset[i] = (_scrollOffset[i] + 1) % totalLength;
            
            // If we've scrolled past the text, show spaces
            if (_scrollOffset[i] >= line.length()) {
                _scrollOffset[i] = (_scrollOffset[i] + 1) % totalLength;
            }
        }
    }
    
    _lastScrollTime = currentTime;
}

void IndicatorInterface::_drawTextLine(int lineIndex, int yPos) {
    if (lineIndex >= _textBufferSize) return;
    
    String& line = _textBuffer[lineIndex];
    
    if (line.length() <= _maxCharsPerLine) {
        // No scrolling needed
        u8g2.drawStr(0, yPos, line.c_str());
    } else {
        // Create scrolled string with circular behavior
        String displayText = "";
        int offset = _scrollOffset[lineIndex];
        
        for (int i = 0; i < _maxCharsPerLine; i++) {
            int charIndex = (offset + i) % (line.length() + 3);
            if (charIndex < line.length()) {
                displayText += line[charIndex];
            } else {
                displayText += " "; // Spacing between end and start
            }
        }
        
        u8g2.drawStr(0, yPos, displayText.c_str());
    }
}
```

**Testing**:
- Test with strings longer than 21 characters
- Verify circular scrolling behavior (text flows smoothly)
- Test with multiple lines scrolling simultaneously
- Verify performance with 5 lines of long text

### Phase 3: Feature Enhancements (Priority: Medium)

#### Step 3.1: Implement Temperature Trend Charts
**Target File**: `data/dashboard.html`

**Requirements**:
- Modal-based charts using Chart.js
- Data from log files
- Alarm event markers with tooltips

**Implementation**:
```html
<!-- Add modal to dashboard.html -->
<div id="trendModal" class="modal">
    <div class="modal-content">
        <span class="close">&times;</span>
        <h2 id="trendTitle">Temperature Trend</h2>
        <canvas id="trendChart" width="800" height="400"></canvas>
    </div>
</div>
```

```javascript
function showTrendChart(pointAddress) {
    fetch(`/api/trend-data/${pointAddress}`)
        .then(response => response.json())
        .then(data => {
            const ctx = document.getElementById('trendChart').getContext('2d');
            
            const chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: data.timestamps,
                    datasets: [{
                        label: 'Temperature',
                        data: data.temperatures,
                        borderColor: '#2196F3',
                        backgroundColor: 'rgba(33, 150, 243, 0.1)',
                        tension: 0.1
                    }, {
                        label: 'High Threshold',
                        data: data.highThresholds,
                        borderColor: '#F44336',
                        borderDash: [5, 5],
                        pointRadius: 0
                    }, {
                        label: 'Low Threshold', 
                        data: data.lowThresholds,
                        borderColor: '#FF9800',
                        borderDash: [5, 5],
                        pointRadius: 0
                    }]
                },
                options: {
                    responsive: true,
                    plugins: {
                        tooltip: {
                            filter: function(tooltipItem) {
                                // Show alarm events in tooltips
                                return data.alarmEvents[tooltipItem.dataIndex];
                            },
                            callbacks: {
                                afterBody: function(context) {
                                    const index = context[0].dataIndex;
                                    const alarmEvent = data.alarmEvents[index];
                                    if (alarmEvent) {
                                        return `Alarm: ${alarmEvent.type} (${alarmEvent.priority})`;
                                    }
                                    return '';
                                }
                            }
                        }
                    }
                }
            });
            
            document.getElementById('trendModal').style.display = 'block';
        });
}
```

**Testing**:
- Test chart loading with real log data
- Verify alarm markers appear correctly
- Test modal open/close functionality
- Performance test with 24 hours of data (1440 points)

#### Step 3.2: Add Relay Modbus Control
**Target File**: `src/TempModbusServer.cpp`, `include/RegisterMap.h`

**Implementation**:
```cpp
// Add relay control registers
class RegisterMap {
public:
    static const uint16_t RELAY1_CONTROL = 950;
    static const uint16_t RELAY2_CONTROL = 951;
    static const uint16_t RELAY3_CONTROL = 952;
    static const uint16_t RELAY_TRIGGER = 999;
    
    void setRelayState(uint8_t relayNumber, bool state) {
        // Implementation to control relays via IndicatorInterface
    }
};
```

**Testing**:
- Test individual relay control via Modbus
- Verify relay states are read back correctly
- Test safety trigger mechanism
- Test interaction with alarm-driven relay control

#### Step 3.3: Russian Translation
**Target Files**: All HTML files in `data/` directory

**Implementation**:
```javascript
// Add to each HTML file
const translations = {
    en: {
        "dashboard": "Dashboard",
        "temperature": "Temperature", 
        "alarms": "Alarms",
        "points": "Points",
        "system": "System",
        "current": "Current",
        "minimum": "Minimum",
        "maximum": "Maximum",
        "threshold": "Threshold",
        "priority": "Priority",
        "hysteresis": "Hysteresis"
    },
    ru: {
        "dashboard": "Панель управления",
        "temperature": "Температура",
        "alarms": "Аварии", 
        "points": "Точки",
        "system": "Система",
        "current": "Текущая",
        "minimum": "Минимум",
        "maximum": "Максимум", 
        "threshold": "Порог",
        "priority": "Приоритет",
        "hysteresis": "Гистерезис"
    }
};

function setLanguage(lang) {
    document.querySelectorAll('[data-translate]').forEach(element => {
        const key = element.getAttribute('data-translate');
        if (translations[lang] && translations[lang][key]) {
            element.textContent = translations[lang][key];
        }
    });
    localStorage.setItem('language', lang);
}
```

**Testing**:
- Test language switching on all pages
- Verify all UI elements are translated
- Test language persistence across page reloads

## Unit Testing Strategy

### Test Categories

#### 1. Alarm System Tests
```cpp
// Test hysteresis behavior
void test_alarm_hysteresis() {
    Alarm alarm(AlarmType::HIGH_TEMPERATURE, &mockPoint, AlarmPriority::PRIORITY_HIGH);
    alarm.setHysteresis(20); // 2.0°C
    
    // Test threshold = 50°C, hysteresis = 2°C
    mockPoint.setCurrentTemperature(51.5); // Below trigger (50 + 2)
    assert(!alarm.updateCondition()); // Should not trigger
    
    mockPoint.setCurrentTemperature(52.5); // Above trigger (50 + 2)  
    assert(alarm.updateCondition()); // Should trigger
    
    mockPoint.setCurrentTemperature(50.5); // Above threshold but below hysteresis
    assert(alarm.isActive()); // Should remain active
    
    mockPoint.setCurrentTemperature(49.5); // Below threshold
    assert(!alarm.isActive()); // Should clear
}
```

#### 2. Button Handling Tests
```cpp
void test_button_short_press() {
    ButtonHandler handler;
    
    // Simulate button press for 100ms
    handler.update(true);
    delay(100);
    handler.update(false);
    
    assert(handler.isShortPressDetected());
    assert(!handler.isLongPressDetected());
}

void test_button_long_press() {
    ButtonHandler handler;
    
    // Simulate button press for 2.5 seconds
    handler.update(true);
    delay(2500);
    handler.update(false);
    
    assert(handler.isLongPressDetected());
    assert(!handler.isShortPressDetected());
}
```

#### 3. Display Scrolling Tests
```cpp
void test_circular_scrolling() {
    IndicatorInterface indicator(Wire, 0x20, -1);
    
    indicator.pushLine("This is a very long text that exceeds display width");
    
    // Test multiple scroll iterations
    for (int i = 0; i < 50; i++) {
        indicator.updateOLED();
        delay(200);
        // Verify scroll offset wraps around correctly
    }
}
```

### Debug Output Strategy

#### 1. Alarm Debug Messages
```cpp
#define ALARM_DEBUG 1

#if ALARM_DEBUG
#define ALARM_LOG(fmt, ...) Serial.printf("[ALARM] " fmt "\n", ##__VA_ARGS__)
#else
#define ALARM_LOG(fmt, ...)
#endif

// Usage in alarm code:
ALARM_LOG("Point %d: Temp=%.1f, Threshold=%.1f, Hysteresis=%d, State=%s", 
    pointAddress, temperature, threshold, hysteresis, stateString);
```

#### 2. Button Debug Messages
```cpp
#define BUTTON_DEBUG 1

#if BUTTON_DEBUG  
#define BUTTON_LOG(fmt, ...) Serial.printf("[BUTTON] " fmt "\n", ##__VA_ARGS__)
#else
#define BUTTON_LOG(fmt, ...)
#endif

// Usage:
BUTTON_LOG("Press detected: duration=%dms, type=%s", duration, pressType);
```

#### 3. Display Debug Messages
```cpp
#define DISPLAY_DEBUG 1

#if DISPLAY_DEBUG
#define DISPLAY_LOG(fmt, ...) Serial.printf("[DISPLAY] " fmt "\n", ##__VA_ARGS__)
#else  
#define DISPLAY_LOG(fmt, ...)
#endif

// Usage:
DISPLAY_LOG("Scroll: line=%d, offset=%d, text='%s'", lineIndex, offset, displayText.c_str());
```

## Risk Assessment and Dependencies

### High Risk Items

#### 1. **Memory Constraints**
- **Risk**: Chart.js and trend data may exceed ESP32 memory limits
- **Mitigation**: Implement data pagination, limit chart data points to 288 (5-minute intervals for 24 hours)
- **Fallback**: Simple text-based trend display if Chart.js fails

#### 2. **Performance Impact**
- **Risk**: Circular scrolling may impact display refresh rate
- **Mitigation**: Optimize scroll calculations, limit text processing per frame
- **Testing**: Monitor performance with 5 long lines scrolling simultaneously

#### 3. **ConfigAssist Integration**
- **Risk**: Changes to alarm configuration interface may break ConfigAssist file handling
- **Mitigation**: Preserve existing CSV structure, add new fields as optional columns
- **Testing**: Verify configuration persistence and loading after changes

### Medium Risk Items

#### 4. **Modbus Timing**
- **Risk**: Explicit trigger mechanism may cause timing issues with SCADA systems  
- **Mitigation**: Implement timeout for trigger register (auto-apply after 30 seconds)
- **Documentation**: Clear Modbus integration guide with timing requirements

#### 5. **Russian Font Support**
- **Risk**: U8g2 library may not support Cyrillic characters
- **Mitigation**: Use Unicode font or English fallback for OLED display
- **Alternative**: Implement custom character mapping for essential Russian text

### Dependencies

#### External Libraries
- **Chart.js**: For trend charts (web interface)
- **U8g2**: Display library (no changes needed)
- **PCF8575**: I/O expander (preserve existing logic)
- **ConfigAssist**: Configuration management (minimal changes)

#### Hardware Dependencies
- **Pin definitions**: Must remain unchanged (hardware constraint)
- **I2C addresses**: PCF8575 (0x20), OLED (auto-detect)
- **Button debouncing**: Physical button characteristics may require timing adjustments

## Implementation Timeline

### Week 1: Core Alarm System
- Day 1-2: Implement hysteresis in Alarm class
- Day 3-4: Fix indicator interface logic and LED control  
- Day 5-6: Redesign alarm configuration web interface (HTML/CSS/JS)
- Day 7: Implement API endpoints for alarm configuration

### Week 2: Safety and Navigation  
- Day 1-2: Implement Modbus configuration safety
- Day 3-4: Enhance button handling with state machine
- Day 5-6: Implement circular display scrolling
- Day 7: Implement sensor binding API endpoints with dropdown support

### Week 3: Feature Enhancements
- Day 1-2: Implement temperature trend charts (HTML/JS + Chart.js)
- Day 3: Implement trend data API endpoint
- Day 4: Add relay Modbus control + API endpoints
- Day 5: Implement system status API enhancements
- Day 6-7: Russian translation and language API

### Week 4: Testing and Documentation
- Day 1-3: Comprehensive testing of all features
- Day 4-5: Performance optimization and bug fixes  
- Day 6-7: Update documentation and user manual

## Success Criteria

### Functional Requirements
✅ **Hysteresis prevents alarm oscillation** (±1°C temperature variations)
✅ **LED indicators match specification table** for all priority levels  
✅ **Single alarm configuration page** shows all points simultaneously
✅ **Modbus configuration requires explicit trigger** to prevent accidental changes
✅ **Button navigation works reliably** with proper debouncing
✅ **Display scrolling is circular** with smooth text flow

### Performance Requirements  
✅ **Display refresh rate ≥ 10 FPS** with scrolling text
✅ **Button response time < 100ms** for acknowledgment
✅ **Web interface loads < 3 seconds** on local network
✅ **Trend charts render < 5 seconds** with 24 hours of data
✅ **API response time < 500ms** for configuration endpoints
✅ **Bulk configuration update < 2 seconds** for all 60 points

### Quality Requirements
✅ **Zero regression** in existing functionality  
✅ **Configuration persistence** survives power cycles
✅ **Memory usage < 80%** of available ESP32 memory
✅ **All user-facing text** available in English and Russian

---

## Next Steps

After planning approval:
1. Create `claude-branch` from current branch if it doesn't exist
2. Begin Phase 1 implementation starting with hysteresis
3. Implement comprehensive testing after each major component
4. Build and test incrementally to catch issues early
5. Document changes in commit messages and update relevant documentation files

**Ready for implementation phase upon approval.**