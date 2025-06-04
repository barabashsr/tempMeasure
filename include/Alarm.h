#pragma once

#include <Arduino.h>
#include <vector>
#include "MeasurementPoint.h"

// Forward declaration
class MeasurementPoint;

enum class AlarmType {
    HIGH_TEMPERATURE,    // This should be OK since it's not just "HIGH"
    LOW_TEMPERATURE,     // This should be OK since it's not just "LOW"
    SENSOR_ERROR,
    SENSOR_DISCONNECTED
};


enum class AlarmStage {
    NEW,                    // Just triggered
    ACTIVE,                 // Confirmed and active
    ACKNOWLEDGED,           // Operator acknowledged
    CLEARED,                // Condition cleared but still in delay
    RESOLVED                // Fully resolved
};

enum class AlarmPriority {
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
};


class Alarm {
public:
    // Constructor
    Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority = AlarmPriority::PRIORITY_MEDIUM);
    
    // Destructor
    ~Alarm();
    
    // Getters
    AlarmType getType() const { return _type; }
    AlarmStage getStage() const { return _stage; }
    AlarmPriority getPriority() const { return _priority; }
    MeasurementPoint* getSource() const { return _source; }
    unsigned long getTimestamp() const { return _timestamp; }
    unsigned long getAcknowledgedTime() const { return _acknowledgedTime; }
    unsigned long getClearedTime() const { return _clearedTime; }
    String getMessage() const { return _message; }
    bool isActive() const { return _stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED; }
    bool isAcknowledged() const { return _stage == AlarmStage::ACKNOWLEDGED || _stage == AlarmStage::CLEARED; }
    bool isResolved() const { return _stage == AlarmStage::RESOLVED; }
    
    // State management
    void acknowledge();
    void clear();
    void resolve();
    void reactivate();
    
    // Update alarm condition
    bool updateCondition();
    
    // Display methods
    String getDisplayText() const;
    String getStatusText() const;
    
    // Alarm behavior configuration
    void setDelayTime(unsigned long delayMs) { _delayTime = delayMs; }
    unsigned long getDelayTime() const { return _delayTime; }
    
    // Check if delay has elapsed
    bool isDelayElapsed() const;
    
    // Comparison operators for sorting
    bool operator<(const Alarm& other) const;
    bool operator==(const Alarm& other) const;

    String getTypeString() const;
    String getStageString() const;

    // Configuration support
    String getConfigKey() const;
    void setConfigKey(const String& key);
    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }
    uint8_t getPointAddress() const { return _source ? _source->getAddress() : 255; }

    void setPriority(AlarmPriority priority);
    void setStage(AlarmStage stage);
    
    void setHysteresis(int16_t hysteresis) { _hysteresis = hysteresis; }
    int16_t getHysteresis() const { return _hysteresis; }


    

private:
    AlarmType _type;
    AlarmStage _stage;
    AlarmPriority _priority;
    MeasurementPoint* _source;
    
    // Timestamps
    unsigned long _timestamp;        // When alarm was created
    unsigned long _acknowledgedTime; // When alarm was acknowledged
    unsigned long _clearedTime;      // When condition cleared

    int16_t _hysteresis;  // Configurable hysteresis value
    
    // Configuration
    unsigned long _delayTime;        // Delay before auto-resolve
    
    // Display message
    String _message;
    
    // Internal methods
    void _updateMessage();
    bool _checkCondition();

    String _getPriorityString() const;

    String _configKey;  // Format: "alarm_<point>_<type>"
    bool _enabled;      // Whether this alarm is active in configuration
};

// Alarm comparison function for sorting by priority and timestamp
struct AlarmComparator {
    bool operator()(const Alarm* a, const Alarm* b) const {
        if (a->getPriority() != b->getPriority()) {
            return static_cast<int>(a->getPriority()) > static_cast<int>(b->getPriority());
        }
        return a->getTimestamp() < b->getTimestamp();
    }
};
