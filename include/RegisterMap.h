#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>
#include "MeasurementPoint.h"

class RegisterMap {
private:
    // Device Information Registers (0-99)
    uint16_t deviceId;
    uint16_t firmwareVersion;
    uint16_t numActiveDS18B20;
    uint16_t numActivePT1000;
    uint16_t deviceStatus[7]; // Registers 4-10

    // Temperature Data Registers (100-599)
    int16_t currentTemps[60];    // 100-159
    int16_t minTemps[60];        // 200-259
    int16_t maxTemps[60];        // 300-359
    uint16_t alarmStatus[60];    // 400-459
    uint16_t errorStatus[60];    // 500-559

    // Configuration Registers (600-799)
    int16_t lowAlarmThresholds[60];  // 600-659
    int16_t highAlarmThresholds[60]; // 700-759

    // Helpers
    bool isValidAddress(uint16_t address);
    bool isReadOnlyRegister(uint16_t address);

public:
    RegisterMap();

    // Register read/write
    uint16_t readHoldingRegister(uint16_t address);
    bool writeHoldingRegister(uint16_t address, uint16_t value);

    // Update register map from measurement point data
    void updateFromMeasurementPoint(const MeasurementPoint& point);

    // Apply config (thresholds) to and from measurement points
    void applyConfigToMeasurementPoint(MeasurementPoint& point);
    void applyConfigFromMeasurementPoint(const MeasurementPoint& point);

    // Utility methods for device info
    void incrementActiveDS18B20() { numActiveDS18B20++; }
    void decrementActiveDS18B20() { if (numActiveDS18B20 > 0) numActiveDS18B20--; }
    void incrementActivePT1000() { numActivePT1000++; }
    void decrementActivePT1000() { if (numActivePT1000 > 0) numActivePT1000--; }

    uint16_t getDeviceId() const { return deviceId; }
    uint16_t getFirmwareVersion() const { return firmwareVersion; }
    uint16_t getNumActiveDS18B20() const { return numActiveDS18B20; }
    uint16_t getNumActivePT1000() const { return numActivePT1000; }

    // Register address constants (as in your original code)
    static const uint16_t DEVICE_ID_REG = 0;
    static const uint16_t FIRMWARE_VERSION_REG = 1;
    static const uint16_t NUM_DS18B20_REG = 2;
    static const uint16_t NUM_PT1000_REG = 3;
    static const uint16_t DEVICE_STATUS_START_REG = 4;
    static const uint16_t DEVICE_STATUS_END_REG = 10;
    static const uint16_t CURRENT_TEMP_DS18B20_START_REG = 100;
    static const uint16_t CURRENT_TEMP_DS18B20_END_REG = 149;
    static const uint16_t CURRENT_TEMP_PT1000_START_REG = 150;
    static const uint16_t CURRENT_TEMP_PT1000_END_REG = 159;
    static const uint16_t MIN_TEMP_DS18B20_START_REG = 200;
    static const uint16_t MIN_TEMP_DS18B20_END_REG = 249;
    static const uint16_t MIN_TEMP_PT1000_START_REG = 250;
    static const uint16_t MIN_TEMP_PT1000_END_REG = 259;
    static const uint16_t MAX_TEMP_DS18B20_START_REG = 300;
    static const uint16_t MAX_TEMP_DS18B20_END_REG = 349;
    static const uint16_t MAX_TEMP_PT1000_START_REG = 350;
    static const uint16_t MAX_TEMP_PT1000_END_REG = 359;
    static const uint16_t ALARM_STATUS_DS18B20_START_REG = 400;
    static const uint16_t ALARM_STATUS_DS18B20_END_REG = 449;
    static const uint16_t ALARM_STATUS_PT1000_START_REG = 450;
    static const uint16_t ALARM_STATUS_PT1000_END_REG = 459;
    static const uint16_t ERROR_STATUS_DS18B20_START_REG = 500;
    static const uint16_t ERROR_STATUS_DS18B20_END_REG = 549;
    static const uint16_t ERROR_STATUS_PT1000_START_REG = 550;
    static const uint16_t ERROR_STATUS_PT1000_END_REG = 559;
    static const uint16_t LOW_ALARM_DS18B20_START_REG = 600;
    static const uint16_t LOW_ALARM_DS18B20_END_REG = 649;
    static const uint16_t LOW_ALARM_PT1000_START_REG = 650;
    static const uint16_t LOW_ALARM_PT1000_END_REG = 659;
    static const uint16_t HIGH_ALARM_DS18B20_START_REG = 700;
    static const uint16_t HIGH_ALARM_DS18B20_END_REG = 749;
    static const uint16_t HIGH_ALARM_PT1000_START_REG = 750;
    static const uint16_t HIGH_ALARM_PT1000_END_REG = 759;
};

#endif // REGISTER_MAP_H
