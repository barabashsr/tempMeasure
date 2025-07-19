#include "TemperatureController.h"

TemperatureController::TemperatureController(uint8_t oneWirePin[4], uint8_t csPin[4], IndicatorInterface& indicator)
: indicator(indicator), 
measurementPeriodSeconds(10), 
deviceId(1), 
firmwareVersion(0x0100),
lastMeasurementTime(0), 
systemInitialized(false), 
_lastAlarmCheck(0),
_lastButtonState(false), 
_lastButtonPressTime(0), 
_currentDisplayedAlarm(nullptr),
_okDisplayStartTime(0), 
_showingOK(false),

_currentActiveAlarmIndex(0), _currentAcknowledgedAlarmIndex(0),
_lastAlarmDisplayTime(0), _acknowledgedAlarmDisplayDelay(5000), // 5 seconds
_displayingActiveAlarm(false)
{
    // Initialize measurement points
    for (uint8_t i = 0; i < 50; ++i)
        dsPoints[i] = MeasurementPoint(i, "DS18B20_Point_" + String(i));
    for (uint8_t i = 0; i < 10; ++i)
        ptPoints[i] = MeasurementPoint(50 + i, "PT1000_Point_" + String(i));
    
    // Initialize bus pins
    for (uint8_t i = 0; i < 4; i++) {
        oneWireBusPin[i] = oneWirePin[i];
        chipSelectPin[i] = csPin[i];
    }
    
    // Initialize OneWire buses
    for (int i = 0; i < 4; ++i) {
        oneWireBuses[i] = new OneWire(oneWireBusPin[i]);
        dallasSensors[i] = new DallasTemperature(oneWireBuses[i]);
    }
}

TemperatureController::~TemperatureController() {
    // Clean up sensors
    for (auto sensor : sensors)
        delete sensor;
    sensors.clear();
    
 
    
    // Clean up OneWire buses
    for (int i = 0; i < 4; ++i) {
        delete dallasSensors[i];
        delete oneWireBuses[i];
    }

    // Clean up configured alarms
    for (auto alarm : _configuredAlarms)
        delete alarm;
    _configuredAlarms.clear();
}

bool TemperatureController::begin() {
    // Initialize register map
    registerMap.writeHoldingRegister(0, deviceId);
    registerMap.writeHoldingRegister(1, firmwareVersion);
    registerMap.writeHoldingRegister(2, 0);
    registerMap.writeHoldingRegister(3, 0);
    for (int i = 4; i <= 10; i++)
        registerMap.writeHoldingRegister(i, 0);
    
    Serial.println("Discovering sensors...");
    discoverPTSensors();
    Serial.println("Setting HMI...");
    
    // Initialize indicator interface
    if (!indicator.begin()) {
        Serial.println("Failed to initialize indicator interface!");
        return false;
    }
    
    // Configure ports
    indicator.setDirection(0b0000000011111111); // P0-P7 as outputs
    
    // Set port names
    indicator.setPortName("BUTTON", 15);
    indicator.setPortName("Relay1", 0);
    indicator.setPortName("Relay2", 1);
    indicator.setPortName("Relay3", 2);
    indicator.setPortName("GreenLED", 4);
    indicator.setPortName("BlueLED", 5);
    indicator.setPortName("YellowLED", 6);
    indicator.setPortName("RedLED", 7);
    
    // Set individual port inversion for ULN2803
    indicator.setPortInverted("Relay1", false);
    indicator.setPortInverted("Relay2", false);
    indicator.setPortInverted("Relay3", false);
    indicator.setPortInverted("GreenLED", false);
    indicator.setPortInverted("BlueLED", false);
    indicator.setPortInverted("YellowLED", false);
    indicator.setPortInverted("RedLED", false);
    indicator.setPortInverted("BUTTON", false);
    
    // Turn off all LEDs initially
    indicator.setAllOutputsLow();
    
    // Set interrupt callback
    indicator.setInterruptCallback([](uint16_t currentState, uint16_t changedPins) {
        Serial.print("PCF8575 Interrupt - State: 0x");
        Serial.print(currentState, HEX);
        Serial.print(", Changed: 0x");
        Serial.println(changedPins, HEX);
    });
    
    // Set normal operation display
    indicator.setOledMode(3);
    indicator.writePort("GreenLED", true); // Normal operation LED
    
    systemInitialized = true;
    Serial.println("Setup complete!");
    indicator.printConfiguration();
    LoggerManager::info("SYSTEM", 
        "TemperatureController started");
    return true;
}

void TemperatureController::update() {
    updateAllSensors();
    readAllPoints();
    
    // Handle PCF8575 interrupts
    indicator.handleInterrupt();
    
    // Update alarm system
    updateAlarms();
    
    // Handle button presses
    _checkButtonPress();
    
    // Handle alarm display and outputs
    handleAlarmDisplay();
    handleAlarmOutputs();
    
    // Update OLED
    indicator.update();
    
    if (!systemInitialized) return;


    unsigned long currentTime = millis();
    if (currentTime - lastMeasurementTime >= measurementPeriodSeconds * 1000) {
        
        updateRegisterMap();
        lastMeasurementTime = currentTime;
        //applyConfigFromRegisterMap();
    }
}

// Alarm Management Methods
void TemperatureController::updateAlarms() {
    unsigned long currentTime = millis();
    if (currentTime - _lastAlarmCheck < _alarmCheckInterval) {
        return;
    }
    _lastAlarmCheck = currentTime;
    
    Serial.println("=== Checking alarms with fresh sensor data ===");
    
    // Check all measurement points for NEW alarm conditions
    for (uint8_t i = 0; i < 50; ++i) {
        if (dsPoints[i].getBoundSensor() != nullptr) {
            Serial.printf("DS Point %d: Temp=%d, High=%d, Low=%d\n",
                         i, dsPoints[i].getCurrentTemp(),
                         dsPoints[i].getHighAlarmThreshold(),
                         dsPoints[i].getLowAlarmThreshold());
            //_checkPointForAlarms(&dsPoints[i]);
        }
    }
    
    for (uint8_t i = 0; i < 10; ++i) {
        if (ptPoints[i].getBoundSensor() != nullptr) {
            Serial.printf("PT Point %d: Temp=%d, High=%d, Low=%d\n",
                         i, ptPoints[i].getCurrentTemp(),
                         ptPoints[i].getHighAlarmThreshold(),
                         ptPoints[i].getLowAlarmThreshold());
            //_checkPointForAlarms(&ptPoints[i]);
        }
    }
    
    // Update existing configured alarms (do NOT remove resolved alarms)
    Serial.println("=== Updating existing alarms ===");
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled()) {
            alarm->updateCondition();
            // Do NOT check for resolved state or remove alarms here
        }
    }
    
    // Sort alarms by priority
    std::sort(_configuredAlarms.begin(), _configuredAlarms.end(), AlarmComparator());
    
    Serial.printf("Active alarms count: %d\n", getActiveAlarms().size());
    
    // Debug: Print all current alarms
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled()) {
            Serial.printf("  Alarm: %s, Stage: %s, Point: %d\n",
                         alarm->getTypeString().c_str(),
                         alarm->getStageString().c_str(),
                         alarm->getSource() ? alarm->getSource()->getAddress() : -1);
        }
    }
}




void TemperatureController::_checkPointForAlarms(MeasurementPoint* point) {
    if (!point || !point->getBoundSensor()) return;
    
    // Check for high temperature alarm
    if (point->getCurrentTemp() >= point->getHighAlarmThreshold()) {
        if (!_hasAlarmForPoint(point, AlarmType::HIGH_TEMPERATURE)) {
            createAlarm(AlarmType::HIGH_TEMPERATURE, point, AlarmPriority::PRIORITY_HIGH);
        }
    }
    
    // Check for low temperature alarm
    if (point->getCurrentTemp() <= point->getLowAlarmThreshold()) {
        if (!_hasAlarmForPoint(point, AlarmType::LOW_TEMPERATURE)) {
            createAlarm(AlarmType::LOW_TEMPERATURE, point, AlarmPriority::PRIORITY_MEDIUM);
        }
    }
    
    // Check for sensor error
    if (point->getErrorStatus() != 0) {
        if (!_hasAlarmForPoint(point, AlarmType::SENSOR_ERROR)) {
            createAlarm(AlarmType::SENSOR_ERROR, point, AlarmPriority::PRIORITY_HIGH);
        }
    }
}

