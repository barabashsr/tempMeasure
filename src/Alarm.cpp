#include "Alarm.h"

Alarm::Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority)
    : _type(type), _stage(AlarmStage::NEW), _priority(priority), _source(source),
      _timestamp(millis()), _acknowledgedTime(0), _clearedTime(0), _delayTime(5 * 60 * 1000), _enabled(true) // 5 minutes default
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
                return true; // Keep resolved alarm
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
                return true; // Keep resolved alarm
            }
            break;
            
        case AlarmStage::RESOLVED:
            // Resolved alarms stay resolved and are kept
            return true;
    }
    
    if (oldStage != _stage) {
        _updateMessage();
    }
    
    return true; // Always keep alarm
}



bool Alarm::_checkCondition() {
    if (!_source) return false;
    
    const int HYSTERESIS = 1; // 1 degree hysteresis
    
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE:
            if (_stage == AlarmStage::CLEARED) {
                // When cleared, need temperature to go below threshold - hysteresis
                return _source->getCurrentTemp() >= (_source->getHighAlarmThreshold() - HYSTERESIS);
            } else {
                // Normal check
                return _source->getCurrentTemp() >= _source->getHighAlarmThreshold();
            }
            
        case AlarmType::LOW_TEMPERATURE:
            if (_stage == AlarmStage::CLEARED) {
                // When cleared, need temperature to go above threshold + hysteresis
                return _source->getCurrentTemp() <= (_source->getLowAlarmThreshold() + HYSTERESIS);
            } else {
                // Normal check
                return _source->getCurrentTemp() <= _source->getLowAlarmThreshold();
            }
            
        // ... other cases remain the same
    }
    
    return false;
}


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