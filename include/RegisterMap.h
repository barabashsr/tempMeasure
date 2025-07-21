/**
 * @file RegisterMap.h
 * @brief Modbus register mapping for temperature monitoring system
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Defines the Modbus register layout for external communication with the
 *          temperature monitoring system. Provides register addresses and access methods
 *          for device information, temperature data, and configuration parameters.
 * 
 * @section dependencies Dependencies
 * - MeasurementPoint.h for temperature data interface
 * 
 * @section hardware Hardware Requirements
 * - Modbus RTU/TCP communication interface
 * - Support for 60 total measurement points (50 DS18B20 + 10 PT1000)
 * 
 * @section register_layout Register Layout
 * - 0-99: Device Information (ID, version, status)
 * - 100-199: Current temperature values
 * - 200-299: Minimum temperature values
 * - 300-399: Maximum temperature values
 * - 400-499: Alarm status flags
 * - 500-599: Error status flags
 * - 600-699: Low temperature alarm thresholds
 * - 700-799: High temperature alarm thresholds
 * - 800-859: Alarm configuration (enable flags and priorities)
 * - 860-869: Relay control
 * - 870-889: Hysteresis configuration
 * - 899: Command register
 */

#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>
#include "MeasurementPoint.h"

/**
 * @class RegisterMap
 * @brief Manages Modbus register mapping for temperature monitoring
 * @details Provides a standardized register interface for external systems to
 *          access temperature data, device status, and configuration parameters.
 *          Implements Modbus holding register protocol with read/write capabilities.
 */
class RegisterMap {
private:
    // Device Information Registers (0-99)
    uint16_t deviceId;                    ///< Device identifier (register 0)
    uint16_t firmwareVersion;             ///< Firmware version (register 1)
    uint16_t numActiveDS18B20;            ///< Number of active DS18B20 sensors (register 2)
    uint16_t numActivePT1000;             ///< Number of active PT1000 sensors (register 3)
    uint16_t deviceStatus[7];             ///< Device status flags (registers 4-10)
    uint16_t relayStatus[3];              ///< Relay status (registers 11-13) bit0: commanded, bit1: actual

    // Temperature Data Registers (100-599)
    int16_t currentTemps[60];             ///< Current temperature values (registers 100-159)
    int16_t minTemps[60];                 ///< Minimum temperature values (registers 200-259)
    int16_t maxTemps[60];                 ///< Maximum temperature values (registers 300-359)
    uint16_t alarmStatus[60];             ///< Alarm status flags (registers 400-459)
    uint16_t errorStatus[60];             ///< Error status flags (registers 500-559)

    // Configuration Registers (600-799)
    int16_t lowAlarmThresholds[60];      ///< Low temperature alarm thresholds (registers 600-659)
    int16_t highAlarmThresholds[60];     ///< High temperature alarm thresholds (registers 700-759)

    // Alarm Control Registers (800-899)
    uint16_t alarmConfig[60];             ///< Alarm configuration (enable flags and priorities) (registers 800-859)
    uint16_t relayControl[6];             ///< Relay control and status (registers 860-865)
    uint16_t hysteresis[20];              ///< Hysteresis values (registers 870-889)
    uint16_t commandRegister;             ///< Command execution register (register 899)
    bool commandPending;                  ///< Flag indicating command needs processing

    // Helpers
    /**
     * @brief Check if register address is valid
     * @param[in] address Register address to validate
     * @return true if address is within valid range
     */
    bool isValidAddress(uint16_t address);
    
    /**
     * @brief Check if register is read-only
     * @param[in] address Register address to check
     * @return true if register is read-only
     */
    bool isReadOnlyRegister(uint16_t address);

public:
    /**
     * @brief Construct a new Register Map object
     * @details Initializes all registers to default values
     */
    RegisterMap();

    // Register read/write
    /**
     * @brief Read value from Modbus holding register
     * @param[in] address Register address to read
     * @return uint16_t Register value or 0xFFFF if invalid address
     */
    uint16_t readHoldingRegister(uint16_t address);
    