bool TemperatureController::_hasAlarmForPoint(MeasurementPoint* point, AlarmType type) {
    for (auto alarm : _configuredAlarms) {
        if (alarm->getSource() == point && 
            alarm->getType() == type && 
            alarm->isEnabled() &&
            alarm->isActive()) {
            return true;
        }
    }
    return false;
}


void TemperatureController::createAlarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority) {
    // Check if this alarm already exists in configured alarms
    String configKey = "alarm_" + String(source->getAddress()) + "_" + String(static_cast<int>(type));
    
    for (auto alarm : _configuredAlarms) {
        if (alarm->getConfigKey() == configKey) {
            // Alarm already exists, just enable it if it's disabled
            if (!alarm->isEnabled()) {
                alarm->setEnabled(true);
                alarm->setStage(AlarmStage::NEW); // Reset stage
            }
            return;
        }
    }
    
    // Create new alarm and add to configured alarms
    Alarm* newAlarm = new Alarm(type, source, priority);
    newAlarm->setConfigKey(configKey);
    _configuredAlarms.push_back(newAlarm);
    
    // Sort alarms by priority
    std::sort(_configuredAlarms.begin(), _configuredAlarms.end(), AlarmComparator());
}


Alarm* TemperatureController::getHighestPriorityAlarm() const {
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && alarm->isActive()) {
            return alarm;
        }
    }
    return nullptr;
}


void TemperatureController::acknowledgeHighestPriorityAlarm() {
    Alarm* alarm = getHighestPriorityAlarm();
    if (alarm) {
        alarm->acknowledge();
        Serial.printf("Alarm acknowledged: %s\n", alarm->getStatusText().c_str());
    }
}

void TemperatureController::acknowledgeAllAlarms() {
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && alarm->isActive() && !alarm->isAcknowledged()) {
            alarm->acknowledge();
        }
    }
}


std::vector<Alarm*> TemperatureController::getActiveAlarms() const {
    std::vector<Alarm*> activeAlarms;
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && alarm->isActive()) {
            activeAlarms.push_back(alarm);
        }
    }
    return activeAlarms;
}

void TemperatureController::clearResolvedAlarms() {
    for (auto it = _configuredAlarms.begin(); it != _configuredAlarms.end();) {
        if ((*it)->isResolved()) {
            if (_currentDisplayedAlarm == *it) {
                _currentDisplayedAlarm = nullptr;
            }
            Serial.printf("Manually clearing resolved alarm: %s\n", (*it)->getConfigKey().c_str());
            delete *it;
            it = _configuredAlarms.erase(it);
        } else {
            ++it;
        }
    }
}

void TemperatureController::clearConfiguredAlarms() {
    for (auto it = _configuredAlarms.begin(); it != _configuredAlarms.end();) {
            if (_currentDisplayedAlarm == *it) {
                _currentDisplayedAlarm = nullptr;
            }
            Serial.printf("Manually clearing resolved alarm: %s\n", (*it)->getConfigKey().c_str());
            delete *it;
            it = _configuredAlarms.erase(it);
    }
}



// void TemperatureController::handleAlarmDisplay() {
//     Alarm* highestPriorityAlarm = getHighestPriorityAlarm();
    
//     if (highestPriorityAlarm) {
//         // Display alarm
//         _currentDisplayedAlarm = highestPriorityAlarm;
//         _showingOK = false;
        
//         indicator.setOledMode(2);
//         String displayText = highestPriorityAlarm->getDisplayText();
        
//         // Split display text into lines
//         int newlineIndex = displayText.indexOf('\n');
//         String line1 = displayText.substring(0, newlineIndex);
//         String line2 = displayText.substring(newlineIndex + 1);
        
//         String displayLines[2] = {line1, line2};
//         indicator.printText(displayLines, 2);
        
//     } else if (_currentDisplayedAlarm && !_showingOK) {
//         // No more alarms, show OK for 1 minute
//         _showOKAndTurnOffOLED();
        
//     } else if (_showingOK) {
//         // Check if OK display time has elapsed
//         if (millis() - _okDisplayStartTime >= 60000) { // 1 minute
//             indicator.setOLEDOff();
//             _showingOK = false;
//             _currentDisplayedAlarm = nullptr;
//         }
        
//     } else {
//         // Normal operation - show normal display
//         _updateNormalDisplay();
//     }
// }

// void TemperatureController::handleAlarmOutputs() {
//     Alarm* highestPriorityAlarm = getHighestPriorityAlarm();
    
//     if (highestPriorityAlarm) {
//         // Handle alarm outputs based on type and stage
//         if (highestPriorityAlarm->getType() == AlarmType::HIGH_TEMPERATURE) {
//             // High temperature alarm
//             indicator.writePort("RedLED", true);
//             indicator.writePort("GreenLED", false);
            
//             if (!highestPriorityAlarm->isAcknowledged()) {
//                 indicator.writePort("Relay1", true);
//             } else {
//                 indicator.writePort("Relay1", false);
//             }
//             indicator.writePort("Relay2", true);
//         }
//         // Add other alarm type handling here
        
//     } else {
//         // No active alarms - normal operation
//         indicator.writePort("GreenLED", true);
//         indicator.writePort("RedLED", false);
//         indicator.writePort("Relay1", false);
//         indicator.writePort("Relay2", false);
//     }
// }
void TemperatureController::handleAlarmOutputs() {
    // Get alarm counts by priority and stage for precise control
    int criticalActive = getAlarmCount(AlarmPriority::PRIORITY_CRITICAL, AlarmStage::ACTIVE);
    int criticalAcknowledged = getAlarmCount(AlarmPriority::PRIORITY_CRITICAL, AlarmStage::ACKNOWLEDGED);
    int highActive = getAlarmCount(AlarmPriority::PRIORITY_HIGH, AlarmStage::ACTIVE);
    int highAcknowledged = getAlarmCount(AlarmPriority::PRIORITY_HIGH, AlarmStage::ACKNOWLEDGED);
    int mediumActive = getAlarmCount(AlarmPriority::PRIORITY_MEDIUM, AlarmStage::ACTIVE);
    int mediumAcknowledged = getAlarmCount(AlarmPriority::PRIORITY_MEDIUM, AlarmStage::ACKNOWLEDGED);
    int lowActive = getAlarmCount(AlarmPriority::PRIORITY_LOW, AlarmStage::ACTIVE);
    int lowAcknowledged = getAlarmCount(AlarmPriority::PRIORITY_LOW, AlarmStage::ACKNOWLEDGED);
    
    // Determine highest priority alarm state for output control
    bool hasCritical = (criticalActive + criticalAcknowledged) > 0;
    bool hasHigh = (highActive + highAcknowledged) > 0;
    bool hasMedium = (mediumActive + mediumAcknowledged) > 0;
    bool hasLow = (lowActive + lowAcknowledged) > 0;
    
    // Determine if alarms are acknowledged
    bool criticalAcknowledgedOnly = (criticalActive == 0 && criticalAcknowledged > 0);
    bool highAcknowledgedOnly = (highActive == 0 && highAcknowledged > 0);
    bool mediumAcknowledgedOnly = (mediumActive == 0 && mediumAcknowledged > 0);
    
    // Apply alarm notification logic based on priority and acknowledgment state
    // Following the specification table from PLANNING_RESULTS.md
    
    bool relay1State = false; // Siren
    bool relay2State = false; // Flash beacon
    bool redLedState = false;
    bool yellowLedState = false;
    bool blueLedState = false;
    
    // Stop all blinking first - we'll restart as needed
    indicator.stopBlinking("Relay2");
    indicator.stopBlinking("YellowLED");
    
    if (hasCritical) {
        if (criticalAcknowledgedOnly) {
            // CRITICAL acknowledged: Beacon ON (constant)
            relay2State = true;
        } else {
            // CRITICAL active: Siren ON + Beacon ON (constant)
            relay1State = true;
            relay2State = true;
        }
        // Red LED always on for critical alarms
        redLedState = true;
        yellowLedState = true; // Both LEDs for critical
    } else if (hasHigh) {
        if (highAcknowledgedOnly) {
            // HIGH acknowledged: Beacon ON (blink 2s on/30s off)
            indicator.startBlinking("Relay2", 2000, 30000);
        } else {
            // HIGH active: Beacon ON (constant)
            relay2State = true;
        }
        // Red LED for high priority (when not acknowledged)
        redLedState = !highAcknowledgedOnly;
        yellowLedState = true;
    } else if (hasMedium) {
        if (!mediumAcknowledgedOnly) {
            // MEDIUM active: Beacon ON (blink 2s on/30s off)
            indicator.startBlinking("Relay2", 2000, 30000);
        }
        // MEDIUM acknowledged: Beacon OFF (no relay action)
        // Yellow LED blinking for medium priority
        if (!mediumAcknowledgedOnly) {
            indicator.startBlinking("YellowLED", 2000, 30000);
        }
    } else if (hasLow) {
        // LOW priority: No relay action (as per specification)
        // No LED indication for low priority
    }
    
    // Log alarm summary periodically
    static unsigned long lastSummaryLog = 0;
    unsigned long now = millis();
    if (now - lastSummaryLog > 30000 || // Log every 30 seconds
        (hasCritical || hasHigh || hasMedium) && now - lastSummaryLog > 5000) { // Or every 5 seconds if significant alarms
        
        String summary = "Alarm summary - Critical: " + String(criticalActive) + "/" + String(criticalAcknowledged) + 
                        ", High: " + String(highActive) + "/" + String(highAcknowledged) + 
                        ", Medium: " + String(mediumActive) + "/" + String(mediumAcknowledged) + 
                        ", Low: " + String(lowActive) + "/" + String(lowAcknowledged);
        LoggerManager::info("ALARM_OUTPUT", summary);
        lastSummaryLog = now;
    }
    
    // Update relay states (only if not blinking)
    if (!indicator.isBlinking("Relay1") && relay1State != _relay1State) {
        LoggerManager::info("INDICATION", 
            "Relay1 (Siren) state change: " + String(_relay1State ? "ON" : "OFF") + 
            " -> " + String(relay1State ? "ON" : "OFF"));
        indicator.writePort("Relay1", relay1State);
        _relay1State = relay1State;
    }
    
    if (!indicator.isBlinking("Relay2") && relay2State != _relay2State) {
        LoggerManager::info("INDICATION", 
            "Relay2 (Beacon) state change: " + String(_relay2State ? "ON" : "OFF") + 
            " -> " + String(relay2State ? "ON" : "OFF"));
        indicator.writePort("Relay2", relay2State);
        _relay2State = relay2State;
    }
    
    // Update LED states (only if not blinking)
    if (redLedState != _redLedState) {
        LoggerManager::info("INDICATION", 
            "Red LED state change: " + String(_redLedState ? "ON" : "OFF") + 
            " -> " + String(redLedState ? "ON" : "OFF"));
        indicator.writePort("RedLED", redLedState);
        _redLedState = redLedState;
    }
    
    if (!indicator.isBlinking("YellowLED") && yellowLedState != _yellowLedState) {
        LoggerManager::info("INDICATION", 
            "Yellow LED state change: " + String(_yellowLedState ? "ON" : "OFF") + 
            " -> " + String(yellowLedState ? "ON" : "OFF"));
        indicator.writePort("YellowLED", yellowLedState);
        _yellowLedState = yellowLedState;
    }
    
    if (blueLedState != _blueLedState) {
        LoggerManager::info("INDICATION", 
            "Blue LED state change: " + String(_blueLedState ? "ON" : "OFF") + 
            " -> " + String(blueLedState ? "ON" : "OFF"));
        indicator.writePort("BlueLED", blueLedState);
        _blueLedState = blueLedState;
    }
}


