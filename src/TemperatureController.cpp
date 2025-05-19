#include "TemperatureController.h"
#include <OneWire.h>
#include <DallasTemperature.h>

TemperatureController::TemperatureController(uint8_t oneWirePin)
    : oneWireBusPin(oneWirePin),
      measurementPeriodSeconds(10),  // Default to 10 seconds
      deviceId(1000),                // Default device ID
      firmwareVersion(0x0100),       // v1.0
      lastMeasurementTime(0),
      systemInitialized(false) {
}

TemperatureController::~TemperatureController() {
    // Clean up all sensor objects
    for (auto sensor : sensors) {
        delete sensor;
    }
    sensors.clear();
}

bool TemperatureController::begin() {
    // Set device information in register map
    registerMap.writeHoldingRegister(0, deviceId);
    registerMap.writeHoldingRegister(1, firmwareVersion);
    registerMap.writeHoldingRegister(2, 0); // No active DS18B20 sensors yet
    registerMap.writeHoldingRegister(3, 0); // No active PT1000 sensors yet
    
    // Initialize device status registers to 0
    for (int i = 4; i <= 10; i++) {
        registerMap.writeHoldingRegister(i, 0);
    }
    
    // Set system as initialized
    systemInitialized = true;
    
    return true;
}

bool TemperatureController::addSensor(SensorType type, uint8_t address, const String& name) {
    // Check if address is already in use
    if (findSensor(address) != nullptr) {
        return false; // Address already exists
    }
    
    // Validate address range based on sensor type
    if ((type == SensorType::DS18B20 && address >= 50) || 
        (type == SensorType::PT1000 && (address < 50 || address >= 60))) {
        return false; // Invalid address for sensor type
    }
    
    // Create new sensor
    Sensor* newSensor = new Sensor(type, address, name);
    
    // Add to vector
    sensors.push_back(newSensor);
    
    // Update register map count
    if (type == SensorType::DS18B20) {
        registerMap.incrementActiveDS18B20();
    } else {
        registerMap.incrementActivePT1000();
    }
    
    return true;
}

bool TemperatureController::removeSensor(uint8_t address) {
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        if ((*it)->getAddress() == address) {
            // Update register map count
            if ((*it)->getType() == SensorType::DS18B20) {
                registerMap.decrementActiveDS18B20();
            } else {
                registerMap.decrementActivePT1000();
            }
            
            // Delete sensor and remove from vector
            delete *it;
            sensors.erase(it);
            return true;
        }
    }
    return false; // Sensor not found
}

bool TemperatureController::updateSensorAddress(uint8_t oldAddress, uint8_t newAddress) {
    // Check if new address is already in use
    if (findSensor(newAddress) != nullptr) {
        return false; // New address already exists
    }
    
    Sensor* sensor = findSensor(oldAddress);
    if (sensor == nullptr) {
        return false; // Old address not found
    }
    
    // Validate address range based on sensor type
    SensorType type = sensor->getType();
    if ((type == SensorType::DS18B20 && newAddress >= 50) || 
        (type == SensorType::PT1000 && (newAddress < 50 || newAddress >= 60))) {
        return false; // Invalid address for sensor type
    }
    
    // Update the address
    sensor->setAddress(newAddress);
    return true;
}

bool TemperatureController::updateSensorName(uint8_t address, const String& newName) {
    Sensor* sensor = findSensor(address);
    if (sensor == nullptr) {
        return false; // Address not found
    }
    
    sensor->setName(newName);
    return true;
}

Sensor* TemperatureController::findSensor(uint8_t address) {
    for (auto sensor : sensors) {
        if (sensor->getAddress() == address) {
            return sensor;
        }
    }
    return nullptr; // Not found
}

void TemperatureController::update() {
    if (!systemInitialized) {
        return;
    }
    
    // Check if it's time to take measurements
    unsigned long currentTime = millis();
    if (currentTime - lastMeasurementTime >= measurementPeriodSeconds) {
        readAllSensors();
        updateRegisterMap();
        lastMeasurementTime = currentTime;
    }
    
    // Apply any configuration changes from register map
    applyConfigFromRegisterMap();
}

void TemperatureController::readAllSensors() {
    for (auto sensor : sensors) {
        sensor->readTemperature();
        sensor->updateAlarmStatus();
    }
}

void TemperatureController::updateRegisterMap() {
    for (auto sensor : sensors) {
        registerMap.updateFromSensor(*sensor);
    }
}

void TemperatureController::applyConfigFromRegisterMap() {
    for (auto sensor : sensors) {
        registerMap.applyConfigToSensor(*sensor);
    }
}

