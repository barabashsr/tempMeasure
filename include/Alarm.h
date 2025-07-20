/**
 * @file Alarm.h
 * @brief Alarm management system for temperature monitoring
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file contains the alarm system implementation for temperature monitoring.
 *          It provides comprehensive alarm handling with different types, priorities, and stages.
 * 
 * @section dependencies Dependencies
 * - Arduino.h for core functionality
 * - MeasurementPoint.h for measurement point association
 * - LoggerManager.h for alarm logging
 * 
 * @section hardware Hardware Requirements
 * - ESP32 microcontroller
 * - Temperature sensors (DS18B20, PT1000)
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "MeasurementPoint.h"
#include "LoggerManager.h"

// Forward declaration
class MeasurementPoint;

/**
 * @brief Enumeration defining different types of alarms
 * @details Specifies the various alarm conditions that can occur in the temperature monitoring system
 */
enum class AlarmType {
    HIGH_TEMPERATURE,    ///< Temperature exceeds high threshold
    LOW_TEMPERATURE,     ///< Temperature falls below low threshold  
    SENSOR_ERROR,        ///< Sensor communication or reading error
    SENSOR_DISCONNECTED  ///< Sensor physically disconnected
};


/**
 * @brief Enumeration defining alarm lifecycle stages
 * @details Tracks the progression of an alarm from initial trigger to resolution
 */
enum class AlarmStage {
    NEW,                    ///< Alarm just triggered, awaiting confirmation
    CLEARED,                ///< Condition cleared but still in acknowledgment delay
    RESOLVED,               ///< Alarm fully resolved and inactive
    ACKNOWLEDGED,           ///< Operator has acknowledged the alarm
    ACTIVE                  ///< Alarm confirmed and actively requiring attention
};

/**
 * @brief Enumeration defining alarm priority levels
 * @details Determines the urgency and handling of different alarms
 */
enum class AlarmPriority {
    PRIORITY_LOW,        ///< Low priority - informational alerts
    PRIORITY_MEDIUM,     ///< Medium priority - standard operational alerts
    PRIORITY_HIGH,       ///< High priority - important system alerts
    PRIORITY_CRITICAL    ///< Critical priority - immediate attention required
};


/**
 * @brief Main alarm class for temperature monitoring system
 * @details Manages individual alarm instances with full lifecycle tracking,
 *          priority handling, and integration with measurement points
 */