// void TemperatureController::handleAlarmOutputs() {
//     // Calculate new output states using enhanced getAlarmCount methods
//     bool newRelay1 = getAlarmCount(AlarmStage::ACTIVE) > 0;
    
//     // HIGH or CRITICAL priority alarms in ACKNOWLEDGED or ACTIVE states
//     bool highPriorityRelay2 = getAlarmCount(AlarmPriority::PRIORITY_HIGH, AlarmStage::ACKNOWLEDGED, ">=", ">=") > 0;
    
//     // LOW priority alarms in ACKNOWLEDGED or ACTIVE states (for blinking)
//     bool lowPriorityExists = getAlarmCount(AlarmPriority::PRIORITY_LOW, AlarmStage::ACKNOWLEDGED, "==", ">=") > 0;
    
//     // CRITICAL priority alarms in ACKNOWLEDGED or ACTIVE states  
//     bool newRedLed = getAlarmCount(AlarmPriority::PRIORITY_CRITICAL, AlarmStage::ACKNOWLEDGED, "==", ">=") > 0;
    
//     // HIGH priority alarms in ACKNOWLEDGED or ACTIVE states
//     bool newYellowLed = getAlarmCount(AlarmPriority::PRIORITY_HIGH, AlarmStage::ACKNOWLEDGED, "==", ">=") > 0;
    
//     // MEDIUM priority alarms in ACKNOWLEDGED or ACTIVE states
//     bool mediumPriorityBlueLed = getAlarmCount(AlarmPriority::PRIORITY_MEDIUM, AlarmStage::ACKNOWLEDGED, "==", ">=") > 0;
    
//     // Handle blinking for low priority alarms
//     if (lowPriorityExists) {
//         // Start blinking if not already blinking
//         if (!highPriorityRelay2 && !indicator.isBlinking("Relay2")) {
//             indicator.startBlinking("Relay2", 1000, 5000);  // 2s on, 30s off
            

//         }
        
//         if (!indicator.isBlinking("BlueLED")) {
//             indicator.startBlinking("BlueLED", 500, 500);    // 500ms on, 500ms off
//         }
//     } else {
//         // Stop blinking if no low priority alarms
//         indicator.stopBlinking("Relay2");
//         indicator.stopBlinking("BlueLED");
//     }
    
//     // Calculate final states (excluding blinking ports)
//     bool newRelay2 = highPriorityRelay2;  // Don't set if blinking
//     bool newBlueLed = mediumPriorityBlueLed;  // Don't set if blinking
    
//     // Only update non-blinking outputs if state has changed
//     if (newRelay1 != _relay1State) {
//         indicator.writePort("Relay1", newRelay1);
//         _relay1State = newRelay1;
//     }
    
//     // Only control Relay2 directly if not blinking for low priority
//     if (!indicator.isBlinking("Relay2") && newRelay2 != _relay2State) {
//         indicator.writePort("Relay2", newRelay2);
//         _relay2State = newRelay2;
//     }
    
//     if (newRedLed != _redLedState) {
//         indicator.writePort("RedLED", newRedLed);
//         _redLedState = newRedLed;
//     }
    
//     if (newYellowLed != _yellowLedState) {
//         indicator.writePort("YellowLED", newYellowLed);
//         _yellowLedState = newYellowLed;
//     }
    
//     // Only control BlueLED directly if not blinking for low priority
//     if (!indicator.isBlinking("BlueLED") && newBlueLed != _blueLedState) {
//         indicator.writePort("BlueLED", newBlueLed);
//         _blueLedState = newBlueLed;
//     }
// }




// void TemperatureController::_checkButtonPress() {
//     bool currentButtonState = indicator.readPort("BUTTON");
    
//     // Detect button press (HIGH to LOW transition)
//     if (_lastButtonState == true && currentButtonState == false) {
//         if ((millis() - _lastButtonPressTime) > _buttonDebounceDelay) {
//             Serial.println("BUTTON PRESS DETECTED!");
//             acknowledgeHighestPriorityAlarm();
//             _lastButtonPressTime = millis();
//         }
//     }
    
//     _lastButtonState = currentButtonState;
// }

void TemperatureController::_updateNormalDisplay() {
    // Show normal system status
    indicator.setOledMode(3);
    String lines[3] = {
        "System Normal",
        "Temp Monitor",
        "Ready"
    };
    indicator.printText(lines, 3);
}

void TemperatureController::_showOKAndTurnOffOLED() {
    indicator.displayOK();
    _okDisplayStartTime = millis();
    _showingOK = true;
}