void TemperatureController::applyConfigToRegisterMap() {
    for (auto sensor : sensors) {
        registerMap.applyConfigFromSensor(*sensor);
    }
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

void TemperatureController::setDeviceId(uint16_t id) {
    deviceId = id;
    registerMap.writeHoldingRegister(0, deviceId);
}

void TemperatureController::setFirmwareVersion(uint16_t version) {
    firmwareVersion = version;
    registerMap.writeHoldingRegister(1, firmwareVersion);
}

bool TemperatureController::discoverDS18B20Sensors() {
    // Create temporary OneWire and DallasTemperature instances for discovery
    OneWire oneWire(oneWireBusPin);
    DallasTemperature dallasSensors(&oneWire);
    dallasSensors.begin();
    
    // Get count of devices on the bus
    int deviceCount = dallasSensors.getDeviceCount();
    if (deviceCount == 0) {
        return false; // No sensors found
    }
    
    // Get addresses for all sensors
    DeviceAddress sensorAddress;
    bool anyAdded = false;
    
    // Find next available address for DS18B20 sensors
    uint8_t nextAddress = 0;
    for (auto sensor : sensors) {
        if (sensor->getType() == SensorType::DS18B20 && sensor->getAddress() >= nextAddress) {
            nextAddress = sensor->getAddress() + 1;
        }
    }
    
    // Add discovered sensors
    for (int i = 0; i < deviceCount; i++) {
        if (dallasSensors.getAddress(sensorAddress, i)) {
            // Skip if we've reached address limit
            if (nextAddress >= 50) {
                break;
            }
            
            // Check if this sensor already exists by comparing ROM address
            bool alreadyExists = false;
            for (auto sensor : sensors) {
                if (sensor->getType() == SensorType::DS18B20) {
                    const uint8_t* existingROM = sensor->getDS18B20Address();
                    if (existingROM != nullptr) {
                        // Compare ROM addresses
                        if (memcmp(existingROM, sensorAddress, 8) == 0) {
                            alreadyExists = true;
                            break;
                        }
                    }
                }
            }
            
            if (!alreadyExists) {
                // Create a new sensor with the next available address
                String sensorName = "DS18B20_" + String(nextAddress);
                Sensor* newSensor = new Sensor(SensorType::DS18B20,  nextAddress, sensorName);
                
                // Set up the DS18B20 with the pin and address
                newSensor->setupDS18B20(oneWireBusPin, sensorAddress);
                
                // Initialize the sensor
                if (newSensor->initialize()) {
                    // Add to vector
                    sensors.push_back(newSensor);
                    
                    // Update register map count
                    registerMap.incrementActiveDS18B20();
                    
                    // Set default alarm thresholds
                    //newSensor->setLowAlarmThreshold(-10);
                    //newSensor->setHighAlarmThreshold(50);
                    
                    anyAdded = true;
                    nextAddress++;
                } else {
                    delete newSensor;
                }
            }
        }
    }
    
    return anyAdded;
}



String TemperatureController::getSensorsJson() {
    DynamicJsonDocument doc(4096); // Adjust size based on your needs
    JsonArray sensorArray = doc.createNestedArray("sensors");
    
    for (auto sensor : sensors) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj["address"] = sensor->getAddress();
        sensorObj["name"] = sensor->getName();
        sensorObj["type"] = (sensor->getType() == SensorType::DS18B20) ? "DS18B20" : "PT1000";
        sensorObj["currentTemp"] = sensor->getCurrentTemp();
        sensorObj["minTemp"] = sensor->getMinTemp();
        sensorObj["maxTemp"] = sensor->getMaxTemp();
        sensorObj["lowAlarmThreshold"] = sensor->getLowAlarmThreshold();
        sensorObj["highAlarmThreshold"] = sensor->getHighAlarmThreshold();
        sensorObj["alarmStatus"] = sensor->getAlarmStatus();
        sensorObj["errorStatus"] = sensor->getErrorStatus();
        
        // Add ROM address for DS18B20 sensors
        if (sensor->getType() == SensorType::DS18B20) {
            const uint8_t* romAddress = sensor->getDS18B20Address();
            if (romAddress != nullptr) {
                JsonArray romArray = sensorObj.createNestedArray("romAddress");
                for (int i = 0; i < 8; i++) {
                    romArray.add(romAddress[i]);
                }
            }
        }
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}



String TemperatureController::getSystemStatusJson() {
    DynamicJsonDocument doc(1024); // Adjust size based on your needs
    
    doc["deviceId"] = deviceId;
    doc["firmwareVersion"] = firmwareVersion;
    doc["ds18b20Count"] = getDS18B20Count();
    doc["pt1000Count"] = getPT1000Count();
    doc["measurementPeriod"] = measurementPeriodSeconds;
    doc["uptime"] = millis() / 1000; // Uptime in seconds
    
    // Add device status registers
    JsonArray statusArray = doc.createNestedArray("deviceStatus");
    for (int i = 4; i <= 10; i++) {
        statusArray.add(registerMap.readHoldingRegister(i));
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void TemperatureController::resetMinMaxValues() {
    for (auto sensor : sensors) {
        sensor->resetMinMaxTemp();
    }
}

bool TemperatureController::addSensor(Sensor* sensor) {
    if (sensor == nullptr) {
        return false;
    }
    
    // Check if address is already in use
    if (findSensor(sensor->getAddress()) != nullptr) {
        return false; // Address already exists
    }
    
    // Add to vector
    sensors.push_back(sensor);
    
    // Update register map count
    if (sensor->getType() == SensorType::DS18B20) {
        registerMap.incrementActiveDS18B20();
    } else {
        registerMap.incrementActivePT1000();
    }
    
    return true;
}

Sensor* TemperatureController::getSensorByIndex(int index) {
    if (index >= 0 && index < sensors.size()) {
        return sensors[index];
    }
    return nullptr;
}

bool TemperatureController::addSensorFromConfig(Sensor* sensor) {
    if (sensor == nullptr) {
        return false;
    }
    
    // Check if address is already in use
    if (findSensor(sensor->getAddress()) != nullptr) {
        return false; // Address already exists
    }
    
    // Add to vector
    sensors.push_back(sensor);
    Serial.printf("Controller: New HAS: %d, New LAS: %d \n", sensor->getHighAlarmThreshold(), sensor->getLowAlarmThreshold());
    
    // Update register map count
    if (sensor->getType() == SensorType::DS18B20) {
        registerMap.incrementActiveDS18B20();
    } else {
        registerMap.incrementActivePT1000();
    }
    applyConfigToRegisterMap();
    
    return true;
}

