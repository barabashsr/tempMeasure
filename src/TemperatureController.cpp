#include "TemperatureController.h"

TemperatureController::TemperatureController(uint8_t oneWirePin[4], uint8_t csPin[4], IndicatorInterface& indicator)
    : measurementPeriodSeconds(10),
      deviceId(1000),
      firmwareVersion(0x0100),
      lastMeasurementTime(0),
      systemInitialized(false),
      indicator(indicator),
      _lastAlarmCheck(0),
      _lastButtonState(true),
      _lastButtonPressTime(0),
      _currentDisplayedAlarm(nullptr),
      _okDisplayStartTime(0),
      _showingOK(false)
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
    
    // Clean up alarms
    for (auto alarm : _alarms)
        delete alarm;
    _alarms.clear();
    
    // Clean up OneWire buses
    for (int i = 0; i < 4; ++i) {
        delete dallasSensors[i];
        delete oneWireBuses[i];
    }
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
    indicator.updateOLED();
    
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
    
    // Ensure we have fresh sensor data before checking alarms
    Serial.println("Checking alarms with fresh sensor data...");
    
    // Check all measurement points for alarm conditions
    for (uint8_t i = 0; i < 50; ++i) {
        if (dsPoints[i].getBoundSensor() != nullptr) {
            Serial.printf("DS Point %d: Temp=%d, High=%d, Low=%d\n", 
                         i, dsPoints[i].getCurrentTemp(), 
                         dsPoints[i].getHighAlarmThreshold(), 
                         dsPoints[i].getLowAlarmThreshold());
            _checkPointForAlarms(&dsPoints[i]);
        }
    }
    
    for (uint8_t i = 0; i < 10; ++i) {
        if (ptPoints[i].getBoundSensor() != nullptr) {
            Serial.printf("PT Point %d: Temp=%d, High=%d, Low=%d\n", 
                         i, ptPoints[i].getCurrentTemp(), 
                         ptPoints[i].getHighAlarmThreshold(), 
                         ptPoints[i].getLowAlarmThreshold());
            _checkPointForAlarms(&ptPoints[i]);
        }
    }
    
    // Update existing alarms
    for (auto it = _alarms.begin(); it != _alarms.end();) {
        bool conditionExists = (*it)->updateCondition();
        if (!conditionExists && (*it)->isResolved()) {
            // Alarm is resolved, remove it
            if (_currentDisplayedAlarm == *it) {
                _currentDisplayedAlarm = nullptr;
            }
            Serial.printf("Removing resolved alarm for point %d\n", 
                         (*it)->getSource() ? (*it)->getSource()->getAddress() : -1);
            delete *it;
            it = _alarms.erase(it);
        } else {
            ++it;
        }
    }
    
    // Sort alarms by priority
    std::sort(_alarms.begin(), _alarms.end(), AlarmComparator());
    
    Serial.printf("Active alarms count: %d\n", getActiveAlarms().size());
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
    for (auto alarm : _alarms) {
        if (alarm->getSource() == point && 
            alarm->getType() == type && 
            alarm->isActive()) {
            return true;
        }
    }
    return false;
}

void TemperatureController::createAlarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority) {
    Alarm* newAlarm = new Alarm(type, source, priority);
    _alarms.push_back(newAlarm);
    
    // Sort alarms by priority
    std::sort(_alarms.begin(), _alarms.end(), AlarmComparator());
}