String TemperatureController::getAlarmsJson() {
    DynamicJsonDocument doc(4096);
    JsonArray alarmArray = doc.createNestedArray("alarms");
    
    for (auto alarm : _configuredAlarms) {
        JsonObject obj = alarmArray.createNestedObject();
        obj["configKey"] = alarm->getConfigKey();
        obj["type"] = static_cast<int>(alarm->getType());
        obj["priority"] = static_cast<int>(alarm->getPriority());
        obj["enabled"] = alarm->isEnabled();
        obj["pointAddress"] = alarm->getPointAddress();
        obj["stage"] = static_cast<int>(alarm->getStage());
        obj["isActive"] = alarm->isActive();
        obj["isAcknowledged"] = alarm->isAcknowledged();
        obj["timestamp"] = alarm->getTimestamp();
        obj["acknowledgedTime"] = alarm->getAcknowledgedTime();
        obj["acknowledgedTimeLeft"] = alarm->getAcknowledgedTimeLeft();
        
        if (alarm->getSource()) {
            obj["pointName"] = alarm->getSource()->getName();
            obj["currentTemp"] = alarm->getSource()->getCurrentTemp();
            obj["threshold"] = (alarm->getType() == AlarmType::HIGH_TEMPERATURE)
                ? alarm->getSource()->getHighAlarmThreshold()
                : alarm->getSource()->getLowAlarmThreshold();
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}


// void TemperatureController::update() {
//     updateAllSensors();
    
//     // Add this line - handle PCF8575 interrupts
//     indicator.handleInterrupt();
    
//     // Handle alarm logic
    
    
//     // Update OLED
//     indicator.updateOLED();
    
//     if (!systemInitialized) return;
    
//     unsigned long currentTime = millis();
//     if (currentTime - lastMeasurementTime >= measurementPeriodSeconds) {
//         readAllPoints();
//         updateRegisterMap();
//         lastMeasurementTime = currentTime;
//         applyConfigFromRegisterMap();
//     }
// }



MeasurementPoint* TemperatureController::getMeasurementPoint(uint8_t address) {
    if (isDS18B20Address(address))
        return &dsPoints[address];
    if (isPT1000Address(address))
        return &ptPoints[address - 50];
    return nullptr;
}

MeasurementPoint* TemperatureController::getDS18B20Point(uint8_t idx) {
    return (idx < 50) ? &dsPoints[idx] : nullptr;
}

MeasurementPoint* TemperatureController::getPT1000Point(uint8_t idx) {
    return (idx < 10) ? &ptPoints[idx] : nullptr;
}

bool TemperatureController::addSensor(Sensor* sensor) {
    if (!sensor) return false;
    // For DS18B20, check by ROM string
    if (sensor->getType() == SensorType::DS18B20) {
        String romStr = sensor->getDS18B20RomString();
        if (findSensorByRom(romStr)) return false;
    } else if (sensor->getType() == SensorType::PT1000) {
        if (findSensorByChipSelect(sensor->getPT1000ChipSelectPin())) return false;
    }
    sensors.push_back(sensor);
    if (sensor->getType() == SensorType::DS18B20)
        registerMap.incrementActiveDS18B20();
    else
        registerMap.incrementActivePT1000();
    return true;
}

bool TemperatureController::removeSensorByRom(const String& romString) {
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        if ((*it)->getType() == SensorType::DS18B20 &&
            (*it)->getDS18B20RomString() == romString) {
            // Unbind from any point
            for (uint8_t i = 0; i < 50; ++i) {
                if (dsPoints[i].getBoundSensor() == *it)
                    dsPoints[i].unbindSensor();
            }
            registerMap.decrementActiveDS18B20();
            delete *it;
            sensors.erase(it);
            return true;
        }
    }
    return false;
}

Sensor* TemperatureController::findSensorByRom(const String& romString) {
    for (auto s : sensors) {
            Serial.println("Sensor ROM" + s->getDS18B20RomString());
        if (s->getType() == SensorType::DS18B20 &&
            s->getDS18B20RomString() == romString)
            return s;
    }
    return nullptr;
}

Sensor* TemperatureController::findSensorByChipSelect(uint8_t csPin) {
    for (auto s : sensors)
        if (s->getType() == SensorType::PT1000 &&
            s->getPT1000ChipSelectPin() == csPin)
            return s;
    return nullptr;
}

Sensor* TemperatureController::getSensorByIndex(int idx) {
    return (idx >= 0 && idx < sensors.size()) ? sensors[idx] : nullptr;
}

bool TemperatureController::bindSensorToPointByRom(const String& romString, uint8_t pointAddress) {
    if(pointAddress > 49) return false;
    Sensor* sensor = findSensorByRom(romString);
    unbindSensorFromPointBySensor(sensor);
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!sensor || !point){
        LoggerManager::warning("BINDING", 
            "Failed to bind sensor " + romString + " to point " + String(pointAddress));
        
        return false;} 

        point->bindSensor(sensor);
    


        String pointName = point->getName().isEmpty() ? 
            "Point_" + String(pointAddress) : point->getName();
        LoggerManager::info("BINDING", 
            "Sensor " + romString + " bound to point " + String(pointAddress) + 
            " (" + pointName + ")");




    return true;
}

bool TemperatureController::bindSensorToPointByChipSelect(uint8_t csPin, uint8_t pointAddress) {
    Serial.printf("Point address: %d\n", pointAddress);
    if((pointAddress < 50) || (pointAddress > 59)) 
        return false;
    Serial.printf("Point address: %d PASSED!\n", pointAddress);
    Sensor* sensor = findSensorByChipSelect(csPin);
    unbindSensorFromPointBySensor(sensor);
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!sensor || !point) {
        
        LoggerManager::warning("BINDING", 
            "Failed to bind PT1000 sensor CS" + String(csPin) + 
            " to point " + String(pointAddress));
        return false;}
    point->bindSensor(sensor);
    String pointName = point->getName().isEmpty() ? 
            "Point_" + String(pointAddress) : point->getName();
        LoggerManager::info("BINDING", 
            "PT1000 sensor CS" + String(csPin) + " bound to point " + 
            String(pointAddress) + " (" + pointName + ")");
    return true;
}

bool TemperatureController::unbindSensorFromPoint(uint8_t pointAddress) {
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!point) {
        LoggerManager::error("BINDING", 
            "Faild to unbound sensor from point " + String(pointAddress) + 
            " (" + point->getName() + ")");
        return false;
    }

    if (point->getBoundSensor()) {
        String sensorInfo = point->getBoundSensor()->getType() == SensorType::DS18B20 ? 
        point->getBoundSensor()->getDS18B20RomString() : 
            "CS" + String(point->getBoundSensor()->getPT1000ChipSelectPin());
        
        LoggerManager::info("BINDING", 
            "Sensor " + sensorInfo + " unbound from point " + String(pointAddress) + 
            " (" + point->getName() + ")");
    }

    point->unbindSensor();
    return true;
}

Sensor* TemperatureController::getBoundSensor(uint8_t pointAddress) {
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    return point ? point->getBoundSensor() : nullptr;
}

void TemperatureController::readAllPoints() {
    for (uint8_t i = 0; i < 50; ++i)
        dsPoints[i].update();
    for (uint8_t i = 0; i < 10; ++i)
        ptPoints[i].update();
}

void TemperatureController::updateRegisterMap() {
    for (uint8_t i = 0; i < 50; ++i)
        registerMap.updateFromMeasurementPoint(dsPoints[i]);
    for (uint8_t i = 0; i < 10; ++i)
        registerMap.updateFromMeasurementPoint(ptPoints[i]);
}

void TemperatureController::applyConfigFromRegisterMap() {
    for (uint8_t i = 0; i < 50; ++i)
        registerMap.applyConfigToMeasurementPoint(dsPoints[i]);
    for (uint8_t i = 0; i < 10; ++i)
        registerMap.applyConfigToMeasurementPoint(ptPoints[i]);
}

void TemperatureController::applyConfigToRegisterMap() {
    for (uint8_t i = 0; i < 50; ++i)
        registerMap.applyConfigFromMeasurementPoint(dsPoints[i]);
    for (uint8_t i = 0; i < 10; ++i)
        registerMap.applyConfigFromMeasurementPoint(ptPoints[i]);
}

