/**
 * @file CSVConfigManager.cpp
 * @brief Implementation of CSV configuration management for measurement points
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements CSV export/import functionality for temperature measurement
 *          points, including sensor configuration, alarm settings, and priorities.
 * 
 * @section dependencies Dependencies
 * - CSVConfigManager.h for class definition
 * - TemperatureController for measurement point access
 * - LittleFS for file operations
 * 
 * @section format CSV Format
 * - Header includes all configuration fields
 * - Sample line with point -1 shows alarm priorities
 * - Supports DS18B20 and PT1000 sensor configurations
 * - Includes alarm threshold and priority settings
 */

#include "CSVConfigManager.h"
#include <LittleFS.h>

CSVConfigManager::CSVConfigManager(TemperatureController& controller) 
    : _controller(controller), _lastError("") {
}

String CSVConfigManager::exportPointsWithAlarmsToCSV() {
    // Create header with bus number instead of chip select
    String csv = "PointAddress,PointName,PointType,CurrentTemp,MinTemp,MaxTemp,";
    csv += "LowTempThreshold,HighTempThreshold,SensorROM,SensorBusNumber,";
    csv += "HIGH_TEMPERATURE,LOW_TEMPERATURE,SENSOR_ERROR,SENSOR_DISCONNECTED\n";
    
    // Add sample line with point -1 showing all possible priorities
    csv += "-1,SAMPLE_POINT,SAMPLE,0,0,0,0,0,,";
    csv += ",CRITICAL,HIGH,MEDIUM,LOW\n";
    
    // Export DS18B20 points (0-49)
    for (int i = 0; i < 50; i++) {
        MeasurementPoint* point = _controller.getDS18B20Point(i);
        if (point) {
            _exportPointToCSV(csv, point, "DS18B20");
        }
    }
    
    // Export PT1000 points (50-59)
    for (int i = 0; i < 10; i++) {
        MeasurementPoint* point = _controller.getPT1000Point(i);
        if (point) {
            _exportPointToCSV(csv, point, "PT1000");
        }
    }
    
    return csv;
}

void CSVConfigManager::_exportPointToCSV(String& csv, MeasurementPoint* point, const String& pointType) {
    if (!point) return;
    
    // Get bound sensor info
    Sensor* sensor = point->getBoundSensor();
    String romString = "";
    String busNumber = "";
    
    if (sensor) {
        if (sensor->getType() == SensorType::DS18B20) {
            romString = sensor->getDS18B20RomString();
        } else if (sensor->getType() == SensorType::PT1000) {
            busNumber = String(_controller.getSensorBus(sensor)); // Changed from ChipSelectPin to BusNumber
        }
    }
    
    // Build the CSV row
    csv += String(point->getAddress()) + ",";
    csv += _escapeCSVField(point->getName()) + ",";
    csv += pointType + ",";
    csv += String(point->getCurrentTemp()) + ",";
    csv += String(point->getMinTemp()) + ",";
    csv += String(point->getMaxTemp()) + ",";
    csv += String(point->getLowAlarmThreshold()) + ",";
    csv += String(point->getHighAlarmThreshold()) + ",";
    csv += _escapeCSVField(romString) + ",";
    csv += busNumber + ","; // Changed variable name
    
    // Add alarm priorities for each type
    csv += _getAlarmPriorityForPoint(point->getAddress(), AlarmType::HIGH_TEMPERATURE) + ",";
    csv += _getAlarmPriorityForPoint(point->getAddress(), AlarmType::LOW_TEMPERATURE) + ",";
    csv += _getAlarmPriorityForPoint(point->getAddress(), AlarmType::SENSOR_ERROR) + ",";
    csv += _getAlarmPriorityForPoint(point->getAddress(), AlarmType::SENSOR_DISCONNECTED) + "\n";
}


bool CSVConfigManager::importPointsWithAlarmsFromCSV(const String& csvData) {
    if (!validatePointsCSV(csvData)) {
        return false;
    }
    
    int lineStart = 0;
    int lineEnd = csvData.indexOf('\n');
    
    // Skip header line
    if (lineEnd == -1) {
        _lastError = "Invalid CSV format";
        return false;
    }
    lineStart = lineEnd + 1;
    
    // Skip sample line (point -1)
    lineEnd = csvData.indexOf('\n', lineStart);
    if (lineEnd == -1) {
        _lastError = "Missing sample line";
        return false;
    }
    lineStart = lineEnd + 1;
    
    // Clear existing alarms
    _controller.clearConfiguredAlarms();
    
    // Parse each data line
    while (lineStart < csvData.length()) {
        lineEnd = csvData.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = csvData.length();
        
        String line = csvData.substring(lineStart, lineEnd);
        line.trim();
        
        if (line.length() > 0) {
            if (!_parseCSVLine(line)) {
                return false;
            }
        }
        
        lineStart = lineEnd + 1;
    }
    
    return true;
}


