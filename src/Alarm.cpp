#include "Alarm.h"
#include "LoggerManager.h" 

// Alarm::Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority)
//     : _type(type), _stage(AlarmStage::NEW), _priority(priority), _source(source),
//       _timestamp(millis()), _acknowledgedTime(0), _clearedTime(0),
//       _acknowledgedDelay(10 * 60 * 1000), // Default 10 minutes 
//       _delayTime(5 * 60 * 1000), _enabled(true), _hysteresis(1) // Add _hysteresis(1) - 1 degree default
// {
//     if (_source) {
//         _configKey = "alarm_" + String(_source->getAddress()) + "_" + String(static_cast<int>(_type));
//     }

//     _updateMessage();
    
//     Serial.printf("New alarm created: %s for point %d (%s)\n", 
//                   getTypeString().c_str(), 
//                   _source ? _source->getAddress() : -1,
//                   _source ? _source->getName().c_str() : "Unknown");
// }

Alarm::Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority)
    : _type(type), _stage(AlarmStage::NEW), _priority(priority), _source(source),
      _timestamp(millis()), _acknowledgedTime(0), _clearedTime(0),
      _acknowledgedDelay(10 * 60 * 1000), _delayTime(5 * 60 * 1000), 
      _enabled(true), _hysteresis(1)
{
    if (_source) {
        _configKey = "alarm_" + String(_source->getAddress()) + "_" + String(static_cast<int>(_type));
    }

    _updateMessage();
    
    // LOG: Alarm creation
    String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
    String description = "New alarm created: " + getTypeString() + 
                        " for point " + String(_source ? _source->getAddress() : -1) + 
                        " (" + (_source ? _source->getName() : "Unknown") + ")";
    LoggerManager::info(source_, description);
    
    Serial.printf("New alarm created: %s for point %d (%s)\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1,
                  _source ? _source->getName().c_str() : "Unknown");
}




// Alarm::~Alarm() {
//     Serial.printf("Alarm destroyed: %s for point %d\n", 
//                   getTypeString().c_str(), 
//                   _source ? _source->getAddress() : -1);
// }