bool TemperatureController::discoverDS18B20Sensors() {
    bool anyAdded = false;
    Serial.println("Discover method started...");
    LoggerManager::info("DISCOVERY", "Starting DS18B20 sensor discovery");
    uint totalFound = 0;
    // OneWire oneWire[] = { OneWire(oneWireBusPin[0]), OneWire(oneWireBusPin[1]), OneWire(oneWireBusPin[2]), OneWire(oneWireBusPin[3]) };
    // DallasTemperature dallasSensors[] = {DallasTemperature(&oneWire[0]), DallasTemperature(&oneWire[1]), DallasTemperature(&oneWire[2]), DallasTemperature(&oneWire[3])};
    for (uint j = 0; j < 4; j++){
        Serial.printf("Discover bus %d pin %d started...\n", j, oneWireBusPin[j]);
        
    //OneWire oneWire(oneWireBusPin[j]);
    
    dallasSensors[j]->begin();

    int deviceCount = dallasSensors[j]->getDeviceCount();
    Serial.printf("Devices on bus %d: %d\n", j, deviceCount);
    if (deviceCount == 0) continue;
    totalFound += deviceCount;

    DeviceAddress sensorAddress;
    if (deviceCount > 0) {
        LoggerManager::info("DISCOVERY", 
            "Found " + String(deviceCount) + " DS18B20 sensors on bus " + String(j));
        }
    

    for (int i = 0; i < deviceCount; i++) {
        if (dallasSensors[j]->getAddress(sensorAddress, i)) {
            Serial.printf("Bus %d. Device %d of %d\n", j, i, deviceCount);
            // Convert ROM to string for uniqueness
            char buf[17];
            for (int u = 0; u < 8; ++u) sprintf(buf + u*2, "%02X", sensorAddress[u]);
            String romString(buf);
            Serial.printf("ROM: %s\n", romString);

            // Check if already exists
            if (findSensorByRom(romString)){
                if(getSensorBus(findSensorByRom(romString)) != j) {
                    removeSensorByRom(romString);
                    Serial.println("Device existed on enother bus. Deleting");
                    //continue;

                } else continue;

            }
             

            String sensorName = "DS18B20_" + romString;
            Sensor* newSensor = new Sensor(SensorType::DS18B20, 0, sensorName); // address field not used for DS
            Serial.printf("Sensor created with name %s on bus %d\n", newSensor->getName(), getSensorBus(newSensor));

            newSensor->setupDS18B20(oneWireBusPin[j], sensorAddress);
            Serial.printf("Sensor %s set on bus %d/ pin %d\n", newSensor->getName(), getSensorBus(newSensor), newSensor->getOneWirePin());

            if (newSensor->initialize()) {
                sensors.push_back(newSensor);
                registerMap.incrementActiveDS18B20();
                anyAdded = true;
                Serial.printf("Sensor %s set on bus %d/ pin %d status: Connected\n", newSensor->getName(), getSensorBus(newSensor), newSensor->getOneWirePin());
                

            } else {
                delete newSensor;
            }
        }
    }
    }

    LoggerManager::info("DISCOVERY", 
        "DS18B20 discovery completed. Total sensors: " + String(totalFound));

    return anyAdded;
}


bool TemperatureController::discoverPTSensors() {
    bool anyAdded = false;
    Serial.println("Discover PT method started...");
    LoggerManager::info("DISCOVERY", "Starting PT1000 sensor discovery");
    for (uint j = 0; j < 4; j++){
        Serial.printf("Bus: %d: PIN: %d", j, chipSelectPin[j]);
    }
    // OneWire oneWire[] = { OneWire(oneWireBusPin[0]), OneWire(oneWireBusPin[1]), OneWire(oneWireBusPin[2]), OneWire(oneWireBusPin[3]) };
    // DallasTemperature dallasSensors[] = {DallasTemperature(&oneWire[0]), DallasTemperature(&oneWire[1]), DallasTemperature(&oneWire[2]), DallasTemperature(&oneWire[3])};
    for (uint j = 0; j < 4; j++){
        Serial.printf("Discover PT: bus %d pin %d started...\n", j, chipSelectPin[j]);
        

        if(findSensorByChipSelect(chipSelectPin[j]) != nullptr){
            Serial.printf("Sensor already discovered on bus %d\n", j);
            continue;

        }
  
        
             

            String sensorName = "PT1000_" + String(j);
            Sensor* newSensor = new Sensor(SensorType::PT1000, j, sensorName); // address field not used for DS
            Serial.printf("Sensor created with name %s on bus %d\n", newSensor->getName(), getSensorBus(newSensor));

            newSensor->setupPT1000(chipSelectPin[j], j);
            Serial.printf("Sensor %s set on bus %d/ pin %d\n", newSensor->getName(), getSensorBus(newSensor), newSensor->getPT1000ChipSelectPin());
            

            if (newSensor->initialize()) {

                


                sensors.push_back(newSensor);
                registerMap.incrementActivePT1000();
                anyAdded = true;
                Serial.printf("Sensor %s set on bus %d/ pin %d status: Connected\n", newSensor->getName(), getSensorBus(newSensor), newSensor->getPT1000ChipSelectPin());
                LoggerManager::info("DISCOVERY", 
                    "Added PT1000 sensor on bus: " + String(getSensorBus(newSensor)) + ", CS pin: " + String(newSensor->getPT1000ChipSelectPin()));

            } else {
                delete newSensor;
            }
        
    
        }
    return anyAdded;
}


String TemperatureController::getSensorsJson() {
    DynamicJsonDocument doc(8192);
    JsonArray sensorArray = doc.createNestedArray("sensors");

    for (auto sensor : sensors) {
        JsonObject obj = sensorArray.createNestedObject();
        obj["type"] = (sensor->getType() == SensorType::DS18B20) ? "DS18B20" : "PT1000";
        obj["name"] = sensor->getName();
        obj["currentTemp"] = sensor->getCurrentTemp();
        obj["minTemp"] = sensor->getMinTemp();
        obj["maxTemp"] = sensor->getMaxTemp();
        obj["lowAlarmThreshold"] = sensor->getLowAlarmThreshold();
        obj["highAlarmThreshold"] = sensor->getHighAlarmThreshold();
        obj["alarmStatus"] = sensor->getAlarmStatus();
        obj["errorStatus"] = sensor->getErrorStatus();
        obj["bus"] = getSensorBus(sensor);

        if (sensor->getType() == SensorType::DS18B20) {
            obj["romString"] = sensor->getDS18B20RomString();
            JsonArray romArr = obj.createNestedArray("romArray");
            uint8_t rom[8];
            sensor->getDS18B20RomArray(rom);
            for (int j = 0; j < 8; ++j) romArr.add(rom[j]);
            
        } else if (sensor->getType() == SensorType::PT1000) {
            obj["chipSelectPin"] = sensor->getPT1000ChipSelectPin();
        }

        // Binding info
        int boundPoint = -1;
        if (sensor->getType() == SensorType::DS18B20) {
            String romString = sensor->getDS18B20RomString();
            for (uint8_t i = 0; i < 50; ++i) {
                Sensor* bound = dsPoints[i].getBoundSensor();
                if (bound && bound->getType() == SensorType::DS18B20 &&
                    bound->getDS18B20RomString() == romString) {
                    boundPoint = dsPoints[i].getAddress();
                    break;
                }
            }
        } else if (sensor->getType() == SensorType::PT1000) {
            for (uint8_t i = 0; i < 10; ++i) {
                Sensor* bound = ptPoints[i].getBoundSensor();
                if (bound && bound == sensor) {
                    boundPoint = ptPoints[i].getAddress();
                    break;
                }
            }
        }
        if (boundPoint >= 0) obj["boundPoint"] = boundPoint;
        else obj["boundPoint"] = nullptr;
    }

    String out;
    serializeJson(doc, out);
    return out;
}

