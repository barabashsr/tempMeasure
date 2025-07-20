/**
 * @file MeasurementPoint.cpp
 * @brief Implementation of temperature measurement point management
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements MeasurementPoint class for managing individual temperature
 *          monitoring locations with sensor binding, alarm management, and data tracking.
 * 
 * @section dependencies Dependencies
 * - MeasurementPoint.h for class definition
 * - Sensor.h for temperature sensor interface
 * 
 * @section features Features
 * - Sensor binding and unbinding
 * - Temperature value tracking (current/min/max)
 * - Alarm threshold management
 * - Error status monitoring
 * - Temperature reading from bound sensors
 */

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
      //oneWireBus(0)
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
    if (newName != name) {
        String oldName = name.isEmpty() ? "Point_" + String(address) : name;
        name = newName;
        LoggerManager::info("POINT_CONFIG", 
            "Point " + String(address) + " name changed from '" + 
            oldName + "' to '" + name + "'");
    }
    //name = newName;
}

void MeasurementPoint::setLowAlarmThreshold(int16_t threshold) {

    if (lowAlarmThreshold != threshold) {
        LoggerManager::info("POINT_CONFIG", 
            "Point " + String(address) + " (" + name + 
            ") low alarm threshold changed from " + String(lowAlarmThreshold) + 
            "°C to " + String(threshold) + "°C");
        lowAlarmThreshold = threshold;
    }

    //lowAlarmThreshold = threshold;
    updateAlarmStatus();
}

void MeasurementPoint::setHighAlarmThreshold(int16_t threshold) {
    if (highAlarmThreshold != threshold) {
        LoggerManager::info("POINT_CONFIG", 
            "Point " + String(address) + " (" + name + 
            ") high alarm threshold changed from " + String(highAlarmThreshold) + 
            "°C to " + String(threshold) + "°C");
        highAlarmThreshold = threshold;
    }
    //highAlarmThreshold = threshold;
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
    if (boundSensor != nullptr) {
        // Example: ask the sensor for the latest temperature
        //if (boundSensor->readTemperature()) {
            currentTemp = boundSensor->getCurrentTemp();
            //Serial.printf("\nPoint: %d. %s. Sensor: %s. Temp: %d\n", getAddress(), getName(), boundSensor->getName(), currentTemp);
            if (currentTemp < minTemp) minTemp = currentTemp;
            if (currentTemp > maxTemp) maxTemp = currentTemp;
            errorStatus = boundSensor->getErrorStatus();
        //} else {
            //errorStatus = boundSensor->getErrorStatus();
        //}
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

// void MeasurementPoint::setOneWireBus(uint8_t bus) {
//     oneWireBus = bus;

// }
// uint8_t MeasurementPoint::getOneWireBus() {
//     return oneWireBus;

// }
