#include "Alarm.h"

Alarm::Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority)
    : _type(type), _stage(AlarmStage::NEW), _priority(priority), _source(source),
      _timestamp(millis()), _acknowledgedTime(0), _clearedTime(0),
      _acknowledgedDelay(10 * 60 * 1000), // Default 10 minutes 
      _delayTime(5 * 60 * 1000), _enabled(true), _hysteresis(1) // Add _hysteresis(1) - 1 degree default
{
    if (_source) {
        _configKey = "alarm_" + String(_source->getAddress()) + "_" + String(static_cast<int>(_type));
    }

    _updateMessage();
    
    Serial.printf("New alarm created: %s for point %d (%s)\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1,
                  _source ? _source->getName().c_str() : "Unknown");
}


Alarm::~Alarm() {
    Serial.printf("Alarm destroyed: %s for point %d\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}

void Alarm::acknowledge() {
    if (_stage == AlarmStage::NEW || _stage == AlarmStage::ACTIVE) {
        _stage = AlarmStage::ACKNOWLEDGED;
        _acknowledgedTime = millis();
        _updateMessage();
        
        Serial.printf("Alarm acknowledged: %s for point %d\n", 
                      getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

void Alarm::clear() {
    if (_stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED) {
        _stage = AlarmStage::CLEARED;
        _clearedTime = millis();
        _updateMessage();
        
        Serial.printf("Alarm cleared: %s for point %d\n", 
                      getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

void Alarm::resolve() {
    _stage = AlarmStage::RESOLVED;
    _updateMessage();
    
    Serial.printf("Alarm resolved: %s for point %d\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}

void Alarm::reactivate() {
    if (_stage == AlarmStage::CLEARED) {
        _stage = _acknowledgedTime > 0 ? AlarmStage::ACKNOWLEDGED : AlarmStage::ACTIVE;
        _clearedTime = 0;
        _updateMessage();
        
        Serial.printf("Alarm reactivated: %s for point %d\n", 
                      getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

bool Alarm::_checkCondition() {
    if (!_source) {
        Serial.println("Alarm: No source point");
        return false;
    }
    
    int16_t currentTemp = _source->getCurrentTemp();
    bool condition = false;
    
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE:
            {
                int16_t threshold = _source->getHighAlarmThreshold();
                if (_stage == AlarmStage::CLEARED || _stage == AlarmStage::RESOLVED) {
                    // When cleared/resolved, need temperature to drop below (threshold - hysteresis) to stay cleared
                    // Return true if still above (threshold - hysteresis) = condition still exists
                    condition = currentTemp > (threshold - _hysteresis);
                } else {
                    // Normal activation check: activate when >= threshold
                    condition = currentTemp >= threshold;
                }
                Serial.printf("HIGH_TEMP check: Point %d, Temp=%d, Threshold=%d, Hysteresis=%d, Stage=%s, Condition=%s\n",
                             _source->getAddress(), currentTemp, threshold, _hysteresis,
                             getStageString().c_str(), condition ? "TRUE" : "FALSE");
            }
            break;
            
        case AlarmType::LOW_TEMPERATURE:
            {
                int16_t threshold = _source->getLowAlarmThreshold();
                if (_stage == AlarmStage::CLEARED || _stage == AlarmStage::RESOLVED) {
                    // When cleared/resolved, need temperature to rise above (threshold + hysteresis) to stay cleared
                    // Return true if still below (threshold + hysteresis) = condition still exists
                    condition = currentTemp < (threshold + _hysteresis);
                } else {
                    // Normal activation check: activate when <= threshold
                    condition = currentTemp <= threshold;
                }
                Serial.printf("LOW_TEMP check: Point %d, Temp=%d, Threshold=%d, Hysteresis=%d, Stage=%s, Condition=%s\n",
                             _source->getAddress(), currentTemp, threshold, _hysteresis,
                             getStageString().c_str(), condition ? "TRUE" : "FALSE");
            }
            break;
            
        case AlarmType::SENSOR_ERROR:
            condition = _source->getErrorStatus() != 0;
            Serial.printf("SENSOR_ERROR check: Point %d, Error=%d, Condition=%s\n",
                         _source->getAddress(), _source->getErrorStatus(), 
                         condition ? "TRUE" : "FALSE");
            break;
            
        case AlarmType::SENSOR_DISCONNECTED:
            condition = _source->getBoundSensor() == nullptr;
            Serial.printf("DISCONNECTED check: Point %d, Sensor=%p, Condition=%s\n",
                         _source->getAddress(), _source->getBoundSensor(), 
                         condition ? "TRUE" : "FALSE");
            break;
            
        default:
            Serial.println("Unknown alarm type");
            return false;
    }
    
    return condition;
}




// bool Alarm::_checkCondition() {
//     if (!_source) {
//         Serial.println("Alarm: No source point");
//         return false;
//     }
//     const int HYSTERESIS = 2; // 1 degree hysteresis
    
//     bool condition = false;
    
//     switch (_type) {
//         // case AlarmType::HIGH_TEMPERATURE:
//         //     condition = _source->getCurrentTemp() >= _source->getHighAlarmThreshold();
//         //     Serial.printf("HIGH_TEMP check: Point %d, Temp=%d, Threshold=%d, Condition=%s\n",
//         //                  _source->getAddress(), _source->getCurrentTemp(), 
//         //                  _source->getHighAlarmThreshold(), condition ? "TRUE" : "FALSE");
//         //     break;
            
//         // case AlarmType::LOW_TEMPERATURE:
//         //     condition = _source->getCurrentTemp() <= _source->getLowAlarmThreshold();
//         //     Serial.printf("LOW_TEMP check: Point %d, Temp=%d, Threshold=%d, Condition=%s\n",
//         //                  _source->getAddress(), _source->getCurrentTemp(), 
//         //                  _source->getLowAlarmThreshold(), condition ? "TRUE" : "FALSE");
//         //     break;
        

//         case AlarmType::HIGH_TEMPERATURE:
//             if (_stage == AlarmStage::CLEARED) {
//                 // When cleared, need temperature to go below threshold - hysteresis
//                 return _source->getCurrentTemp() >= (_source->getHighAlarmThreshold() - HYSTERESIS);
//             } else {
//                 // Normal check
//                 return _source->getCurrentTemp() >= _source->getHighAlarmThreshold();
//             }
            
//         case AlarmType::LOW_TEMPERATURE:
//             if (_stage == AlarmStage::CLEARED) {
//                 // When cleared, need temperature to go above threshold + hysteresis
//                 return _source->getCurrentTemp() <= (_source->getLowAlarmThreshold() + HYSTERESIS);
//             } else {
//                 // Normal check
//                 return _source->getCurrentTemp() <= _source->getLowAlarmThreshold();
//             }
            
//         case AlarmType::SENSOR_ERROR:
//             condition = _source->getErrorStatus() != 0;
//             Serial.printf("SENSOR_ERROR check: Point %d, Error=%d, Condition=%s\n",
//                          _source->getAddress(), _source->getErrorStatus(), 
//                          condition ? "TRUE" : "FALSE");
//             break;
            
//         case AlarmType::SENSOR_DISCONNECTED:
//             condition = _source->getBoundSensor() == nullptr;
//             Serial.printf("DISCONNECTED check: Point %d, Sensor=%p, Condition=%s\n",
//                          _source->getAddress(), _source->getBoundSensor(), 
//                          condition ? "TRUE" : "FALSE");
//             break;
            
//         default:
//             Serial.println("Unknown alarm type");
//             return false;
//     }
    
//     return condition;
// }



bool Alarm::isDelayElapsed() const {
    if (_stage != AlarmStage::CLEARED || _clearedTime == 0) {
        return false;
    }
    return (millis() - _clearedTime) >= _delayTime;
}

String Alarm::getDisplayText() const {
    if (!_source) return "Unknown Alarm";
    
    String text = String(_source->getAddress()) + "." + _source->getName();
    text += "\n";
    
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE:
            text += "H: " + String(_source->getCurrentTemp()) + "°C";
            break;
        case AlarmType::LOW_TEMPERATURE:
            text += "L: " + String(_source->getCurrentTemp()) + "°C";
            break;
        case AlarmType::SENSOR_ERROR:
            text += "E: " + String(_source->getErrorStatus());
            break;
        case AlarmType::SENSOR_DISCONNECTED:
            text += "DISCONNECTED";
            break;
    }
    
    if (_stage == AlarmStage::ACKNOWLEDGED) {
        text += " ACK";
    }
    
    return text;
}

String Alarm::getStatusText() const {
    String status = getTypeString() + " - " + getStageString();
    if (_source) {
        status += " (Point " + String(_source->getAddress()) + ")";
    }
    return status;
}

void Alarm::_updateMessage() {
    _message = getDisplayText();
}

String Alarm::getTypeString() const {
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE: return "HIGH_TEMP";
        case AlarmType::LOW_TEMPERATURE: return "LOW_TEMP";
        case AlarmType::SENSOR_ERROR: return "SENSOR_ERROR";
        case AlarmType::SENSOR_DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

String Alarm::getStageString() const {
    switch (_stage) {
        case AlarmStage::NEW: return "NEW";
        case AlarmStage::ACTIVE: return "ACTIVE";
        case AlarmStage::ACKNOWLEDGED: return "ACKNOWLEDGED";
        case AlarmStage::CLEARED: return "CLEARED";
        case AlarmStage::RESOLVED: return "RESOLVED";
        default: return "UNKNOWN";
    }
}

String Alarm::_getPriorityString() const {
    switch (_priority) {
        case AlarmPriority::PRIORITY_LOW: return "LOW";
        case AlarmPriority::PRIORITY_MEDIUM: return "MEDIUM";
        case AlarmPriority::PRIORITY_HIGH: return "HIGH";
        case AlarmPriority::PRIORITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

bool Alarm::operator<(const Alarm& other) const {
    // Sort by priority first (higher priority first), then by timestamp (older first)
    if (_priority != other._priority) {
        return static_cast<int>(_priority) > static_cast<int>(other._priority);
    }
    return _timestamp < other._timestamp;
}

bool Alarm::operator==(const Alarm& other) const {
    return _type == other._type && 
           _source == other._source && 
           _timestamp == other._timestamp;
}



String Alarm::getConfigKey() const {
    return _configKey;
}

void Alarm::setConfigKey(const String& key) {
    _configKey = key;
}


void Alarm::setPriority(AlarmPriority priority){
    _priority = priority;
    
};

void Alarm::setStage(AlarmStage stage){
    _stage = stage;
};

// Modify the updateCondition method to handle acknowledged timeout
bool Alarm::updateCondition() {
    if (!_source) {
        Serial.println("Alarm updateCondition: No source");
        return true; // Keep alarm even without source
    }
    
    bool conditionExists = _checkCondition();
    AlarmStage oldStage = _stage;
    
    Serial.printf("Alarm update: Point %d, Type=%s, Stage=%s, Condition=%s\n",
                  _source->getAddress(), getTypeString().c_str(), 
                  getStageString().c_str(), conditionExists ? "EXISTS" : "CLEARED");
    
    switch (_stage) {
        case AlarmStage::NEW:
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                Serial.printf("Alarm %s: NEW -> ACTIVE\n", getTypeString().c_str());
            } else {
                // Condition cleared before becoming active
                resolve();
                Serial.printf("Alarm %s: NEW -> RESOLVED (condition cleared)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::ACTIVE:
            if (!conditionExists) {
                clear();
                Serial.printf("Alarm %s: ACTIVE -> CLEARED (condition no longer exists)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::ACKNOWLEDGED:
            if (!conditionExists) {
                clear();
                Serial.printf("Alarm %s: ACKNOWLEDGED -> CLEARED (condition no longer exists)\n", getTypeString().c_str());
            } else if (isAcknowledgedDelayElapsed()) {
                // NEW: Return to ACTIVE if acknowledged delay has elapsed and condition still exists
                _stage = AlarmStage::ACTIVE;
                Serial.printf("Alarm %s: ACKNOWLEDGED -> ACTIVE (acknowledged delay elapsed)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::CLEARED:
            if (conditionExists) {
                // Condition returned, reactivate
                _stage = _acknowledgedTime > 0 ? AlarmStage::ACKNOWLEDGED : AlarmStage::ACTIVE;
                _clearedTime = 0;
                Serial.printf("Alarm %s: CLEARED -> %s (condition returned)\n", 
                             getTypeString().c_str(), getStageString().c_str());
            } else if (isDelayElapsed()) {
                resolve();
                Serial.printf("Alarm %s: CLEARED -> RESOLVED (delay elapsed)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::RESOLVED:
            // Reactivate resolved alarms when condition returns
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                _timestamp = millis(); // Reset timestamp
                _acknowledgedTime = 0; // Reset acknowledged time
                _clearedTime = 0;      // Reset cleared time
                Serial.printf("Alarm %s: RESOLVED -> ACTIVE (condition returned)\n", getTypeString().c_str());
            }
            break;
    }
    
    if (oldStage != _stage) {
        _updateMessage();
    }
    
    return true; // Always keep alarm
}


// Add these new methods to Alarm.cpp
void Alarm::setAcknowledgedDelay(unsigned long delay) {
    _acknowledgedDelay = delay;
}

unsigned long Alarm::getAcknowledgedDelay() const {
    return _acknowledgedDelay;
}

bool Alarm::isAcknowledgedDelayElapsed() const {
    if (_stage != AlarmStage::ACKNOWLEDGED || _acknowledgedTime == 0) {
        return false;
    }
    return (millis() - _acknowledgedTime) >= _acknowledgedDelay;
}

unsigned long Alarm::getAcknowledgedTimeLeft() const {
    if (_stage != AlarmStage::ACKNOWLEDGED || _acknowledgedTime == 0) {
        return 0; // No time left if not acknowledged
    }
    
    unsigned long elapsed = millis() - _acknowledgedTime;
    if (elapsed >= _acknowledgedDelay) {
        return 0; // Time already elapsed
    }
    
    return _acknowledgedDelay - elapsed; // Time remaining in milliseconds
}