String TemperatureController::getPointsJson() {
    DynamicJsonDocument doc(8192);
    JsonArray pointsArray = doc.createNestedArray("points");

    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) { //Should be 50 instad of 2 here
        MeasurementPoint& point = dsPoints[i];
        JsonObject obj = pointsArray.createNestedObject();
        obj["address"] = point.getAddress();
        obj["name"] = point.getName();
        obj["type"] = "DS18B20";
        obj["currentTemp"] = point.getCurrentTemp();
        obj["minTemp"] = point.getMinTemp();
        obj["maxTemp"] = point.getMaxTemp();
        obj["lowAlarmThreshold"] = point.getLowAlarmThreshold();
        obj["highAlarmThreshold"] = point.getHighAlarmThreshold();
        obj["alarmStatus"] = point.getAlarmStatus();
        obj["errorStatus"] = point.getErrorStatus();
        

        Sensor* bound = point.getBoundSensor();
        if (bound && bound->getType() == SensorType::DS18B20) {
            obj["sensorType"] = "DS18B20";
            obj["sensorRomString"] = bound->getDS18B20RomString();
            JsonArray romArr = obj.createNestedArray("sensorRomArray");
            uint8_t rom[8];
            bound->getDS18B20RomArray(rom);
            for (int j = 0; j < 8; ++j) romArr.add(rom[j]);
            obj["bus"] = getSensorBus(bound);
            
        }
    }

    // PT1000 points
    for (uint8_t i = 0; i < 10; ++i) { //should be 10 instaed of 1
        MeasurementPoint& point = ptPoints[i];
        JsonObject obj = pointsArray.createNestedObject();
        obj["address"] = point.getAddress();
        obj["name"] = point.getName();
        obj["type"] = "PT1000";
        obj["currentTemp"] = point.getCurrentTemp();
        obj["minTemp"] = point.getMinTemp();
        obj["maxTemp"] = point.getMaxTemp();
        obj["lowAlarmThreshold"] = point.getLowAlarmThreshold();
        obj["highAlarmThreshold"] = point.getHighAlarmThreshold();
        obj["alarmStatus"] = point.getAlarmStatus();
        obj["errorStatus"] = point.getErrorStatus();

        Sensor* bound = point.getBoundSensor();
        if (bound && bound->getType() == SensorType::PT1000) {
            obj["sensorType"] = "PT1000";
            obj["chipSelectPin"] = bound->getPT1000ChipSelectPin();
            obj["bus"] = getSensorBus(bound);
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

String TemperatureController::getSystemStatusJson() {
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = deviceId;
    doc["firmwareVersion"] = firmwareVersion;
    doc["ds18b20Count"] = getDS18B20Count();
    doc["pt1000Count"] = getPT1000Count();
    doc["measurementPeriod"] = measurementPeriodSeconds;
    doc["uptime"] = millis() / 1000;

    JsonArray statusArray = doc.createNestedArray("deviceStatus");
    for (int i = 4; i <= 10; i++) {
        statusArray.add(registerMap.readHoldingRegister(i));
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void TemperatureController::resetMinMaxValues() {
    LoggerManager::info("SYSTEM", "Min/Max temperature values reset");
    for (uint8_t i = 0; i < 50; ++i)
        dsPoints[i].resetMinMaxTemp();
    for (uint8_t i = 0; i < 10; ++i)
        ptPoints[i].resetMinMaxTemp();
}

void TemperatureController::setDeviceId(uint16_t id) {
    uint16_t oldId = deviceId;
    deviceId = id;
    registerMap.writeHoldingRegister(0, deviceId);
    LoggerManager::info("CONFIG", 
        "Device ID changed from " + String(oldId) + " to " + String(deviceId));
}

uint16_t TemperatureController::getDeviceId() const { return deviceId; }

void TemperatureController::setFirmwareVersion(uint16_t version) {
    firmwareVersion = version;
    registerMap.writeHoldingRegister(1, firmwareVersion);
}

uint16_t TemperatureController::getFirmwareVersion() const { return firmwareVersion; }

void TemperatureController::setMeasurementPeriod(uint16_t seconds) {
    measurementPeriodSeconds = seconds;
    if (measurementPeriodSeconds != seconds) {
        uint16_t oldPeriod = measurementPeriodSeconds;
        measurementPeriodSeconds = seconds;
        LoggerManager::info("CONFIG", 
            "Measurement period changed from " + String(oldPeriod) + 
            "s to " + String(measurementPeriodSeconds) + "s");
    }
}

uint16_t TemperatureController::getMeasurementPeriod() const {
    return measurementPeriodSeconds;
}

void TemperatureController::setOneWireBusPin(uint8_t pin, size_t bus) {
    oneWireBusPin[bus] = pin;
}

int TemperatureController::getDS18B20Count() const {
    int count = 0;
    for (auto sensor : sensors) {
        if (sensor->getType() == SensorType::DS18B20) {
            count++;
        }
    }
    return count;
}

int TemperatureController::getPT1000Count() const {
    int count = 0;
    for (auto sensor : sensors) {
        if (sensor->getType() == SensorType::PT1000) {
            count++;
        }
    }
    return count;
}

void TemperatureController::updateAllSensors() {
    for (auto sensor : sensors) {
        sensor->readTemperature();
    }

    for (auto& sensor : sensors) {
        if (sensor->getErrorStatus() != 0) {
            static std::map<Sensor*, unsigned long> lastErrorLog;
            unsigned long now = millis();
            
            // Only log errors every 5 minutes to avoid spam
            if (lastErrorLog[sensor] == 0 || (now - lastErrorLog[sensor]) > 300000) {
                String sensorId = sensor->getType() == SensorType::DS18B20 ? 
                    sensor->getDS18B20RomString() : 
                    "BUS " + String(getSensorBus(sensor));
                
                LoggerManager::error("SENSOR", 
                    "Sensor error detected: " + sensorId + 
                    " (Error code: " + String(sensor->getErrorStatus()) + ")");
                
                lastErrorLog[sensor] = now;
            }
        }
    }
}

uint8_t TemperatureController::getOneWirePin(size_t bus) {
    return oneWireBusPin[bus];
}

int TemperatureController::getSensorBus(Sensor* sensor) {
    if (sensor->getType() == SensorType::DS18B20){
        uint8_t pin = sensor->getOneWirePin();
        for (int i = 0; i < 4; i++) {
            if (oneWireBusPin[i] == pin) return i;
        } 
    } else if(sensor->getType() == SensorType::PT1000) {
        uint8_t pin = sensor->getPT1000ChipSelectPin();
        for (int i = 0; i < 4; i++) {
            if (chipSelectPin[i] == pin) return i;
        } 
    }

        return -1;

    
}



bool TemperatureController::unbindSensorFromPointBySensor(Sensor* sensor) {
    if (!sensor) return false;
    
    bool anyUnbound = false;
    
    // Search through all DS18B20 points
    for (auto& point : dsPoints) {
        if (point.getBoundSensor() == sensor) {
            point.unbindSensor();
            Serial.printf("Unbound sensor %s from DS18B20 point %d\n", 
                         sensor->getName().c_str(), point.getAddress());
            anyUnbound = true;
        }
    }
    
    // Search through all PT1000 points
    for (auto& point : ptPoints) {
        if (point.getBoundSensor() == sensor) {
            point.unbindSensor();
            Serial.printf("Unbound sensor %s from PT1000 point %d\n", 
                         sensor->getName().c_str(), point.getAddress());
            anyUnbound = true;
        }
    }
    
    return anyUnbound;
}



bool TemperatureController::addAlarm(AlarmType type, uint8_t pointAddress, AlarmPriority priority) {
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!point) return false;
    
    // Check if alarm already exists
    String configKey = "alarm_" + String(pointAddress) + "_" + String(static_cast<int>(type));
    
    for (auto alarm : _configuredAlarms) {
        if (alarm->getConfigKey() == configKey) {
            // Update existing
            alarm->setPriority(priority);
            alarm->setEnabled(true);
            return true;
        }
    }
    
    // Create new alarm
    Alarm* newAlarm = new Alarm(type, point, priority);
    if (newAlarm) {
        unsigned long delay;
        switch (priority) {
            case AlarmPriority::PRIORITY_CRITICAL:
                delay = _acknowledgedDelayCritical;
                break;
            case AlarmPriority::PRIORITY_HIGH:
                delay = _acknowledgedDelayHigh;
                break;
            case AlarmPriority::PRIORITY_MEDIUM:
                delay = _acknowledgedDelayMedium;
                break;
            case AlarmPriority::PRIORITY_LOW:
                delay = _acknowledgedDelayLow;
                break;
            default:
                delay = _acknowledgedDelayMedium;
                break;
        }
        newAlarm->setAcknowledgedDelay(delay);
    }
    newAlarm->setConfigKey(configKey);
    _configuredAlarms.push_back(newAlarm);
    
    Serial.printf("Added alarm configuration: %s\n", configKey.c_str());
    if (newAlarm) {
        MeasurementPoint* point = getMeasurementPoint(pointAddress);
        String pointName = point ? point->getName() : "Unknown";
        LoggerManager::info("ALARM_CONFIG", 
            "Added " + newAlarm->getTypeString() + " alarm for point " + 
            String(pointAddress) + " (" + pointName + ") with priority " + 
            _getPriorityString(priority));
    } else {
        LoggerManager::warning("ALARM_CONFIG", 
            "Failed to add " + newAlarm->getTypeString() + " alarm for point " + 
            String(pointAddress));
    }
    return true;
}

bool TemperatureController::removeAlarm(const String& configKey) {
    for (auto it = _configuredAlarms.begin(); it != _configuredAlarms.end(); ++it) {
        if ((*it)->getConfigKey() == configKey) {
            
            delete *it;
            _configuredAlarms.erase(it);
            Serial.printf("Removed alarm configuration: %s\n", configKey.c_str());
            LoggerManager::info("ALARM_CONFIG", 
                "Removed alarm configuration: " + String(configKey.c_str()));
            return true;
        }
    }
    return false;
}

bool TemperatureController::updateAlarm(const String& configKey, AlarmPriority priority, bool enabled) {
    Alarm* alarm = findAlarm(configKey);
    if (!alarm) return false;
    
    alarm->setPriority(priority);
    alarm->setEnabled(enabled);
    Serial.printf("Updated alarm configuration: %s\n", configKey.c_str());
    return true;
}

Alarm* TemperatureController::findAlarm(const String& configKey) {
    for (auto alarm : _configuredAlarms) {
        if (alarm->getConfigKey() == configKey) {
            return alarm;
        }
    }
    return nullptr;
}

Alarm* TemperatureController::getAlarmByIndex(int idx) {
    return (idx >= 0 && idx < _configuredAlarms.size()) ? _configuredAlarms[idx] : nullptr;
}



// Placeholder methods for alarm handling scenarios
void TemperatureController::handleCriticalAlarms() {
    // TODO: Implement critical alarm handling scenario
    // - Turn on both relays immediately
    // - Red LED on
    // - Display alarm
    // - Wait for acknowledgment
    // - 5-minute delay logic
}

void TemperatureController::handleHighPriorityAlarms() {
    // TODO: Implement high priority alarm handling scenario
    // - Different behavior than critical
    // - Maybe only one relay, yellow LED
}

void TemperatureController::handleMediumPriorityAlarms() {
    // TODO: Implement medium priority alarm handling scenario
}

void TemperatureController::handleLowPriorityAlarms() {
    // TODO: Implement low priority alarm handling scenario
}


bool TemperatureController::bindSensorToPointByBusNumber(uint8_t busNumber, uint8_t pointAddress) {
    // Find PT1000 sensor on the specified bus
    for (auto sensor : sensors) {
        if (sensor->getType() == SensorType::PT1000) {
            uint8_t sensorBus = getSensorBus(sensor);
            if (sensorBus == busNumber) {
                MeasurementPoint* point = getMeasurementPoint(pointAddress);
                if (point) {
                    point->bindSensor(sensor);
                    return true;
                }
            }
        }
    }
    return false;
}




// Implement the setter methods
void TemperatureController::setAcknowledgedDelayCritical(unsigned long delay) {
    _acknowledgedDelayCritical = delay;
    applyAcknowledgedDelaysToAlarms();
}

void TemperatureController::setAcknowledgedDelayHigh(unsigned long delay) {
    _acknowledgedDelayHigh = delay;
    applyAcknowledgedDelaysToAlarms();
}

void TemperatureController::setAcknowledgedDelayMedium(unsigned long delay) {
    _acknowledgedDelayMedium = delay;
    applyAcknowledgedDelaysToAlarms();
}

void TemperatureController::setAcknowledgedDelayLow(unsigned long delay) {
    _acknowledgedDelayLow = delay;
    applyAcknowledgedDelaysToAlarms();
}

// Implement the getter methods
unsigned long TemperatureController::getAcknowledgedDelayCritical() const {
    return _acknowledgedDelayCritical;
}

unsigned long TemperatureController::getAcknowledgedDelayHigh() const {
    return _acknowledgedDelayHigh;
}

unsigned long TemperatureController::getAcknowledgedDelayMedium() const {
    return _acknowledgedDelayMedium;
}

unsigned long TemperatureController::getAcknowledgedDelayLow() const {
    return _acknowledgedDelayLow;
}

// Method to apply delays to all existing alarms
void TemperatureController::applyAcknowledgedDelaysToAlarms() {
    for (auto alarm : _configuredAlarms) {
        unsigned long delay;
        switch (alarm->getPriority()) {
            case AlarmPriority::PRIORITY_CRITICAL:
                delay = _acknowledgedDelayCritical;
                break;
            case AlarmPriority::PRIORITY_HIGH:
                delay = _acknowledgedDelayHigh;
                break;
            case AlarmPriority::PRIORITY_MEDIUM:
                delay = _acknowledgedDelayMedium;
                break;
            case AlarmPriority::PRIORITY_LOW:
                delay = _acknowledgedDelayLow;
                break;
            default:
                delay = _acknowledgedDelayMedium; // Default fallback
                break;
        }
        alarm->setAcknowledgedDelay(delay);
    }
    
    // Also apply to active alarms
    // for (auto alarm : _activeAlarms) {
    //     unsigned long delay;
    //     switch (alarm->getPriority()) {
    //         case AlarmPriority::PRIORITY_CRITICAL:
    //             delay = _acknowledgedDelayCritical;
    //             break;
    //         case AlarmPriority::PRIORITY_HIGH:
    //             delay = _acknowledgedDelayHigh;
    //             break;
    //         case AlarmPriority::PRIORITY_MEDIUM:
    //             delay = _acknowledgedDelayMedium;
    //             break;
    //         case AlarmPriority::PRIORITY_LOW:
    //             delay = _acknowledgedDelayLow;
    //             break;
    //         default:
    //             delay = _acknowledgedDelayMedium;
    //             break;
    //     }
    //     alarm->setAcknowledgedDelay(delay);
    // }
}

// int TemperatureController::getAlarmCount(AlarmPriority priority) const {
//     int count = 0;
//     for (auto alarm : _configuredAlarms) {
//         if (alarm->isEnabled() && alarm->getPriority() == priority) {
//             count++;
//         }
//     }
//     return count;
// }

// int TemperatureController::getAlarmCount(AlarmStage stage) const {
//     int count = 0;
//     for (auto alarm : _configuredAlarms) {
//         if (alarm->isEnabled() && alarm->getStage() == stage) {
//             count++;
//         }
//     }
//     return count;
// }


// Add these method implementations to TemperatureController.cpp
int TemperatureController::getAlarmCount(AlarmPriority priority, const String& comparison) const {
    int count = 0;
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && _comparePriority(alarm->getPriority(), priority, comparison)) {
            count++;
        }
    }
    return count;
}

int TemperatureController::getAlarmCount(AlarmStage stage, const String& comparison) const {
    int count = 0;
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && _compareStage(alarm->getStage(), stage, comparison)) {
            count++;
        }
    }
    return count;
}

