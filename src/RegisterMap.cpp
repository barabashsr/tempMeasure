/**
 * @file RegisterMap.cpp
 * @brief Implementation of Modbus register mapping for temperature monitoring
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements the register map functionality for Modbus communication,
 *          providing access to temperature data, device status, and configuration.
 * 
 * @section dependencies Dependencies
 * - RegisterMap.h for class definition
 * - MeasurementPoint.h for temperature data interface
 * 
 * @section hardware Hardware Requirements
 * - Modbus RTU/TCP communication interface
 * - Temperature sensors (DS18B20, PT1000)
 */

#include "RegisterMap.h"

RegisterMap::RegisterMap() {
    deviceId = 1000;
    firmwareVersion = 0x0100;
    numActiveDS18B20 = 0;
    numActivePT1000 = 0;
    for (int i = 0; i < 7; i++) deviceStatus[i] = 0;
    for (int i = 0; i < 60; i++) {
        currentTemps[i] = 0;
        minTemps[i] = 32767;
        maxTemps[i] = -32768;
        alarmStatus[i] = 0;
        errorStatus[i] = 0;
        lowAlarmThresholds[i] = -10;
        highAlarmThresholds[i] = 50;
    }
}

bool RegisterMap::isValidAddress(uint16_t address) {
    if (address <= DEVICE_STATUS_END_REG) return true;
    if (address >= CURRENT_TEMP_DS18B20_START_REG && address <= CURRENT_TEMP_PT1000_END_REG) return true;
    if (address >= MIN_TEMP_DS18B20_START_REG && address <= MIN_TEMP_PT1000_END_REG) return true;
    if (address >= MAX_TEMP_DS18B20_START_REG && address <= MAX_TEMP_PT1000_END_REG) return true;
    if (address >= ALARM_STATUS_DS18B20_START_REG && address <= ALARM_STATUS_PT1000_END_REG) return true;
    if (address >= ERROR_STATUS_DS18B20_START_REG && address <= ERROR_STATUS_PT1000_END_REG) return true;
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) return true;
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) return true;
    return false;
}

bool RegisterMap::isReadOnlyRegister(uint16_t address) {
    // Only alarm thresholds are writable
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) return false;
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) return false;
    return true;
}

uint16_t RegisterMap::readHoldingRegister(uint16_t address) {
    if (!isValidAddress(address)) return 0xFFFF;

    if (address == DEVICE_ID_REG) return deviceId;
    if (address == FIRMWARE_VERSION_REG) return firmwareVersion;
    if (address == NUM_DS18B20_REG) return numActiveDS18B20;
    if (address == NUM_PT1000_REG) return numActivePT1000;
    if (address >= DEVICE_STATUS_START_REG && address <= DEVICE_STATUS_END_REG)
        return deviceStatus[address - DEVICE_STATUS_START_REG];

    // DS18B20 and PT1000 share the same arrays, just different index offsets
    if (address >= CURRENT_TEMP_DS18B20_START_REG && address <= CURRENT_TEMP_PT1000_END_REG)
        return currentTemps[address - CURRENT_TEMP_DS18B20_START_REG];
    if (address >= MIN_TEMP_DS18B20_START_REG && address <= MIN_TEMP_PT1000_END_REG)
        return minTemps[address - MIN_TEMP_DS18B20_START_REG];
    if (address >= MAX_TEMP_DS18B20_START_REG && address <= MAX_TEMP_PT1000_END_REG)
        return maxTemps[address - MAX_TEMP_DS18B20_START_REG];
    if (address >= ALARM_STATUS_DS18B20_START_REG && address <= ALARM_STATUS_PT1000_END_REG)
        return alarmStatus[address - ALARM_STATUS_DS18B20_START_REG];
    if (address >= ERROR_STATUS_DS18B20_START_REG && address <= ERROR_STATUS_PT1000_END_REG)
        return errorStatus[address - ERROR_STATUS_DS18B20_START_REG];
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG)
        return lowAlarmThresholds[address - LOW_ALARM_DS18B20_START_REG];
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG)
        return highAlarmThresholds[address - HIGH_ALARM_DS18B20_START_REG];

    return 0xFFFF;
}

bool RegisterMap::writeHoldingRegister(uint16_t address, uint16_t value) {
    if (!isValidAddress(address)) return false;
    if (isReadOnlyRegister(address)) return false;

    // Only alarm thresholds are writable
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        lowAlarmThresholds[address - LOW_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        highAlarmThresholds[address - HIGH_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    return false;
}

void RegisterMap::updateFromMeasurementPoint(const MeasurementPoint& point) {
    uint8_t idx = point.getAddress();
    // DS18B20: 0-49, PT1000: 50-59
    if (idx < 60) {
        currentTemps[idx] = point.getCurrentTemp();
        minTemps[idx] = point.getMinTemp();
        maxTemps[idx] = point.getMaxTemp();
        alarmStatus[idx] = point.getAlarmStatus();
        errorStatus[idx] = point.getErrorStatus();
        // Thresholds are updated by config methods, not here
    }
}

void RegisterMap::applyConfigToMeasurementPoint(MeasurementPoint& point) {
    uint8_t idx = point.getAddress();
    if (idx < 60) {
        point.setLowAlarmThreshold(lowAlarmThresholds[idx]);
        point.setHighAlarmThreshold(highAlarmThresholds[idx]);
        //Serial.printf("applyConfigToMeasurementPoint(%d): LAS: %d, HAS: %d\n", idx, lowAlarmThresholds[idx], highAlarmThresholds[idx]);
    }
}

void RegisterMap::applyConfigFromMeasurementPoint(const MeasurementPoint& point) {
    uint8_t idx = point.getAddress();
    if (idx < 60) {
        lowAlarmThresholds[idx] = point.getLowAlarmThreshold();
        highAlarmThresholds[idx] = point.getHighAlarmThreshold();
        //Serial.printf("applyConfigFromMeasurementPoint(%d): LAS: %d, HAS: %d\n", idx, lowAlarmThresholds[idx], highAlarmThresholds[idx]);
    }
}