    /**
     * @brief Write value to Modbus holding register
     * @param[in] address Register address to write
     * @param[in] value Value to write
     * @return true if write successful
     * @return false if address invalid or register is read-only
     */
    bool writeHoldingRegister(uint16_t address, uint16_t value);

    // Update register map from measurement point data
    /**
     * @brief Update registers from measurement point data
     * @param[in] point Measurement point containing temperature data
     * @details Updates current, min, max temperatures and status flags
     */
    void updateFromMeasurementPoint(const MeasurementPoint& point);

    // Apply config (thresholds) to and from measurement points
    /**
     * @brief Apply configuration from registers to measurement point
     * @param[in,out] point Measurement point to update with register values
     * @details Updates alarm thresholds in measurement point
     */
    void applyConfigToMeasurementPoint(MeasurementPoint& point);
    
    /**
     * @brief Apply configuration from measurement point to registers
     * @param[in] point Measurement point containing configuration
     * @details Updates register thresholds from measurement point
     */
    void applyConfigFromMeasurementPoint(const MeasurementPoint& point);

    // Utility methods for device info
    /**
     * @brief Increment active DS18B20 sensor count
     */
    void incrementActiveDS18B20() { numActiveDS18B20++; }
    
    /**
     * @brief Decrement active DS18B20 sensor count
     */
    void decrementActiveDS18B20() { if (numActiveDS18B20 > 0) numActiveDS18B20--; }
    
    /**
     * @brief Increment active PT1000 sensor count
     */
    void incrementActivePT1000() { numActivePT1000++; }
    
    /**
     * @brief Decrement active PT1000 sensor count
     */
    void decrementActivePT1000() { if (numActivePT1000 > 0) numActivePT1000--; }

    /**
     * @brief Get device ID
     * @return uint16_t Device identifier
     */
    uint16_t getDeviceId() const { return deviceId; }
    
    /**
     * @brief Get firmware version
     * @return uint16_t Firmware version number
     */
    uint16_t getFirmwareVersion() const { return firmwareVersion; }
    
    /**
     * @brief Get number of active DS18B20 sensors
     * @return uint16_t Active DS18B20 count
     */
    uint16_t getNumActiveDS18B20() const { return numActiveDS18B20; }
    
    /**
     * @brief Get number of active PT1000 sensors
     * @return uint16_t Active PT1000 count
     */
    uint16_t getNumActivePT1000() const { return numActivePT1000; }

    /**
     * @brief Check if a command is pending execution
     * @return true if command needs to be processed
     */
    bool isCommandPending() const { return commandPending; }
    
    /**
     * @brief Get the pending command value
     * @return uint16_t Command value
     */
    uint16_t getPendingCommand() const { return commandRegister; }
    
    /**
     * @brief Clear the pending command flag
     */
    void clearPendingCommand() { commandPending = false; commandRegister = 0; }
    
    /**
     * @brief Get alarm configuration for a point
     * @param[in] pointIndex Point index (0-59)
     * @return uint16_t Alarm configuration register value
     */
    uint16_t getAlarmConfig(uint8_t pointIndex) const;
    
    /**
     * @brief Set relay control mode
     * @param[in] relayIndex Relay index (0-2)
     * @param[in] mode Control mode (0=Auto, 1=Force Off, 2=Force On)
     */
    void setRelayControl(uint8_t relayIndex, uint16_t mode);
    
    /**
     * @brief Get relay control mode
     * @param[in] relayIndex Relay index (0-2)
     * @return uint16_t Control mode
     */
    uint16_t getRelayControl(uint8_t relayIndex) const;
    
    /**
     * @brief Set relay status
     * @param[in] relayIndex Relay index (0-2)
     * @param[in] state Relay state (0=Off, 1=On)
     */
    void setRelayStatus(uint8_t relayIndex, bool state);
    
    /**
     * @brief Get relay status
     * @param[in] relayIndex Relay index (0-2)
     * @return bool Relay state
     */
    bool getRelayStatus(uint8_t relayIndex) const;
    