int TemperatureController::getAlarmCount(AlarmPriority priority, AlarmStage stage, 
                                       const String& priorityComparison, 
                                       const String& stageComparison) const {
    int count = 0;
    for (auto alarm : _configuredAlarms) {
        if (alarm->isEnabled() && 
            _comparePriority(alarm->getPriority(), priority, priorityComparison) &&
            _compareStage(alarm->getStage(), stage, stageComparison)) {
            count++;
        }
    }
    return count;
}

// Add these helper method implementations to TemperatureController.cpp
bool TemperatureController::_comparePriority(AlarmPriority alarmPriority, AlarmPriority targetPriority, const String& comparison) const {
    int alarmValue = static_cast<int>(alarmPriority);
    int targetValue = static_cast<int>(targetPriority);
    
    if (comparison == ">" || comparison == "gt") {
        return alarmValue > targetValue;
    } else if (comparison == ">=" || comparison == "gte") {
        return alarmValue >= targetValue;
    } else if (comparison == "<" || comparison == "lt") {
        return alarmValue < targetValue;
    } else if (comparison == "<=" || comparison == "lte") {
        return alarmValue <= targetValue;
    } else if (comparison == "!=" || comparison == "ne") {
        return alarmValue != targetValue;
    } else { // Default to "==" or "eq"
        return alarmValue == targetValue;
    }
}

bool TemperatureController::_compareStage(AlarmStage alarmStage, AlarmStage targetStage, const String& comparison) const {
    int alarmValue = static_cast<int>(alarmStage);
    int targetValue = static_cast<int>(targetStage);
    
    if (comparison == ">" || comparison == "gt") {
        return alarmValue > targetValue;
    } else if (comparison == ">=" || comparison == "gte") {
        return alarmValue >= targetValue;
    } else if (comparison == "<" || comparison == "lt") {
        return alarmValue < targetValue;
    } else if (comparison == "<=" || comparison == "lte") {
        return alarmValue <= targetValue;
    } else if (comparison == "!=" || comparison == "ne") {
        return alarmValue != targetValue;
    } else { // Default to "==" or "eq"
        return alarmValue == targetValue;
    }
}


