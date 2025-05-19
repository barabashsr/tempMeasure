#include "TemperatureController.h"


TemperatureController::TemperatureController(uint8_t oneWirePin)
    : measurementPeriodSeconds(10),
      deviceId(1000),
      firmwareVersion(0x0100),
      lastMeasurementTime(0),
      systemInitialized(false),
      oneWireBusPin(oneWirePin)
{
    for (uint8_t i = 0; i < 50; ++i)
        dsPoints[i] = MeasurementPoint(i, "DS18B20_Point_" + String(i));
    for (uint8_t i = 0; i < 10; ++i)
        ptPoints[i] = MeasurementPoint(50 + i, "PT1000_Point_" + String(i));
}

TemperatureController::~TemperatureController() {
    for (auto sensor : sensors)
        delete sensor;
    sensors.clear();
}

bool TemperatureController::begin() {
    registerMap.writeHoldingRegister(0, deviceId);
    registerMap.writeHoldingRegister(1, firmwareVersion);
    registerMap.writeHoldingRegister(2, 0);
    registerMap.writeHoldingRegister(3, 0);
    for (int i = 4; i <= 10; i++)
        registerMap.writeHoldingRegister(i, 0);
    systemInitialized = true;
    return true;
}

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
    Sensor* sensor = findSensorByRom(romString);
    MeasurementPoint* point = getMeasurementPoint(pointAddress);
    if (!sensor || !point) return false;
    point->bindSensor(sensor);
    return true;
}

bool TemperatureController::bindSensorToPointByChipSelect(uint8_t csPin, uint8_t pointAddress) {
    Sensor* sensor = findSensorByChipSelect(csPin);
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

void TemperatureController::update() {
    updateAllSensors();
    if (!systemInitialized) return;
    unsigned long currentTime = millis();
    if (currentTime - lastMeasurementTime >= measurementPeriodSeconds * 1000UL) {
        readAllPoints();
        updateRegisterMap();
        lastMeasurementTime = currentTime;
        applyConfigFromRegisterMap();
    }
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
    OneWire oneWire(oneWireBusPin);
    DallasTemperature dallasSensors(&oneWire);
    dallasSensors.begin();

    int deviceCount = dallasSensors.getDeviceCount();
    if (deviceCount == 0) return false;

    DeviceAddress sensorAddress;
    bool anyAdded = false;

    for (int i = 0; i < deviceCount; i++) {
        if (dallasSensors.getAddress(sensorAddress, i)) {
            // Convert ROM to string for uniqueness
            char buf[17];
            for (int j = 0; j < 8; ++j) sprintf(buf + j*2, "%02X", sensorAddress[j]);
            String romString(buf);

            // Check if already exists
            if (findSensorByRom(romString)) continue;

            String sensorName = "DS18B20_" + romString;
            Sensor* newSensor = new Sensor(SensorType::DS18B20, 0, sensorName); // address field not used for DS
            newSensor->setupDS18B20(oneWireBusPin, sensorAddress);

            if (newSensor->initialize()) {
                sensors.push_back(newSensor);
                registerMap.incrementActiveDS18B20();
                anyAdded = true;
            } else {
                delete newSensor;
            }
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
    for (uint8_t i = 0; i < 50; ++i) {
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
        }
    }

    // PT1000 points
    for (uint8_t i = 0; i < 10; ++i) {
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

void TemperatureController::setOneWireBusPin(uint8_t pin) {
    oneWireBusPin = pin;
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