    /**
     * @brief Update relay status register with commanded and actual states
     * @param[in] relayIndex Relay index (0-2)
     * @param[in] commandedState What the system wants the relay to be
     * @param[in] actualState Current hardware state of the relay
     */
    void updateRelayStatusRegister(uint8_t relayIndex, bool commandedState, bool actualState);

    /**
     * @name Register Address Constants
     * @brief Modbus register address definitions
     * @{
     */
    
    // Device Information Registers (0-99)
    static const uint16_t DEVICE_ID_REG = 0;              ///< Device ID register
    static const uint16_t FIRMWARE_VERSION_REG = 1;       ///< Firmware version register
    static const uint16_t NUM_DS18B20_REG = 2;           ///< Active DS18B20 count register
    static const uint16_t NUM_PT1000_REG = 3;            ///< Active PT1000 count register
    static const uint16_t DEVICE_STATUS_START_REG = 4;   ///< Device status start register
    static const uint16_t DEVICE_STATUS_END_REG = 10;    ///< Device status end register
    static const uint16_t RELAY_STATUS_REG_START = 11;   ///< Relay status registers start
    static const uint16_t RELAY_STATUS_REG_END = 13;     ///< Relay status registers end
    
    // Current Temperature Registers (100-199)
    static const uint16_t CURRENT_TEMP_DS18B20_START_REG = 100;  ///< DS18B20 current temp start
    static const uint16_t CURRENT_TEMP_DS18B20_END_REG = 149;    ///< DS18B20 current temp end
    static const uint16_t CURRENT_TEMP_PT1000_START_REG = 150;   ///< PT1000 current temp start
    static const uint16_t CURRENT_TEMP_PT1000_END_REG = 159;     ///< PT1000 current temp end
    
    // Minimum Temperature Registers (200-299)
    static const uint16_t MIN_TEMP_DS18B20_START_REG = 200;      ///< DS18B20 min temp start
    static const uint16_t MIN_TEMP_DS18B20_END_REG = 249;        ///< DS18B20 min temp end
    static const uint16_t MIN_TEMP_PT1000_START_REG = 250;       ///< PT1000 min temp start
    static const uint16_t MIN_TEMP_PT1000_END_REG = 259;         ///< PT1000 min temp end
    
    // Maximum Temperature Registers (300-399)
    static const uint16_t MAX_TEMP_DS18B20_START_REG = 300;      ///< DS18B20 max temp start
    static const uint16_t MAX_TEMP_DS18B20_END_REG = 349;        ///< DS18B20 max temp end
    static const uint16_t MAX_TEMP_PT1000_START_REG = 350;       ///< PT1000 max temp start
    static const uint16_t MAX_TEMP_PT1000_END_REG = 359;         ///< PT1000 max temp end
    
    // Alarm Status Registers (400-499)
    static const uint16_t ALARM_STATUS_DS18B20_START_REG = 400;  ///< DS18B20 alarm status start
    static const uint16_t ALARM_STATUS_DS18B20_END_REG = 449;    ///< DS18B20 alarm status end
    static const uint16_t ALARM_STATUS_PT1000_START_REG = 450;   ///< PT1000 alarm status start
    static const uint16_t ALARM_STATUS_PT1000_END_REG = 459;     ///< PT1000 alarm status end
    
    // Error Status Registers (500-599)
    static const uint16_t ERROR_STATUS_DS18B20_START_REG = 500;  ///< DS18B20 error status start
    static const uint16_t ERROR_STATUS_DS18B20_END_REG = 549;    ///< DS18B20 error status end
    static const uint16_t ERROR_STATUS_PT1000_START_REG = 550;   ///< PT1000 error status start
    static const uint16_t ERROR_STATUS_PT1000_END_REG = 559;     ///< PT1000 error status end
    