class Alarm {
public:
    /**
     * @brief Constructor for creating a new alarm
     * @param[in] type The type of alarm being created
     * @param[in] source Pointer to the measurement point that triggered this alarm
     * @param[in] priority Priority level for this alarm (default: PRIORITY_MEDIUM)
     * @note The alarm is created in NEW stage and timestamp is set to current time
     */
    Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority = AlarmPriority::PRIORITY_MEDIUM);
    
    /**
     * @brief Destructor for alarm cleanup
     * @details Ensures proper cleanup of alarm resources
     */
    ~Alarm();
    
    // Getters
    /**
     * @brief Get the alarm type
     * @return AlarmType The type of this alarm
     */
    AlarmType getType() const { return _type; }
    
    /**
     * @brief Get the current alarm stage
     * @return AlarmStage The current lifecycle stage
     */
    AlarmStage getStage() const { return _stage; }
    
    /**
     * @brief Get the alarm priority
     * @return AlarmPriority The priority level of this alarm
     */
    AlarmPriority getPriority() const { return _priority; }
    
    /**
     * @brief Get the source measurement point
     * @return MeasurementPoint* Pointer to the measurement point that triggered this alarm
     */
    MeasurementPoint* getSource() const { return _source; }
    
    /**
     * @brief Get the alarm creation timestamp
     * @return unsigned long Timestamp when alarm was created (millis)
     */
    unsigned long getTimestamp() const { return _timestamp; }
    
    /**
     * @brief Get the acknowledgment timestamp
     * @return unsigned long Timestamp when alarm was acknowledged (millis)
     */
    unsigned long getAcknowledgedTime() const { return _acknowledgedTime; }
    
    /**
     * @brief Get the cleared timestamp
     * @return unsigned long Timestamp when alarm condition was cleared (millis)
     */
    unsigned long getClearedTime() const { return _clearedTime; }
    
    /**
     * @brief Get the alarm message
     * @return String Human-readable alarm description
     */
    String getMessage() const { return _message; }
    
    /**
     * @brief Check if alarm is currently active
     * @return bool True if alarm is in ACTIVE or ACKNOWLEDGED stage
     */
    bool isActive() const { return _stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED; }
    
    /**
     * @brief Check if alarm has been acknowledged
     * @return bool True if alarm is in ACKNOWLEDGED or CLEARED stage
     */
    bool isAcknowledged() const { return _stage == AlarmStage::ACKNOWLEDGED || _stage == AlarmStage::CLEARED; }
    
    /**
     * @brief Check if alarm is resolved
     * @return bool True if alarm is in RESOLVED stage
     */
    bool isResolved() const { return _stage == AlarmStage::RESOLVED; }
    
    // State management
    /**
     * @brief Acknowledge the alarm
     * @details Transitions alarm to ACKNOWLEDGED stage, indicating operator awareness
     */
    void acknowledge();
    
    /**
     * @brief Clear the alarm condition
     * @details Marks that the triggering condition has been resolved
     */
    void clear();
    
    /**
     * @brief Resolve the alarm completely
     * @details Transitions alarm to RESOLVED stage, indicating full resolution
     */
    void resolve();
    
    /**
     * @brief Reactivate a previously cleared alarm
     * @details Used when alarm condition returns before full resolution
     */
    void reactivate();
    
    /**
     * @brief Update alarm condition based on current measurement
     * @return bool True if alarm condition has changed
     * @details Checks current measurement point values against alarm thresholds
     */
    bool updateCondition();
    
    // Display methods
    /**
     * @brief Get formatted display text for UI
     * @return String Human-readable text for display interfaces
     */
    String getDisplayText() const;
    
    /**
     * @brief Get status text description
     * @return String Current alarm status as text
     */
    String getStatusText() const;
    
    // Alarm behavior configuration
    /**
     * @brief Set the alarm delay time
     * @param[in] delayMs Delay time in milliseconds before auto-resolution
     */
    void setDelayTime(unsigned long delayMs) { _delayTime = delayMs; }
    
    /**
     * @brief Get the alarm delay time
     * @return unsigned long Current delay time in milliseconds
     */
    unsigned long getDelayTime() const { return _delayTime; }
    
    /**
     * @brief Check if the alarm delay period has elapsed
     * @return bool True if delay time has passed since alarm creation
     */
    bool isDelayElapsed() const;
    
    // Comparison operators for sorting
    /**
     * @brief Less-than comparison operator for alarm sorting
     * @param[in] other The alarm to compare against
     * @return bool True if this alarm has lower priority/older timestamp
     */
    bool operator<(const Alarm& other) const;
    
    /**
     * @brief Equality comparison operator
     * @param[in] other The alarm to compare against
     * @return bool True if alarms are equivalent
     */
    bool operator==(const Alarm& other) const;

    /**
     * @brief Get alarm type as string
     * @return String Human-readable alarm type description
     */
    String getTypeString() const;
    
    /**
     * @brief Get alarm stage as string
     * @return String Human-readable alarm stage description
     */
    String getStageString() const;

    // Configuration support
    /**
     * @brief Get the configuration key for this alarm
     * @return String Configuration key in format "alarm_<point>_<type>"
     */
    String getConfigKey() const;
    
    /**
     * @brief Set the configuration key
     * @param[in] key Configuration key string
     */
    void setConfigKey(const String& key);
    
    /**
     * @brief Check if alarm is enabled in configuration
     * @return bool True if alarm is active in system configuration
     */
    bool isEnabled() const { return _enabled; }
    
    /**
     * @brief Enable or disable the alarm
     * @param[in] enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Get the measurement point address
     * @return uint8_t Address of associated measurement point (255 if no source)
     */
    uint8_t getPointAddress() const { return _source ? _source->getAddress() : 255; }

    /**
     * @brief Set the alarm priority
     * @param[in] priority New priority level for this alarm
     */
    void setPriority(AlarmPriority priority);
    
    /**
     * @brief Set the alarm stage
     * @param[in] stage New stage for alarm lifecycle
     */
    void setStage(AlarmStage stage);
    
    /**
     * @brief Set the hysteresis value for alarm triggering
     * @param[in] hysteresis Hysteresis value to prevent alarm flapping
     */
    void setHysteresis(int16_t hysteresis);
    
    /**
     * @brief Get the hysteresis value
     * @return int16_t Current hysteresis setting
     */
    int16_t getHysteresis() const { return _hysteresis; }

    /**
     * @brief Set the acknowledged delay period
     * @param[in] delay Time in milliseconds before auto-resolution after acknowledgment
     */
    void setAcknowledgedDelay(unsigned long delay);
    
    /**
     * @brief Get the acknowledged delay period
     * @return unsigned long Current acknowledged delay in milliseconds
     */
    unsigned long getAcknowledgedDelay() const;
    
    /**
     * @brief Check if acknowledged delay has elapsed
     * @return bool True if acknowledged delay period has passed
     */
    bool isAcknowledgedDelayElapsed() const;
    
    /**
     * @brief Get remaining acknowledged delay time
     * @return unsigned long Milliseconds remaining in acknowledged delay
     */
    unsigned long getAcknowledgedTimeLeft() const;


    

