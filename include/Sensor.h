/**
 * @file Sensor.h
 * @brief Temperature sensor abstraction layer
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Provides a unified interface for different temperature sensor types
 *          (DS18B20 and PT1000) with error handling, alarm management, and
 *          temperature tracking capabilities.
 * 
 * @section dependencies Dependencies
 * - OneWire library for DS18B20 communication
 * - DallasTemperature for DS18B20 interface
 * - Adafruit_MAX31865 for PT1000 RTD interface
 * 
 * @section hardware Hardware Requirements
 * - DS18B20: OneWire digital temperature sensors
 * - PT1000: RTD sensors with MAX31865 interface
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MAX31865.h>

/**
 * @enum SensorType
 * @brief Enumeration of supported sensor types
 */
enum class SensorType {
    DS18B20,    ///< Dallas DS18B20 digital temperature sensor
    PT1000      ///< PT1000 RTD temperature sensor
};

/**
 * @name Error Status Bitmasks
 * @brief Bit flags for sensor error conditions
 * @{
 */
constexpr uint8_t ERROR_COMMUNICATION = 0x01;  ///< Communication error with sensor
constexpr uint8_t ERROR_OUT_OF_RANGE  = 0x02;  ///< Temperature reading out of valid range
constexpr uint8_t ERROR_DISCONNECTED  = 0x04;  ///< Sensor physically disconnected
/** @} */

/**
 * @name Alarm Status Bitmasks
 * @brief Bit flags for temperature alarm conditions
 * @{
 */
constexpr uint8_t ALARM_LOW_TEMP  = 0x01;      ///< Temperature below low threshold
constexpr uint8_t ALARM_HIGH_TEMP = 0x02;      ///< Temperature above high threshold
/** @} */

/**
 * @class Sensor
 * @brief Abstract temperature sensor interface
 * @details Provides unified access to DS18B20 and PT1000 temperature sensors
 *          with common functionality for temperature reading, alarm management,
 *          and error handling.
 */
class Sensor {
public:
    /**
     * @brief Construct a new Sensor object
     * @param[in] type Sensor type (DS18B20 or PT1000)
     * @param[in] address Logical address for the sensor
     * @param[in] name Human-readable name for the sensor
     */
    Sensor(SensorType type, uint8_t address, const String& name);
    
    /**
     * @brief Destroy the Sensor object
     * @details Cleans up hardware interfaces and dynamic memory
     */
    ~Sensor();

    // Setup methods for each sensor type
    /**
     * @brief Configure sensor as DS18B20
     * @param[in] pin OneWire bus pin number
     * @param[in] deviceAddress 8-byte ROM address of DS18B20
     */
    void setupDS18B20(uint8_t pin, const uint8_t* deviceAddress);
    
    /**
     * @brief Configure sensor as PT1000
     * @param[in] csPin SPI chip select pin
     * @param[in] maxAddress MAX31865 I2C address
     */
    void setupPT1000(uint8_t csPin, uint8_t maxAddress);

    /**
     * @brief Initialize sensor hardware
     * @return true if initialization successful
     * @return false if hardware initialization failed
     */
    bool initialize();

    /**
     * @brief Read temperature from sensor
     * @return true if reading successful
     * @return false if reading failed
     * @details Updates internal temperature values and error status
     */
    bool readTemperature();

    // Accessors
    /**
     * @brief Get sensor type
     * @return SensorType DS18B20 or PT1000
     */
    SensorType getType() const;
    
    /**
     * @brief Get sensor logical address
     * @return uint8_t Sensor address
     */
    uint8_t getAddress() const;
    
    /**
     * @brief Get sensor name
     * @return String Sensor name
     */
    String getName() const;
    
    /**
     * @brief Set sensor name
     * @param[in] newName New name for sensor
     */
    void setName(const String& newName);

    /**
     * @brief Get current temperature
     * @return int16_t Temperature in 0.1°C units
     */
    int16_t getCurrentTemp() const;
    
    /**
     * @brief Get minimum recorded temperature
     * @return int16_t Minimum temperature in 0.1°C units
     */
    int16_t getMinTemp() const;
    