    // Low Alarm Threshold Registers (600-699)
    static const uint16_t LOW_ALARM_DS18B20_START_REG = 600;     ///< DS18B20 low alarm start
    static const uint16_t LOW_ALARM_DS18B20_END_REG = 649;       ///< DS18B20 low alarm end
    static const uint16_t LOW_ALARM_PT1000_START_REG = 650;      ///< PT1000 low alarm start
    static const uint16_t LOW_ALARM_PT1000_END_REG = 659;        ///< PT1000 low alarm end
    
    // High Alarm Threshold Registers (700-799)
    static const uint16_t HIGH_ALARM_DS18B20_START_REG = 700;    ///< DS18B20 high alarm start
    static const uint16_t HIGH_ALARM_DS18B20_END_REG = 749;      ///< DS18B20 high alarm end
    static const uint16_t HIGH_ALARM_PT1000_START_REG = 750;     ///< PT1000 high alarm start
    static const uint16_t HIGH_ALARM_PT1000_END_REG = 759;       ///< PT1000 high alarm end
    
    // Alarm Configuration Registers (800-859)
    static const uint16_t ALARM_CONFIG_DS18B20_START_REG = 800;   ///< DS18B20 alarm config start
    static const uint16_t ALARM_CONFIG_DS18B20_END_REG = 849;     ///< DS18B20 alarm config end
    static const uint16_t ALARM_CONFIG_PT1000_START_REG = 850;    ///< PT1000 alarm config start
    static const uint16_t ALARM_CONFIG_PT1000_END_REG = 859;      ///< PT1000 alarm config end
    
    // Relay Control Registers (860-869)
    static const uint16_t RELAY_CONTROL_START_REG = 860;          ///< Relay control start
    static const uint16_t RELAY_STATUS_START_REG = 863;           ///< Relay status start
    static const uint16_t RELAY_CONTROL_END_REG = 865;            ///< Relay control/status end
    
    // Hysteresis Registers (870-889)
    static const uint16_t HYSTERESIS_START_REG = 870;             ///< Hysteresis start
    static const uint16_t HYSTERESIS_END_REG = 889;               ///< Hysteresis end
    
    // Command Register
    static const uint16_t COMMAND_REG = 899;                      ///< Command execution register
    
    // Alarm Configuration Bit Masks
    static const uint16_t ALARM_CONFIG_LOW_ENABLE_BIT = 0x0001;   ///< Bit 0: Low temp alarm enable
    static const uint16_t ALARM_CONFIG_HIGH_ENABLE_BIT = 0x0002;  ///< Bit 1: High temp alarm enable
    static const uint16_t ALARM_CONFIG_ERROR_ENABLE_BIT = 0x0004; ///< Bit 2: Sensor error alarm enable
    static const uint16_t ALARM_CONFIG_LOW_PRIORITY_MASK = 0x0018;  ///< Bits 3-4: Low priority
    static const uint16_t ALARM_CONFIG_HIGH_PRIORITY_MASK = 0x0060; ///< Bits 5-6: High priority
    static const uint16_t ALARM_CONFIG_ERROR_PRIORITY_MASK = 0x0180;///< Bits 7-8: Error priority
    static const uint16_t ALARM_CONFIG_LOW_PRIORITY_SHIFT = 3;    ///< Shift for low priority bits
    static const uint16_t ALARM_CONFIG_HIGH_PRIORITY_SHIFT = 5;   ///< Shift for high priority bits
    static const uint16_t ALARM_CONFIG_ERROR_PRIORITY_SHIFT = 7;  ///< Shift for error priority bits
    
    // Command Values
    static const uint16_t CMD_APPLY_ALARM_CONFIG = 0x0001;        ///< Apply alarm configuration
    
    /** @} */ // end of Register Address Constants
};

/**
 * @enum RelayControlMode
 * @brief Defines control modes for relay operation
 * @details Used to specify whether a relay should be controlled automatically
 *          based on alarm states or forced to a specific state via Modbus
 */
enum class RelayControlMode : uint16_t {
    AUTO = 0,        ///< Automatic control based on alarm states
    FORCE_OFF = 1,   ///< Force relay off regardless of alarms
    FORCE_ON = 2     ///< Force relay on regardless of alarms
};

#endif // REGISTER_MAP_H
