#pragma once

#include <Arduino.h>
#include <CSV_Parser.h>
#include "TemperatureController.h"

class CSVConfigManager {
public:
    CSVConfigManager(TemperatureController& controller);
    
    // CSV export/import for combined points and alarms
    String exportPointsWithAlarmsToCSV();
    bool importPointsWithAlarmsFromCSV(const String& csvData);
    
    // Individual exports (if needed)
    String exportSensorsToCSV();
    bool importSensorsFromCSV(const String& csvData);
    
    // File operations
    bool saveCSVToFile(const String& filename, const String& csvData);
    String loadCSVFromFile(const String& filename);
    
    // Validation
    bool validatePointsCSV(const String& csvData);
    String getLastError() const { return _lastError; }

private:
    TemperatureController& _controller;
    String _lastError;
    
    // Helper methods
    String _escapeCSVField(const String& field);
    String _unescapeCSVField(const String& field);
    bool _parseAlarmFromCSV(int pointAddress, const String& alarmType, 
                           const String& priority, const String& enabled, 
                           const String& hysteresis);
    
    // Additional helper methods needed
    String _getAlarmTypeString(AlarmType type);
    AlarmType _parseAlarmType(const String& typeStr);
    String _getPriorityString(AlarmPriority priority);
    AlarmPriority _parsePriority(const String& priorityStr);
    void _exportPointToCSV(String& csv, MeasurementPoint* point, const String& pointType);
    bool _parseCSVLine(const String& line);
    String _getAlarmPriorityForPoint(int pointAddress, AlarmType alarmType);
};