void TemperatureController::_handleLowPriorityBlinking() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - _lastLowPriorityBlinkTime;
    
    if (_lowPriorityBlinkState) {
        // Currently ON - check if we should turn OFF
        if (elapsed >= _blinkOnTime) {
            _lowPriorityBlinkState = false;
            _lastLowPriorityBlinkTime = currentTime;
        }
    } else {
        // Currently OFF - check if we should turn ON
        if (elapsed >= _blinkOffTime) {
            _lowPriorityBlinkState = true;
            _lastLowPriorityBlinkTime = currentTime;
        }
    }
}


void TemperatureController::handleAlarmDisplay() {
    // Update alarm queues
    _updateAlarmQueues();
    
    // Handle button press for acknowledgment
    _checkButtonPress();
    
    // Handle alarm display rotation
    _handleAlarmDisplayRotation();
    
    // Update indicator blinking
    indicator.updateBlinking();
}

void TemperatureController::_updateAlarmQueues() {
    _activeAlarmsQueue.clear();
    _acknowledgedAlarmsQueue.clear();
    
    // Get all active alarms from the system
    std::vector<Alarm*> allActiveAlarms = getActiveAlarms();
    
    // Separate alarms into active and acknowledged queues
    for (auto alarm : allActiveAlarms) {
        if (!alarm) continue;
        
        if (alarm->getStage() == AlarmStage::ACTIVE) {
            _activeAlarmsQueue.push_back(alarm);
        } else if (alarm->getStage() == AlarmStage::ACKNOWLEDGED) {
            _acknowledgedAlarmsQueue.push_back(alarm);
        }
    }
    
    // Sort active alarms by priority (highest first), then by timestamp (oldest first)
    std::sort(_activeAlarmsQueue.begin(), _activeAlarmsQueue.end(), 
              [](const Alarm* a, const Alarm* b) {
                  if (a->getPriority() != b->getPriority()) {
                      return static_cast<int>(a->getPriority()) > static_cast<int>(b->getPriority());
                  }
                  return a->getTimestamp() < b->getTimestamp();
              });
    
    // Sort acknowledged alarms by priority (highest first), then by timestamp (oldest first)
    std::sort(_acknowledgedAlarmsQueue.begin(), _acknowledgedAlarmsQueue.end(),
              [](const Alarm* a, const Alarm* b) {
                  if (a->getPriority() != b->getPriority()) {
                      return static_cast<int>(a->getPriority()) > static_cast<int>(b->getPriority());
                  }
                  return a->getTimestamp() < b->getTimestamp();
              });
}
void TemperatureController::_displayNextActiveAlarm() {
    if (_activeAlarmsQueue.empty()) return;
    
    if (_currentActiveAlarmIndex >= _activeAlarmsQueue.size()) {
        _currentActiveAlarmIndex = 0;
    }
    
    _currentDisplayedAlarm = _activeAlarmsQueue[_currentActiveAlarmIndex];
    _showingOK = false;
    
    // Ensure OLED is turned on whenever an alarm is displayed
    indicator.setOLEDOn();
    indicator.setOledMode(2);
    String displayText = _currentDisplayedAlarm->getDisplayText();
    
    int newlineIndex = displayText.indexOf('\n');
    String line1 = displayText.substring(0, newlineIndex);
    String line2 = displayText.substring(newlineIndex + 1);
    
    String displayLines[2] = {line1, line2};
    indicator.printText(displayLines, 2);
    
    Serial.printf("Displaying active alarm: %s\n", displayText.c_str());
}

void TemperatureController::_displayNextAcknowledgedAlarm() {
    if (_acknowledgedAlarmsQueue.empty()) return;
    
    _currentAcknowledgedAlarmIndex = (_currentAcknowledgedAlarmIndex + 1) % _acknowledgedAlarmsQueue.size();
    
    _currentDisplayedAlarm = _acknowledgedAlarmsQueue[_currentAcknowledgedAlarmIndex];
    _lastAlarmDisplayTime = millis();
    _showingOK = false;
    
    // Ensure OLED is turned on whenever an alarm is displayed
    indicator.setOLEDOn();
    indicator.setOledMode(2);
    String displayText = _currentDisplayedAlarm->getDisplayText();
    
    int newlineIndex = displayText.indexOf('\n');
    String line1 = displayText.substring(0, newlineIndex);
    String line2 = displayText.substring(newlineIndex + 1);
    
    String displayLines[2] = {line1, line2};
    indicator.printText(displayLines, 2);
    
    Serial.printf("Displaying acknowledged alarm: %s (%d/%d)\n",
                  displayText.c_str(),
                  _currentAcknowledgedAlarmIndex + 1,
                  _acknowledgedAlarmsQueue.size());
}

void TemperatureController::_handleAlarmDisplayRotation() {
    unsigned long currentTime = millis();
    
    // Priority 1: Display active alarms first
    if (!_activeAlarmsQueue.empty()) {
        _displayingActiveAlarm = true;
        _currentAcknowledgedAlarmIndex = 0;
        
        // Always turn on OLED when there are active alarms
        indicator.setOLEDOn();
        _displayNextActiveAlarm();
        
        if (_currentDisplayedAlarm && _currentDisplayedAlarm->getStage() == AlarmStage::ACKNOWLEDGED) {
            _currentActiveAlarmIndex++;
            if (_currentActiveAlarmIndex >= _activeAlarmsQueue.size()) {
                _currentActiveAlarmIndex = 0;
            }
        }
        return;
    }
    
    // Priority 2: Display acknowledged alarms in round-robin
    if (!_acknowledgedAlarmsQueue.empty()) {
        _displayingActiveAlarm = false;
        
        // Always turn on OLED when there are acknowledged alarms
        indicator.setOLEDOn();
        
        if (_currentDisplayedAlarm == nullptr || 
            _currentDisplayedAlarm->getStage() != AlarmStage::ACKNOWLEDGED ||
            currentTime - _lastAlarmDisplayTime >= _acknowledgedAlarmDisplayDelay) {
            _displayNextAcknowledgedAlarm();
        }
        return;
    }
    
    // No alarms to display - show OK and turn off OLED
    if (_currentDisplayedAlarm && !_showingOK) {
        _showOKAndTurnOffOLED();
    } else if (_showingOK) {
        if (currentTime - _okDisplayStartTime >= 60000) {
            indicator.setOLEDOff();
            _showingOK = false;
            _currentDisplayedAlarm = nullptr;
        }
    } else {
        _updateNormalDisplay();
    }
}

void TemperatureController::_checkButtonPress() {
    // Use the existing indicator interface button reading with built-in debouncing
    bool currentButtonState = indicator.readPort("BUTTON");
    
    // Detect button press (button pressed = true based on your configuration)
    if (currentButtonState && !_lastButtonState) {
        // Button was just pressed
        if (_displayingActiveAlarm && _currentDisplayedAlarm && 
            _currentDisplayedAlarm->getStage() == AlarmStage::ACTIVE) {
            
            _currentDisplayedAlarm->acknowledge();
            
            Serial.printf("Button pressed - Acknowledged alarm: %s\n",
                          _currentDisplayedAlarm->getDisplayText().c_str());
            
            // Move to next active alarm or switch to acknowledged display
            _currentActiveAlarmIndex++;
            if (_currentActiveAlarmIndex >= _activeAlarmsQueue.size()) {
                _currentActiveAlarmIndex = 0;
            }
            
            // Force immediate update of display
            _lastAlarmDisplayTime = 0;
        }
    }
    
    _lastButtonState = currentButtonState;
}

// Helper method to get priority string





    String TemperatureController::_getAlarmTypeString(AlarmType type) const {
        switch (type) {
            case AlarmType::HIGH_TEMPERATURE: return "HIGH_TEMP";
            case AlarmType::LOW_TEMPERATURE: return "LOW_TEMP";
            case AlarmType::SENSOR_ERROR: return "SENSOR_ERROR";
            case AlarmType::SENSOR_DISCONNECTED: return "DISCONNECTED";
            default: return "UNKNOWN";
        }
    }
    
    String TemperatureController::_getPriorityString(AlarmPriority priority) const {
        switch (priority) {
            case AlarmPriority::PRIORITY_LOW: return "LOW";
            case AlarmPriority::PRIORITY_MEDIUM: return "MEDIUM";
            case AlarmPriority::PRIORITY_HIGH: return "HIGH";
            case AlarmPriority::PRIORITY_CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