bool CSVConfigManager::_parseAlarmFromCSV(int pointAddress, const String& alarmType, 
                                         const String& priority, const String& enabled, 
                                         const String& hysteresis) {
    AlarmType type = _parseAlarmType(alarmType);
    AlarmPriority prio = _parsePriority(priority);
    bool isEnabled = (enabled.equalsIgnoreCase("true"));
    int16_t hyst = hysteresis.toInt();
    
    // Add alarm to controller
    if (_controller.addAlarm(type, pointAddress, prio)) {
        // Find the alarm we just added and configure it
        String configKey = "alarm_" + String(pointAddress) + "_" + String(static_cast<int>(type));
        Alarm* alarm = _controller.findAlarm(configKey);
        if (alarm) {
            alarm->setEnabled(isEnabled);
            alarm->setHysteresis(hyst);
            return true;
        }
    }
    
    return false;
}

String CSVConfigManager::_escapeCSVField(const String& field) {
    if (field.indexOf(',') >= 0 || field.indexOf('"') >= 0 || field.indexOf('\n') >= 0) {
        String escaped = "\"";
        for (int i = 0; i < field.length(); i++) {
            if (field.charAt(i) == '"') {
                escaped += "\"\"";
            } else {
                escaped += field.charAt(i);
            }
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

String CSVConfigManager::_unescapeCSVField(const String& field) {
    if (field.startsWith("\"") && field.endsWith("\"")) {
        String unescaped = field.substring(1, field.length() - 1);
        unescaped.replace("\"\"", "\"");
        return unescaped;
    }
    return field;
}

String CSVConfigManager::_getAlarmTypeString(AlarmType type) {
    switch (type) {
        case AlarmType::HIGH_TEMPERATURE: return "HIGH_TEMP";
        case AlarmType::LOW_TEMPERATURE: return "LOW_TEMP";
        case AlarmType::SENSOR_ERROR: return "SENSOR_ERROR";
        case AlarmType::SENSOR_DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

AlarmType CSVConfigManager::_parseAlarmType(const String& typeStr) {
    if (typeStr == "HIGH_TEMP") return AlarmType::HIGH_TEMPERATURE;
    if (typeStr == "LOW_TEMP") return AlarmType::LOW_TEMPERATURE;
    if (typeStr == "SENSOR_ERROR") return AlarmType::SENSOR_ERROR;
    if (typeStr == "DISCONNECTED") return AlarmType::SENSOR_DISCONNECTED;
    return AlarmType::HIGH_TEMPERATURE; // Default
}

String CSVConfigManager::_getPriorityString(AlarmPriority priority) {
    switch (priority) {
        case AlarmPriority::PRIORITY_LOW: return "LOW";
        case AlarmPriority::PRIORITY_MEDIUM: return "MEDIUM";
        case AlarmPriority::PRIORITY_HIGH: return "HIGH";
        case AlarmPriority::PRIORITY_CRITICAL: return "CRITICAL";
        default: return "MEDIUM";
    }
}

AlarmPriority CSVConfigManager::_parsePriority(const String& priorityStr) {
    if (priorityStr == "LOW") return AlarmPriority::PRIORITY_LOW;
    if (priorityStr == "MEDIUM") return AlarmPriority::PRIORITY_MEDIUM;
    if (priorityStr == "HIGH") return AlarmPriority::PRIORITY_HIGH;
    if (priorityStr == "CRITICAL") return AlarmPriority::PRIORITY_CRITICAL;
    return AlarmPriority::PRIORITY_MEDIUM; // Default
}

bool CSVConfigManager::validatePointsCSV(const String& csvData) {
    if (csvData.length() == 0) {
        _lastError = "Empty CSV data";
        return false;
    }
    
    // Check for required headers
    String requiredHeaders[] = {
        "PointAddress", "PointName", "PointType", "CurrentTemp", "MinTemp", "MaxTemp",
        "LowTempThreshold", "HighTempThreshold", "SensorROM", "SensorBusNumber", // Changed header
        "HIGH_TEMPERATURE", "LOW_TEMPERATURE", "SENSOR_ERROR", "SENSOR_DISCONNECTED"
    };
    
    int headerLine = csvData.indexOf('\n');
    if (headerLine == -1) {
        _lastError = "No header line found";
        return false;
    }
    
    String header = csvData.substring(0, headerLine);
    for (const String& reqHeader : requiredHeaders) {
        if (header.indexOf(reqHeader) == -1) {
            _lastError = "Missing required header: " + reqHeader;
            return false;
        }
    }
    
    return true;
}



bool CSVConfigManager::saveCSVToFile(const String& filename, const String& csvData) {
    if (!LittleFS.begin()) {
        _lastError = "Failed to mount filesystem";
        return false;
    }
    
    File file = LittleFS.open(filename, "w");
    if (!file) {
        _lastError = "Failed to open file for writing: " + filename;
        return false;
    }
    
    size_t written = file.print(csvData);
    file.close();
    
    if (written != csvData.length()) {
        _lastError = "Failed to write complete data to file";
        return false;
    }
    
    return true;
}

String CSVConfigManager::loadCSVFromFile(const String& filename) {
    if (!LittleFS.begin()) {
        _lastError = "Failed to mount filesystem";
        return "";
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        _lastError = "Failed to open file for reading: " + filename;
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    return content;
}

// Placeholder implementations for sensor-specific methods
String CSVConfigManager::exportSensorsToCSV() {
    // Implementation for sensor export if needed
    return "";
}

bool CSVConfigManager::importSensorsFromCSV(const String& csvData) {
    // Implementation for sensor import if needed
    return false;
}


bool CSVConfigManager::_parseCSVLine(const String& line) {
    // Simple CSV parsing - split by commas
    int fieldIndex = 0;
    int startPos = 0;
    String fields[14]; // 14 fields in our CSV format
    
    for (int i = 0; i <= line.length(); i++) {
        if (i == line.length() || line.charAt(i) == ',') {
            if (fieldIndex < 14) {
                fields[fieldIndex] = line.substring(startPos, i);
                fields[fieldIndex].trim();
                // Remove quotes if present
                if (fields[fieldIndex].startsWith("\"") && fields[fieldIndex].endsWith("\"")) {
                    fields[fieldIndex] = fields[fieldIndex].substring(1, fields[fieldIndex].length() - 1);
                }
            }
            fieldIndex++;
            startPos = i + 1;
        }
    }
    
    if (fieldIndex < 14) {
        _lastError = "Insufficient fields in CSV line";
        return false;
    }
    
    // Parse fields
    int pointAddress = fields[0].toInt();
    
    // Skip sample line (point -1)
    if (pointAddress == -1) {
        return true;
    }
    
    String pointName = fields[1];
    String pointType = fields[2];
    int16_t lowThreshold = fields[6].toInt();
    int16_t highThreshold = fields[7].toInt();
    String romString = fields[8];
    String busNumberStr = fields[9]; // Changed from chipSelectStr
    
    // Update measurement point
    MeasurementPoint* point = _controller.getMeasurementPoint(pointAddress);
    if (point) {
        point->setName(pointName);
        point->setLowAlarmThreshold(lowThreshold);
        point->setHighAlarmThreshold(highThreshold);
        
        // Bind sensor if specified
        if (!romString.isEmpty()) {
            _controller.bindSensorToPointByRom(romString, pointAddress);
        } else if (!busNumberStr.isEmpty()) {
            int busNumber = busNumberStr.toInt();
            _controller.bindSensorToPointByBusNumber(busNumber, pointAddress); // Changed method name
        }
    }
    
    // Parse alarms for each type (rest remains the same)
    String alarmPriorities[4] = {fields[10], fields[11], fields[12], fields[13]};
    AlarmType alarmTypes[4] = {
        AlarmType::HIGH_TEMPERATURE,
        AlarmType::LOW_TEMPERATURE,
        AlarmType::SENSOR_ERROR,
        AlarmType::SENSOR_DISCONNECTED
    };
    
    for (int i = 0; i < 4; i++) {
        if (!alarmPriorities[i].isEmpty()) {
            AlarmPriority priority = _parsePriority(alarmPriorities[i]);
            if (!_controller.addAlarm(alarmTypes[i], pointAddress, priority)) {
                _lastError = "Failed to add alarm for point " + String(pointAddress);
                return false;
            }
        }
    }
    
    return true;
}


String CSVConfigManager::_getAlarmPriorityForPoint(int pointAddress, AlarmType alarmType) {
    // Find alarm of specific type for this point
    for (int i = 0; i < _controller.getAlarmCount(); i++) {
        Alarm* alarm = _controller.getAlarmByIndex(i);
        if (alarm && alarm->getPointAddress() == pointAddress && alarm->getType() == alarmType) {
            return _getPriorityString(alarm->getPriority());
        }
    }
    return ""; // No alarm of this type for this point
}
