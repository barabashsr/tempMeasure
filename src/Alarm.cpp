#include "Alarm.h"

Alarm::Alarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority)
    : _type(type), _stage(AlarmStage::NEW), _priority(priority), _source(source),
      _timestamp(millis()), _acknowledgedTime(0), _clearedTime(0), _delayTime(5 * 60 * 1000) // 5 minutes default
{
    _updateMessage();
    
    Serial.printf("New alarm created: %s for point %d (%s)\n", 
                  _getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1,
                  _source ? _source->getName().c_str() : "Unknown");
}

Alarm::~Alarm() {
    Serial.printf("Alarm destroyed: %s for point %d\n", 
                  _getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}

void Alarm::acknowledge() {
    if (_stage == AlarmStage::NEW || _stage == AlarmStage::ACTIVE) {
        _stage = AlarmStage::ACKNOWLEDGED;
        _acknowledgedTime = millis();
        _updateMessage();
        
        Serial.printf("Alarm acknowledged: %s for point %d\n", 
                      _getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

void Alarm::clear() {
    if (_stage == AlarmStage::ACTIVE || _stage == AlarmStage::ACKNOWLEDGED) {
        _stage = AlarmStage::CLEARED;
        _clearedTime = millis();
        _updateMessage();
        
        Serial.printf("Alarm cleared: %s for point %d\n", 
                      _getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

void Alarm::resolve() {
    _stage = AlarmStage::RESOLVED;
    _updateMessage();
    
    Serial.printf("Alarm resolved: %s for point %d\n", 
                  _getTypeString().c_str(), 
                  _source ? _source->getAddress() : -1);
}

void Alarm::reactivate() {
    if (_stage == AlarmStage::CLEARED) {
        _stage = _acknowledgedTime > 0 ? AlarmStage::ACKNOWLEDGED : AlarmStage::ACTIVE;
        _clearedTime = 0;
        _updateMessage();
        
        Serial.printf("Alarm reactivated: %s for point %d\n", 
                      _getTypeString().c_str(), 
                      _source ? _source->getAddress() : -1);
    }
}

bool Alarm::updateCondition() {
    if (!_source) return false;
    
    bool conditionExists = _checkCondition();
    
    switch (_stage) {
        case AlarmStage::NEW:
            if (conditionExists) {
                _stage = AlarmStage::ACTIVE;
                _updateMessage();
                return true;
            } else {
                // Condition cleared before becoming active
                resolve();
                return false;
            }
            break;
            
        case AlarmStage::ACTIVE:
        case AlarmStage::ACKNOWLEDGED:
            if (!conditionExists) {
                clear();
                return true;
            }
            break;
            
        case AlarmStage::CLEARED:
            if (conditionExists) {
                reactivate();
                return true;
            } else if (isDelayElapsed()) {
                resolve();
                return false;
            }
            break;
            
        case AlarmStage::RESOLVED:
            // Resolved alarms don't update
            return false;
    }
    
    return conditionExists;
}

bool Alarm::_checkCondition() {
    if (!_source) return false;
    
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE:
            return _source->getCurrentTemp() >= _source->getHighAlarmThreshold();
            
        case AlarmType::LOW_TEMPERATURE:
            return _source->getCurrentTemp() <= _source->getLowAlarmThreshold();
            
        case AlarmType::SENSOR_ERROR:
            return _source->getErrorStatus() != 0;
            
        case AlarmType::SENSOR_DISCONNECTED:
            return _source->getBoundSensor() == nullptr || !_source->getBoundSensor()->initialize();
            
        default:
            return false;
    }
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
            text += "HIGH: " + String(_source->getCurrentTemp()) + "°C";
            break;
        case AlarmType::LOW_TEMPERATURE:
            text += "LOW: " + String(_source->getCurrentTemp()) + "°C";
            break;
        case AlarmType::SENSOR_ERROR:
            text += "ERROR: " + String(_source->getErrorStatus());
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
    String status = _getTypeString() + " - " + _getStageString();
    if (_source) {
        status += " (Point " + String(_source->getAddress()) + ")";
    }
    return status;
}

void Alarm::_updateMessage() {
    _message = getDisplayText();
}

String Alarm::_getTypeString() const {
    switch (_type) {
        case AlarmType::HIGH_TEMPERATURE: return "HIGH_TEMP";
        case AlarmType::LOW_TEMPERATURE: return "LOW_TEMP";
        case AlarmType::SENSOR_ERROR: return "SENSOR_ERROR";
        case AlarmType::SENSOR_DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

String Alarm::_getStageString() const {
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
