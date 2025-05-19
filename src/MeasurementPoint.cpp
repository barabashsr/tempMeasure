#include "MeasurementPoint.h"


MeasurementPoint::MeasurementPoint(uint8_t address, const String& name)
    : address(address),
      name(name),
      currentTemp(0),
      minTemp(32767),
      maxTemp(-32768),
      lowAlarmThreshold(-10),
      highAlarmThreshold(50),
      alarmStatus(0),
      errorStatus(0),
      boundSensor(nullptr)
{
    // Nothing else needed
}

MeasurementPoint::~MeasurementPoint() {
    // Do not delete boundSensor here; ownership is external
    boundSensor = nullptr;
}

uint8_t MeasurementPoint::getAddress() const {
    return address;
}

String MeasurementPoint::getName() const {
    return name;
}

int16_t MeasurementPoint::getCurrentTemp() const {
    return currentTemp;
}

int16_t MeasurementPoint::getMinTemp() const {
    return minTemp;
}

int16_t MeasurementPoint::getMaxTemp() const {
    return maxTemp;
}

int16_t MeasurementPoint::getLowAlarmThreshold() const {
    return lowAlarmThreshold;
}

int16_t MeasurementPoint::getHighAlarmThreshold() const {
    return highAlarmThreshold;
}

uint8_t MeasurementPoint::getAlarmStatus() const {
    return alarmStatus;
}

uint8_t MeasurementPoint::getErrorStatus() const {
    return errorStatus;
}

void MeasurementPoint::setName(const String& newName) {
    name = newName;
}

void MeasurementPoint::setLowAlarmThreshold(int16_t threshold) {
    lowAlarmThreshold = threshold;
    updateAlarmStatus();
}

void MeasurementPoint::setHighAlarmThreshold(int16_t threshold) {
    highAlarmThreshold = threshold;
    updateAlarmStatus();
}

void MeasurementPoint::bindSensor(Sensor* sensor) {
    boundSensor = sensor;
}

void MeasurementPoint::unbindSensor() {
    boundSensor = nullptr;
}

Sensor* MeasurementPoint::getBoundSensor() const {
    return boundSensor;
}

void MeasurementPoint::update() {
    if (boundSensor) {
        // Example: ask the sensor for the latest temperature
        if (boundSensor->readTemperature()) {
            currentTemp = boundSensor->getCurrentTemp();
            if (currentTemp < minTemp) minTemp = currentTemp;
            if (currentTemp > maxTemp) maxTemp = currentTemp;
            errorStatus = boundSensor->getErrorStatus();
        } else {
            errorStatus = boundSensor->getErrorStatus();
        }
    } else {
        errorStatus = 0x01; // Example: error code for "not bound"
    }
    updateAlarmStatus();
}

void MeasurementPoint::resetMinMaxTemp() {
    minTemp = currentTemp;
    maxTemp = currentTemp;
}

void MeasurementPoint::updateAlarmStatus() {
    alarmStatus = 0;
    if (errorStatus != 0) return;
    if (currentTemp < lowAlarmThreshold) alarmStatus |= 0x01; // Low alarm bit
    if (currentTemp > highAlarmThreshold) alarmStatus |= 0x02; // High alarm bit
}
