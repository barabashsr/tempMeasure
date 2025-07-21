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
    commandRegister = 0;
    commandPending = false;
    
    for (int i = 0; i < 7; i++) deviceStatus[i] = 0;
    for (int i = 0; i < 60; i++) {
        currentTemps[i] = 0;
        minTemps[i] = 32767;
        maxTemps[i] = -32768;
        alarmStatus[i] = 0;
        errorStatus[i] = 0;
        lowAlarmThresholds[i] = -10;
        highAlarmThresholds[i] = 50;
        // Initialize alarm config with all alarms disabled, medium priority
        alarmConfig[i] = (1 << ALARM_CONFIG_LOW_PRIORITY_SHIFT) | 
                        (1 << ALARM_CONFIG_HIGH_PRIORITY_SHIFT) | 
                        (1 << ALARM_CONFIG_ERROR_PRIORITY_SHIFT);
    }
    
    // Initialize relay control (all auto mode)
    for (int i = 0; i < 6; i++) {
        relayControl[i] = 0;
    }
    
    // Initialize relay status registers
    for (int i = 0; i < 3; i++) {
        relayStatus[i] = 0;
    }
    
    // Initialize hysteresis to 5 degrees
    for (int i = 0; i < 20; i++) {
        hysteresis[i] = 50; // 5.0 degrees in 0.1 degree units
    }
}

bool RegisterMap::isValidAddress(uint16_t address) {
    if (address <= DEVICE_STATUS_END_REG) return true;
    if (address >= RELAY_STATUS_REG_START && address <= RELAY_STATUS_REG_END) return true;
    if (address >= CURRENT_TEMP_DS18B20_START_REG && address <= CURRENT_TEMP_PT1000_END_REG) return true;
    if (address >= MIN_TEMP_DS18B20_START_REG && address <= MIN_TEMP_PT1000_END_REG) return true;
    if (address >= MAX_TEMP_DS18B20_START_REG && address <= MAX_TEMP_PT1000_END_REG) return true;
    if (address >= ALARM_STATUS_DS18B20_START_REG && address <= ALARM_STATUS_PT1000_END_REG) return true;
    if (address >= ERROR_STATUS_DS18B20_START_REG && address <= ERROR_STATUS_PT1000_END_REG) return true;
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) return true;
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) return true;
    if (address >= ALARM_CONFIG_DS18B20_START_REG && address <= ALARM_CONFIG_PT1000_END_REG) return true;
    if (address >= RELAY_CONTROL_START_REG && address <= RELAY_CONTROL_END_REG) return true;
    if (address >= HYSTERESIS_START_REG && address <= HYSTERESIS_END_REG) return true;
    if (address == COMMAND_REG) return true;
    return false;
}

bool RegisterMap::isReadOnlyRegister(uint16_t address) {
    // Writable registers
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) return false;
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) return false;
    if (address >= ALARM_CONFIG_DS18B20_START_REG && address <= ALARM_CONFIG_PT1000_END_REG) return false;
    if (address >= RELAY_CONTROL_START_REG && address <= RELAY_CONTROL_START_REG + 2) return false; // Only control, not status
    if (address >= HYSTERESIS_START_REG && address <= HYSTERESIS_END_REG) return false;
    if (address == COMMAND_REG) return false;
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
    if (address >= RELAY_STATUS_REG_START && address <= RELAY_STATUS_REG_END)
        return relayStatus[address - RELAY_STATUS_REG_START];

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
    
    // Alarm configuration registers
    if (address >= ALARM_CONFIG_DS18B20_START_REG && address <= ALARM_CONFIG_PT1000_END_REG)
        return alarmConfig[address - ALARM_CONFIG_DS18B20_START_REG];
    
    // Relay control and status registers
    if (address >= RELAY_CONTROL_START_REG && address <= RELAY_CONTROL_END_REG)
        return relayControl[address - RELAY_CONTROL_START_REG];
    
    // Hysteresis registers
    if (address >= HYSTERESIS_START_REG && address <= HYSTERESIS_END_REG)
        return hysteresis[address - HYSTERESIS_START_REG];
    
    // Command register
    if (address == COMMAND_REG)
        return commandRegister;

    return 0xFFFF;
}

bool RegisterMap::writeHoldingRegister(uint16_t address, uint16_t value) {
    if (!isValidAddress(address)) return false;
    if (isReadOnlyRegister(address)) return false;

    // Alarm thresholds
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        lowAlarmThresholds[address - LOW_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        highAlarmThresholds[address - HIGH_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    
    // Alarm configuration
    if (address >= ALARM_CONFIG_DS18B20_START_REG && address <= ALARM_CONFIG_PT1000_END_REG) {
        alarmConfig[address - ALARM_CONFIG_DS18B20_START_REG] = value;
        return true;
    }
    
    // Relay control (only control registers, not status)
    if (address >= RELAY_CONTROL_START_REG && address <= RELAY_CONTROL_START_REG + 2) {
        relayControl[address - RELAY_CONTROL_START_REG] = value;
        return true;
    }
    
    // Hysteresis
    if (address >= HYSTERESIS_START_REG && address <= HYSTERESIS_END_REG) {
        hysteresis[address - HYSTERESIS_START_REG] = value;
        return true;
    }
    
    // Command register
    if (address == COMMAND_REG) {
        commandRegister = value;
        commandPending = true;
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

uint16_t RegisterMap::getAlarmConfig(uint8_t pointIndex) const {
    if (pointIndex < 60) {
        return alarmConfig[pointIndex];
    }
    return 0;
}

void RegisterMap::setRelayControl(uint8_t relayIndex, uint16_t mode) {
    if (relayIndex < 3) {
        relayControl[relayIndex] = mode;
    }
}

uint16_t RegisterMap::getRelayControl(uint8_t relayIndex) const {
    if (relayIndex < 3) {
        return relayControl[relayIndex];
    }
    return 0;
}

void RegisterMap::setRelayStatus(uint8_t relayIndex, bool state) {
    if (relayIndex < 3) {
        relayControl[3 + relayIndex] = state ? 1 : 0;
    }
}

bool RegisterMap::getRelayStatus(uint8_t relayIndex) const {
    if (relayIndex < 3) {
        return relayControl[3 + relayIndex] != 0;
    }
    return false;
}

void RegisterMap::updateRelayStatusRegister(uint8_t relayIndex, bool commandedState, bool actualState) {
    if (relayIndex >= 3) return;
    
    // Relay status register format:
    // Bit 0: Commanded state (what system wants relay to be)
    // Bit 1: Actual state (current hardware state)
    uint16_t statusValue = 0;
    if (commandedState) statusValue |= 0x0001;
    if (actualState) statusValue |= 0x0002;
    
    relayStatus[relayIndex] = statusValue;
}