    /**
     * @brief Get maximum recorded temperature
     * @return int16_t Maximum temperature in 0.1°C units
     */
    int16_t getMaxTemp() const;
    
    /**
     * @brief Get low temperature alarm threshold
     * @return int16_t Low threshold in 0.1°C units
     */
    int16_t getLowAlarmThreshold() const;
    
    /**
     * @brief Get high temperature alarm threshold
     * @return int16_t High threshold in 0.1°C units
     */
    int16_t getHighAlarmThreshold() const;
    
    /**
     * @brief Get alarm status flags
     * @return uint8_t Bitmask of active alarms
     * @see ALARM_LOW_TEMP, ALARM_HIGH_TEMP
     */
    uint8_t getAlarmStatus() const;
    
    /**
     * @brief Get error status flags
     * @return uint8_t Bitmask of active errors
     * @see ERROR_COMMUNICATION, ERROR_OUT_OF_RANGE, ERROR_DISCONNECTED
     */
    uint8_t getErrorStatus() const;
    
    /**
     * @brief Get PT1000 chip select pin
     * @return uint8_t Chip select pin number
     */
    uint8_t getPT1000ChipSelectPin() const;
    
    /**
     * @brief Set sensor logical address
     * @param[in] newAddress New address value
     */
    void setAddress(uint8_t newAddress);
    
    /**
     * @brief Set low temperature alarm threshold
     * @param[in] threshold Threshold in 0.1°C units
     */
    void setLowAlarmThreshold(int16_t threshold);
    
    /**
     * @brief Set high temperature alarm threshold
     * @param[in] threshold Threshold in 0.1°C units
     */
    void setHighAlarmThreshold(int16_t threshold);

    /**
     * @brief Get DS18B20 ROM address
     * @return const uint8_t* Pointer to 8-byte ROM address
     * @note Only valid for DS18B20 sensors
     */
    const uint8_t* getDS18B20Address() const;

    /**
     * @brief Reset min/max temperature records
     */
    void resetMinMaxTemp();

    /**
     * @brief Update alarm status based on current temperature
     * @details Call after reading temperature or changing thresholds
     */
    void updateAlarmStatus();
    
    /**
     * @brief Get DS18B20 ROM address as hex string
     * @return String ROM address in hexadecimal format
     */
    String getDS18B20RomString() const;
    
    /**
     * @brief Get DS18B20 ROM address as array
     * @param[out] out 8-byte array to receive ROM address
     */
    void getDS18B20RomArray(uint8_t out[8]) const;
    
    /**
     * @brief Get OneWire bus pin
     * @return uint8_t OneWire pin number
     */
    uint8_t getOneWirePin() {return connection.ds18b20.oneWirePin;}

private:
    uint8_t address;                    ///< Logical sensor address
    String name;                        ///< Human-readable sensor name
    SensorType type;                    ///< Type of sensor (DS18B20 or PT1000)

    int16_t currentTemp;                ///< Current temperature in 0.1°C units
    int16_t minTemp;                    ///< Minimum recorded temperature
    int16_t maxTemp;                    ///< Maximum recorded temperature
    int16_t lowAlarmThreshold;          ///< Low temperature alarm threshold
    int16_t highAlarmThreshold;         ///< High temperature alarm threshold
    uint8_t alarmStatus;                ///< Current alarm status flags
    uint8_t errorStatus;                ///< Current error status flags

    // Hardware-specific members
    OneWire* oneWire;                   ///< OneWire interface for DS18B20
    DallasTemperature* dallasTemperature; ///< Dallas temperature library instance
    Adafruit_MAX31865* max31865;        ///< MAX31865 interface for PT1000

    /**
     * @brief Union for sensor-specific connection details
     */
    union {
        /**
         * @brief DS18B20 connection parameters
         */
        struct {
            uint8_t oneWirePin;         ///< OneWire bus GPIO pin
            uint8_t oneWireAddress[8];  ///< 8-byte ROM address
        } ds18b20;
        
        /**
         * @brief PT1000 connection parameters
         */
        struct {
            uint8_t csPin;              ///< SPI chip select pin
            uint8_t maxAddress;         ///< MAX31865 I2C address
        } pt1000;
    } connection;                       ///< Sensor connection details
};

#endif // SENSOR_H