Alarm::~Alarm() {
    // LOG: Alarm destruction
    String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
    String description = "Alarm destroyed: " + getTypeString() + 
                        " for point " + String(_source ? _source->getAddress() : -1);
    LoggerManager::info(source_, description);
    
    Serial.printf("Alarm destroyed: %s for point %d\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}


void Alarm::acknowledge() {
    if (_stage == AlarmStage::NEW || _stage == AlarmStage::ACTIVE) {
        String oldStage = getStageString();
        _stage = AlarmStage::ACKNOWLEDGED;
        _acknowledgedTime = millis();
        _updateMessage();
        
        // LOG: Manual acknowledgment
        String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
        String description = "Alarm acknowledged: " + getTypeString() + 
                            " for point " + String(_source ? _source->getAddress() : -1) + 
                            " (" + (_source ? _source->getName() : "Unknown") + ")";
        LoggerManager::info(source_, description);
        
        // LOG: Alarm state change
        if (_source) {
            int16_t currentTemp = _source->getCurrentTemp();
            int16_t threshold = (_type == AlarmType::HIGH_TEMPERATURE) ? 
                               _source->getHighAlarmThreshold() : _source->getLowAlarmThreshold();
            
            LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                             getTypeString(), _getPriorityString(),
                                             oldStage, "ACKNOWLEDGED", currentTemp, threshold);
        }
        
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
        
        // LOG: Alarm cleared
        String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
        String description = "Alarm cleared: " + getTypeString() + 
                            " for point " + String(_source ? _source->getAddress() : -1) + 
                            " (" + (_source ? _source->getName() : "Unknown") + ")";
        LoggerManager::info(source_, description);
        
        Serial.printf("Alarm cleared: %s for point %d\n", 
                      getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

void Alarm::resolve() {
    _stage = AlarmStage::RESOLVED;
    _updateMessage();
    
    // LOG: Alarm resolved
    String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
    String description = "Alarm resolved: " + getTypeString() + 
                        " for point " + String(_source ? _source->getAddress() : -1) + 
                        " (" + (_source ? _source->getName() : "Unknown") + ")";
    LoggerManager::info(source_, description);
    
    Serial.printf("Alarm resolved: %s for point %d\n", 
                  getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}


void Alarm::reactivate() {
    if (_stage == AlarmStage::CLEARED) {
        _stage = _acknowledgedTime > 0 ? AlarmStage::ACKNOWLEDGED : AlarmStage::ACTIVE;
        _clearedTime = 0;
        _updateMessage();
        
        // LOG: Alarm reactivated
        String source_ = "ALARM_" + String(_source ? _source->getAddress() : -1);
        String description = "Alarm reactivated: " + getTypeString() + 
                            " for point " + String(_source ? _source->getAddress() : -1) + 
                            " (" + (_source ? _source->getName() : "Unknown") + ")";
        LoggerManager::warning(source_, description);
        
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

void Alarm::setPriority(AlarmPriority priority) {
    if (_priority != priority) {
        AlarmPriority oldPriority = _priority;
        _priority = priority;
        
        // LOG: Priority change
        String source_ = "CONFIG_" + String(_source ? _source->getAddress() : -1);
        String description = "Alarm priority changed from " + _getPriorityString(oldPriority) + 
                            " to " + _getPriorityString(priority) + 
                            " for " + getTypeString() + " alarm";
        LoggerManager::info(source_, description);
    }
}

void Alarm::setHysteresis(int16_t hysteresis) {
    if (_hysteresis != hysteresis) {
        int16_t oldHysteresis = _hysteresis;
        _hysteresis = hysteresis;
        
        // LOG: Hysteresis change
        String source_ = "CONFIG_" + String(_source ? _source->getAddress() : -1);
        String description = "Alarm hysteresis changed from " + String(oldHysteresis) + 
                            " to " + String(hysteresis) + 
                            " for " + getTypeString() + " alarm";
        LoggerManager::info(source_, description);
    }
}

void Alarm::setEnabled(bool enabled) {
    if (_enabled != enabled) {
        _enabled = enabled;
        
        // LOG: Enable/disable change
        String source_ = "CONFIG_" + String(_source ? _source->getAddress() : -1);
        String description = getTypeString() + " alarm " + (enabled ? "enabled" : "disabled");
        LoggerManager::info(source_, description);
    }
}



void Alarm::setStage(AlarmStage stage){
    _stage = stage;
};

// Modify the updateCondition method to handle acknowledged timeout
bool Alarm::updateCondition() {
    if (!_source) {
        Serial.println("Alarm updateCondition: No source");
        return true;
    }
    
    bool conditionExists = _checkCondition();
    AlarmStage oldStage = _stage;
    
    Serial.printf("Alarm update: Point %d, Type=%s, Stage=%s, Condition=%s\n",
                  _source->getAddress(), getTypeString().c_str(), 
                  getStageString().c_str(), conditionExists ? "EXISTS" : "CLEARED");
    
    String source_ = "ALARM_" + String(_source->getAddress());
    String baseDescription = getTypeString() + " alarm for point " + 
                            String(_source->getAddress()) + " (" + _source->getName() + ")";
    
    // Get current temperature and threshold for logging
    int16_t currentTemp = _source->getCurrentTemp();
    int16_t threshold = 0;
    
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE:
            threshold = _source->getHighAlarmThreshold();
            break;
        case AlarmType::LOW_TEMPERATURE:
            threshold = _source->getLowAlarmThreshold();
            break;
        case AlarmType::SENSOR_ERROR:
        case AlarmType::SENSOR_DISCONNECTED:
            threshold = 0; // Not applicable for these alarm types
            break;
    }
    
    switch (_stage) {
        case AlarmStage::NEW:
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                
                // LOG: NEW -> ACTIVE
                LoggerManager::error(source_, baseDescription + " activated");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "NEW", "ACTIVE", currentTemp, threshold);
                
                Serial.printf("Alarm %s: NEW -> ACTIVE\n", getTypeString().c_str());
            } else {
                resolve();
                
                // LOG: NEW -> RESOLVED
                LoggerManager::info(source_, baseDescription + " resolved before activation");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "NEW", "RESOLVED", currentTemp, threshold);
                
                Serial.printf("Alarm %s: NEW -> RESOLVED (condition cleared)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::ACTIVE:
            if (!conditionExists) {
                clear();
                
                // LOG: ACTIVE -> CLEARED
                LoggerManager::info(source_, baseDescription + " condition cleared");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "ACTIVE", "CLEARED", currentTemp, threshold);
                
                Serial.printf("Alarm %s: ACTIVE -> CLEARED (condition no longer exists)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::ACKNOWLEDGED:
            if (!conditionExists) {
                clear();
                
                // LOG: ACKNOWLEDGED -> CLEARED
                LoggerManager::info(source_, baseDescription + " condition cleared while acknowledged");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "ACKNOWLEDGED", "CLEARED", currentTemp, threshold);
                
                Serial.printf("Alarm %s: ACKNOWLEDGED -> CLEARED (condition no longer exists)\n", getTypeString().c_str());
            } else if (isAcknowledgedDelayElapsed()) {
                _stage = AlarmStage::ACTIVE;
                
                // LOG: ACKNOWLEDGED -> ACTIVE (timeout)
                LoggerManager::warning(source_, baseDescription + " acknowledgment timeout - returned to active");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "ACKNOWLEDGED", "ACTIVE", currentTemp, threshold);
                
                Serial.printf("Alarm %s: ACKNOWLEDGED -> ACTIVE (acknowledged delay elapsed)\n", getTypeString().c_str());
            }
            break;
            
        case AlarmStage::CLEARED:
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                _clearedTime = 0;
                
                // LOG: CLEARED -> ACTIVE (condition returned)
                LoggerManager::warning(source_, baseDescription + " condition returned");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "CLEARED", "ACTIVE", currentTemp, threshold);
                
                Serial.printf("Alarm %s: CLEARED -> ACTIVE (condition returned)\n", getTypeString().c_str());
            } else if (isDelayElapsed()) {
                resolve();
                
                // LOG: CLEARED -> RESOLVED (delay elapsed)
                LoggerManager::info(source_, baseDescription + " auto-resolved after delay");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "CLEARED", "RESOLVED", currentTemp, threshold);
                
                Serial.printf("Alarm %s: CLEARED -> RESOLVED (delay elapsed)\n", getTypeString().c_str());
            }
            break;
        
        case AlarmStage::RESOLVED:
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                _timestamp = millis();
                _acknowledgedTime = 0;
                _clearedTime = 0;
                
                // LOG: RESOLVED -> ACTIVE (condition returned)
                LoggerManager::error(source_, baseDescription + " reoccurred after resolution");
                LoggerManager::logAlarmStateChange(_source->getAddress(), _source->getName(),
                                                 getTypeString(), _getPriorityString(),
                                                 "RESOLVED", "ACTIVE", currentTemp, threshold);
                
                Serial.printf("Alarm %s: RESOLVED -> ACTIVE (condition returned)\n", getTypeString().c_str());
            }
            break;
    }
    
    if (oldStage != _stage) {
        _updateMessage();
    }
    
    return true;
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


String Alarm::_getPriorityString(AlarmPriority priority) const {
    switch (priority) {
        case AlarmPriority::PRIORITY_LOW: return "LOW";
        case AlarmPriority::PRIORITY_MEDIUM: return "MEDIUM";
        case AlarmPriority::PRIORITY_HIGH: return "HIGH";
        case AlarmPriority::PRIORITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}