private:
    AlarmType _type;                 ///< Type of alarm (temperature, sensor error, etc.)
    AlarmStage _stage;               ///< Current lifecycle stage
    AlarmPriority _priority;         ///< Priority level for handling
    MeasurementPoint* _source;       ///< Associated measurement point
    
    // Timestamps
    unsigned long _timestamp;        ///< When alarm was created (millis)
    unsigned long _acknowledgedTime; ///< When alarm was acknowledged (millis)
    unsigned long _clearedTime;      ///< When condition was cleared (millis)

    int16_t _hysteresis;             ///< Hysteresis value to prevent flapping
    
    // Configuration
    unsigned long _delayTime;        ///< Delay before auto-resolve (millis)
    unsigned long _acknowledgedDelay; ///< Delay after acknowledgment (millis)
    String _configKey;               ///< Configuration key "alarm_<point>_<type>"
    bool _enabled;                   ///< Whether alarm is active in configuration
    
    // Display message
    String _message;                 ///< Human-readable alarm description
    
    // Internal methods
    /**
     * @brief Update the alarm display message
     * @details Generates human-readable message based on current alarm state
     */
    void _updateMessage();
    
    /**
     * @brief Check if alarm condition is currently met
     * @return bool True if triggering condition is present
     */
    bool _checkCondition();

    /**
     * @brief Get priority as string (internal version)
     * @return String Priority level as text
     */
    String _getPriorityString() const;
    
    /**
     * @brief Get priority as string with parameter
     * @param[in] priority Priority level to convert
     * @return String Priority level as text
     */
    String _getPriorityString(AlarmPriority priority) const;
};

/**
 * @brief Alarm comparison function for priority-based sorting
 * @details Provides comparison logic for sorting alarms by priority (highest first)
 *          and timestamp (oldest first) for equal priorities
 */
struct AlarmComparator {
    /**
     * @brief Comparison operator for alarm sorting
     * @param[in] a First alarm to compare
     * @param[in] b Second alarm to compare
     * @return bool True if alarm 'a' should be sorted before alarm 'b'
     * @note Higher priority alarms sort first, older alarms sort first within same priority
     */
    bool operator()(const Alarm* a, const Alarm* b) const {
        if (a->getPriority() != b->getPriority()) {
            return static_cast<int>(a->getPriority()) > static_cast<int>(b->getPriority());
        }
        return a->getTimestamp() < b->getTimestamp();
    }
};
