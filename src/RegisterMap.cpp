#include "RegisterMap.h"

RegisterMap::RegisterMap() {
    // Initialize device information registers
    deviceId = 1000;  // Example model number
    firmwareVersion = 0x0100;  // Version 1.0
    numActiveDS18B20 = 0;
    numActivePT1000 = 0;
    
    // Initialize device status registers to 0
    for (int i = 0; i < 7; i++) {
        deviceStatus[i] = 0;
    }
    
    // Initialize all temperature and status registers
    for (int i = 0; i < 60; i++) {
        currentTemps[i] = 0;
        minTemps[i] = 32767;  // Max value for int16_t
        maxTemps[i] = -32768; // Min value for int16_t
        alarmStatus[i] = 0;
        errorStatus[i] = 0;
        
        // Set default alarm thresholds
        lowAlarmThresholds[i] = -10;  // Default low alarm at -10°C
        highAlarmThresholds[i] = 50;  // Default high alarm at 50°C
    }
}

bool RegisterMap::isValidAddress(uint16_t address) {
    // Check if the address is within valid ranges
    if (address <= DEVICE_STATUS_END_REG) {
        return true;
    }
    
    if (address >= CURRENT_TEMP_DS18B20_START_REG && address <= CURRENT_TEMP_PT1000_END_REG) {
        return true;
    }
    
    if (address >= MIN_TEMP_DS18B20_START_REG && address <= MIN_TEMP_PT1000_END_REG) {
        return true;
    }
    
    if (address >= MAX_TEMP_DS18B20_START_REG && address <= MAX_TEMP_PT1000_END_REG) {
        return true;
    }
    
    if (address >= ALARM_STATUS_DS18B20_START_REG && address <= ALARM_STATUS_PT1000_END_REG) {
        return true;
    }
    
    if (address >= ERROR_STATUS_DS18B20_START_REG && address <= ERROR_STATUS_PT1000_END_REG) {
        return true;
    }
    
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        return true;
    }
    
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        return true;
    }
    
    return false;
}

bool RegisterMap::isReadOnlyRegister(uint16_t address) {
    // All registers are read-only except for alarm thresholds
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        return false;
    }
    
    if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        return false;
    }
    
    return true;
}

uint16_t RegisterMap::readHoldingRegister(uint16_t address) {
    if (!isValidAddress(address)) {
        return 0xFFFF;  // Invalid address
    }
    
    // Device Information Registers
    if (address == DEVICE_ID_REG) {
        return deviceId;
    } else if (address == FIRMWARE_VERSION_REG) {
        return firmwareVersion;
    } else if (address == NUM_DS18B20_REG) {
        return numActiveDS18B20;
    } else if (address == NUM_PT1000_REG) {
        return numActivePT1000;
    } else if (address >= DEVICE_STATUS_START_REG && address <= DEVICE_STATUS_END_REG) {
        return deviceStatus[address - DEVICE_STATUS_START_REG];
    }
    
    // Current Temperature Registers
    else if (address >= CURRENT_TEMP_DS18B20_START_REG && address <= CURRENT_TEMP_PT1000_END_REG) {
        return currentTemps[address - CURRENT_TEMP_DS18B20_START_REG];
    }
    
    // Min Temperature Registers
    else if (address >= MIN_TEMP_DS18B20_START_REG && address <= MIN_TEMP_PT1000_END_REG) {
        return minTemps[address - MIN_TEMP_DS18B20_START_REG];
    }
    
    // Max Temperature Registers
    else if (address >= MAX_TEMP_DS18B20_START_REG && address <= MAX_TEMP_PT1000_END_REG) {
        return maxTemps[address - MAX_TEMP_DS18B20_START_REG];
    }
    
    // Alarm Status Registers
    else if (address >= ALARM_STATUS_DS18B20_START_REG && address <= ALARM_STATUS_PT1000_END_REG) {
        return alarmStatus[address - ALARM_STATUS_DS18B20_START_REG];
    }
    
    // Error Status Registers
    else if (address >= ERROR_STATUS_DS18B20_START_REG && address <= ERROR_STATUS_PT1000_END_REG) {
        return errorStatus[address - ERROR_STATUS_DS18B20_START_REG];
    }
    
    // Low Alarm Threshold Registers
    else if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        return lowAlarmThresholds[address - LOW_ALARM_DS18B20_START_REG];
    }
    
    // High Alarm Threshold Registers
    else if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        return highAlarmThresholds[address - HIGH_ALARM_DS18B20_START_REG];
    }
    
    return 0xFFFF;  // Should never reach here if isValidAddress works correctly
}

bool RegisterMap::writeHoldingRegister(uint16_t address, uint16_t value) {
    if (!isValidAddress(address)) {
        return false;  // Invalid address
    }
    
    if (isReadOnlyRegister(address)) {
        return false;  // Cannot write to read-only register
    }
    
    // Low Alarm Threshold Registers
    if (address >= LOW_ALARM_DS18B20_START_REG && address <= LOW_ALARM_PT1000_END_REG) {
        // Cast to int16_t for temperature value
        lowAlarmThresholds[address - LOW_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    
    // High Alarm Threshold Registers
    else if (address >= HIGH_ALARM_DS18B20_START_REG && address <= HIGH_ALARM_PT1000_END_REG) {
        // Cast to int16_t for temperature value
        highAlarmThresholds[address - HIGH_ALARM_DS18B20_START_REG] = static_cast<int16_t>(value);
        return true;
    }
    
    return false;  // Should never reach here if isValidAddress and isReadOnlyRegister work correctly
}

void RegisterMap::updateFromSensor(const Sensor& sensor) {
    uint8_t sensorAddress = sensor.getAddress();
    uint16_t index = sensorAddress;
    
    // Update temperature and status registers based on sensor type
    if (sensor.getType() == SensorType::DS18B20) {
        if (index < 50) {  // Valid DS18B20 address range
            currentTemps[index] = sensor.getCurrentTemp();
            minTemps[index] = sensor.getMinTemp();
            maxTemps[index] = sensor.getMaxTemp();
            alarmStatus[index] = sensor.getAlarmStatus();
            errorStatus[index] = sensor.getErrorStatus();
        }
    } else if (sensor.getType() == SensorType::PT1000) {
        if (index >= 50 && index < 60) {  // Valid PT1000 address range
            currentTemps[index] = sensor.getCurrentTemp();
            minTemps[index] = sensor.getMinTemp();
            maxTemps[index] = sensor.getMaxTemp();
            alarmStatus[index] = sensor.getAlarmStatus();
            errorStatus[index] = sensor.getErrorStatus();
        }
    }
}

void RegisterMap::applyConfigToSensor(Sensor& sensor) {
    uint8_t sensorAddress = sensor.getAddress();
    uint16_t index = sensorAddress;
    
    // Apply alarm thresholds based on sensor type
    if (sensor.getType() == SensorType::DS18B20) {
        if (index < 50) {  // Valid DS18B20 address range
            sensor.setLowAlarmThreshold(lowAlarmThresholds[index]);
            sensor.setHighAlarmThreshold(highAlarmThresholds[index]);
        }
    } else if (sensor.getType() == SensorType::PT1000) {
        if (index >= 50 && index < 60) {  // Valid PT1000 address range
            sensor.setLowAlarmThreshold(lowAlarmThresholds[index]);
            sensor.setHighAlarmThreshold(highAlarmThresholds[index]);
        }
    }
}