Alarm* TemperatureController::getHighestPriorityAlarm() const {
    for (auto alarm : _alarms) {
        if (alarm->isActive()) {
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
    for (auto alarm : _alarms) {
        if (alarm->isActive() && !alarm->isAcknowledged()) {
            alarm->acknowledge();
        }
    }
}

std::vector<Alarm*> TemperatureController::getActiveAlarms() const {
    std::vector<Alarm*> activeAlarms;
    for (auto alarm : _alarms) {
        if (alarm->isActive()) {
            activeAlarms.push_back(alarm);
        }
    }
    return activeAlarms;
}

void TemperatureController::clearResolvedAlarms() {
    for (auto it = _alarms.begin(); it != _alarms.end();) {
        if ((*it)->isResolved()) {
            if (_currentDisplayedAlarm == *it) {
                _currentDisplayedAlarm = nullptr;
            }
            delete *it;
            it = _alarms.erase(it);
        } else {
            ++it;
        }
    }
}

void TemperatureController::handleAlarmDisplay() {
    Alarm* highestPriorityAlarm = getHighestPriorityAlarm();
    
    if (highestPriorityAlarm) {
        // Display alarm
        _currentDisplayedAlarm = highestPriorityAlarm;
        _showingOK = false;
        
        indicator.setOledMode(2);
        String displayText = highestPriorityAlarm->getDisplayText();
        
        // Split display text into lines
        int newlineIndex = displayText.indexOf('\n');
        String line1 = displayText.substring(0, newlineIndex);
        String line2 = displayText.substring(newlineIndex + 1);
        
        String displayLines[2] = {line1, line2};
        indicator.printText(displayLines, 2);
        
    } else if (_currentDisplayedAlarm && !_showingOK) {
        // No more alarms, show OK for 1 minute
        _showOKAndTurnOffOLED();
        
    } else if (_showingOK) {
        // Check if OK display time has elapsed
        if (millis() - _okDisplayStartTime >= 60000) { // 1 minute
            indicator.setOLEDOff();
            _showingOK = false;
            _currentDisplayedAlarm = nullptr;
        }
        
    } else {
        // Normal operation - show normal display
        _updateNormalDisplay();
    }
}

void TemperatureController::handleAlarmOutputs() {
    Alarm* highestPriorityAlarm = getHighestPriorityAlarm();
    
    if (highestPriorityAlarm) {
        // Handle alarm outputs based on type and stage
        if (highestPriorityAlarm->getType() == AlarmType::HIGH_TEMPERATURE) {
            // High temperature alarm
            indicator.writePort("RedLED", true);
            indicator.writePort("GreenLED", false);
            
            if (!highestPriorityAlarm->isAcknowledged()) {
                indicator.writePort("Relay1", true);
            } else {
                indicator.writePort("Relay1", false);
            }
            indicator.writePort("Relay2", true);
        }
        // Add other alarm type handling here
        
    } else {
        // No active alarms - normal operation
        indicator.writePort("GreenLED", true);
        indicator.writePort("RedLED", false);
        indicator.writePort("Relay1", false);
        indicator.writePort("Relay2", false);
    }
}

void TemperatureController::_checkButtonPress() {
    bool currentButtonState = indicator.readPort("BUTTON");
    
    // Detect button press (HIGH to LOW transition)
    if (_lastButtonState == true && currentButtonState == false) {
        if ((millis() - _lastButtonPressTime) > _buttonDebounceDelay) {
            Serial.println("BUTTON PRESS DETECTED!");
            acknowledgeHighestPriorityAlarm();
            _lastButtonPressTime = millis();
        }
    }
    
    _lastButtonState = currentButtonState;
}

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

String TemperatureController::getAlarmsJson() const {
    DynamicJsonDocument doc(4096);
    JsonArray alarmArray = doc.createNestedArray("alarms");
    
    for (auto alarm : _alarms) {
        JsonObject obj = alarmArray.createNestedObject();
        obj["type"] = static_cast<int>(alarm->getType());
        obj["stage"] = static_cast<int>(alarm->getStage());
        obj["priority"] = static_cast<int>(alarm->getPriority());
        obj["timestamp"] = alarm->getTimestamp();
        obj["message"] = alarm->getMessage();
        obj["isActive"] = alarm->isActive();
        obj["isAcknowledged"] = alarm->isAcknowledged();
        
        if (alarm->getSource()) {
            obj["pointAddress"] = alarm->getSource()->getAddress();
            obj["pointName"] = alarm->getSource()->getName();
            obj["currentTemp"] = alarm->getSource()->getCurrentTemp();
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
    if (!sensor || !point) return false;
    point->bindSensor(sensor);
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
    if (!sensor || !point) return false;
    point->bindSensor(sensor);
    return true;
}

bool TemperatureController::unbindSensorFromPoint(uint8_t pointAddress) {
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!point) return false;
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
    // OneWire oneWire[] = { OneWire(oneWireBusPin[0]), OneWire(oneWireBusPin[1]), OneWire(oneWireBusPin[2]), OneWire(oneWireBusPin[3]) };
    // DallasTemperature dallasSensors[] = {DallasTemperature(&oneWire[0]), DallasTemperature(&oneWire[1]), DallasTemperature(&oneWire[2]), DallasTemperature(&oneWire[3])};
    for (uint j = 0; j < 4; j++){
        Serial.printf("Discover bus %d pin %d started...\n", j, oneWireBusPin[j]);
        
    //OneWire oneWire(oneWireBusPin[j]);
    
    dallasSensors[j]->begin();

    int deviceCount = dallasSensors[j]->getDeviceCount();
    Serial.printf("Devices on bus %d: %d\n", j, deviceCount);
    if (deviceCount == 0) continue;

    DeviceAddress sensorAddress;
    

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
    return anyAdded;
}


bool TemperatureController::discoverPTSensors() {
    bool anyAdded = false;
    Serial.println("Discover PT method started...");
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
    for (uint8_t i = 0; i < 50; ++i)
        dsPoints[i].resetMinMaxTemp();
    for (uint8_t i = 0; i < 10; ++i)
        ptPoints[i].resetMinMaxTemp();
}

void TemperatureController::setDeviceId(uint16_t id) {
    deviceId = id;
    registerMap.writeHoldingRegister(0, deviceId);
}

uint16_t TemperatureController::getDeviceId() const { return deviceId; }

void TemperatureController::setFirmwareVersion(uint16_t version) {
    firmwareVersion = version;
    registerMap.writeHoldingRegister(1, firmwareVersion);
}

uint16_t TemperatureController::getFirmwareVersion() const { return firmwareVersion; }

void TemperatureController::setMeasurementPeriod(uint16_t seconds) {
    measurementPeriodSeconds = seconds;
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

