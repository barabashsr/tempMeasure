/**
 * @file MeasurementPoint.h
 * @brief Measurement point abstraction for temperature monitoring system
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file defines a measurement point class that represents a logical temperature
 *          measurement location with alarm thresholds, min/max tracking, and sensor binding.
 * 
 * @section dependencies Dependencies
 * - Sensor.h for physical sensor integration
 * - LoggerManager.h for event logging
 * 
 * @section hardware Hardware Requirements
 * - Compatible with DS18B20 and PT1000 temperature sensors
 */

#ifndef MEASUREMENT_POINT_H
#define MEASUREMENT_POINT_H

#include <Arduino.h>
#include "Sensor.h"
#include "LoggerManager.h"



/**
 * @brief Logical measurement point for temperature monitoring
 * @details Represents a temperature measurement location with configurable alarm thresholds,
 *          min/max tracking, and optional sensor binding for direct hardware access
 */
class MeasurementPoint {
public:
    /**
     * @brief Default constructor
     * @details Initializes measurement point with default values and safe ranges
     */
    MeasurementPoint() : address(0), name(""), currentTemp(0), minTemp(32767), maxTemp(-32768),
    lowAlarmThreshold(-10), highAlarmThreshold(50), alarmStatus(0), errorStatus(0), boundSensor(nullptr) {}
    
    /**
     * @brief Constructor with address and name
     * @param[in] address Unique address for this measurement point (0-255)
     * @param[in] name Human-readable name for the measurement point
     */
    MeasurementPoint(uint8_t address, const String& name);

    /**
     * @brief Destructor
     * @details Cleans up resources and unbinds any attached sensor
     */
    ~MeasurementPoint();

    // Getters
    /**
     * @brief Get measurement point address
     * @return uint8_t Unique address (0-255)
     */
    uint8_t getAddress() const;
    
    /**
     * @brief Get measurement point name
     * @return String Human-readable name
     */
    String getName() const;
    
    /**
     * @brief Get current temperature
     * @return int16_t Current temperature in °C (integer)
     */
    int16_t getCurrentTemp() const;
    
    /**
     * @brief Get minimum recorded temperature
     * @return int16_t Minimum temperature since last reset
     */
    int16_t getMinTemp() const;
    
    /**
     * @brief Get maximum recorded temperature
     * @return int16_t Maximum temperature since last reset
     */
    int16_t getMaxTemp() const;
    
    /**
     * @brief Get low alarm threshold
     * @return int16_t Low temperature alarm threshold in °C
     */
    int16_t getLowAlarmThreshold() const;
    
    /**
     * @brief Get high alarm threshold
     * @return int16_t High temperature alarm threshold in °C
     */
    int16_t getHighAlarmThreshold() const;
    
    /**
     * @brief Get alarm status bits
     * @return uint8_t Bit field representing active alarms
     */
    uint8_t getAlarmStatus() const;
    
    /**
     * @brief Get error status bits
     * @return uint8_t Bit field representing error conditions
     */
    uint8_t getErrorStatus() const;

    // Setters
    /**
     * @brief Set measurement point name
     * @param[in] newName New human-readable name
     */
    void setName(const String& newName);
    
    /**
     * @brief Set low alarm threshold
     * @param[in] threshold Low temperature alarm threshold in °C
     */
    void setLowAlarmThreshold(int16_t threshold);
    
    /**
     * @brief Set high alarm threshold
     * @param[in] threshold High temperature alarm threshold in °C
     */
    void setHighAlarmThreshold(int16_t threshold);

    // Sensor binding (optional)
    /**
     * @brief Bind a physical sensor to this measurement point
     * @param[in] sensor Pointer to sensor instance
     * @details Links measurement point to physical sensor for direct readings
     */
    void bindSensor(Sensor* sensor);
    
    /**
     * @brief Unbind the currently bound sensor
     * @details Removes association with physical sensor
     */
    void unbindSensor();
    
    /**
     * @brief Get pointer to bound sensor
     * @return Sensor* Pointer to bound sensor, nullptr if none bound
     */
    Sensor* getBoundSensor() const;

    // Operations
    /**
     * @brief Update measurement point (call periodically)
     * @details Refreshes temperature reading and updates alarm/error status
     */
    void update();
    
    /**
     * @brief Reset min/max temperature records
     * @details Sets min/max to current temperature value
     */
    void resetMinMaxTemp();
    // void setOneWireBus(uint8_t bus);
    // uint8_t getOneWireBus();

private:
    uint8_t address;             ///< Unique measurement point address
    String name;                 ///< Human-readable measurement point name

    int16_t currentTemp;         ///< Latest temperature reading (°C)
    int16_t minTemp;             ///< Minimum recorded temperature
    int16_t maxTemp;             ///< Maximum recorded temperature
    int16_t lowAlarmThreshold;   ///< Low temperature alarm threshold
    int16_t highAlarmThreshold;  ///< High temperature alarm threshold
    uint8_t alarmStatus;         ///< Bit field for alarm conditions
    uint8_t errorStatus;         ///< Bit field for error conditions

    Sensor* boundSensor;         ///< Pointer to bound physical sensor

    /**
     * @brief Update alarm status based on current temperature
     * @details Internal method to evaluate alarm conditions and update status bits
     */
    void updateAlarmStatus();
};


#endif // TEMPERATURE_CONTROLLER_H
