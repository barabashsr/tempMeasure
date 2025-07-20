/**
 * @file CSVConfigManager.h
 * @brief CSV configuration import/export manager for measurement points and alarms
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file provides CSV-based configuration management for measurement points
 *          and alarm settings, enabling bulk configuration through CSV files.
 * 
 * @section dependencies Dependencies
 * - CSV_Parser.h for CSV parsing functionality
 * - TemperatureController.h for system integration
 * 
 * @section hardware Hardware Requirements
 * - ESP32 with LittleFS support for file operations
 */

#pragma once

#include <Arduino.h>
#include <CSV_Parser.h>
#include "TemperatureController.h"

/**
 * @brief CSV configuration manager for measurement points and alarms
 * @details Handles import/export of system configuration through CSV files,
 *          enabling bulk configuration management and backup/restore operations
 */
class CSVConfigManager {
public:
    /**
     * @brief Constructor for CSV configuration manager
     * @param[in] controller Reference to the temperature controller
     * @details Initializes CSV manager with temperature controller integration
     */
    CSVConfigManager(TemperatureController& controller);
    
    // CSV export/import for combined points and alarms
    /**
     * @brief Export measurement points with alarm configuration to CSV
     * @return String CSV formatted string containing all measurement points and alarms
     * @details Generates comprehensive CSV export including point names, sensor configs, and alarm settings
     */
    String exportPointsWithAlarmsToCSV();
    
    /**
     * @brief Import measurement points and alarms from CSV data
     * @param[in] csvData CSV formatted string containing configuration
     * @return bool True if import successful, false if validation or parsing failed
     * @details Parses CSV data and applies configuration to measurement points and alarms
     */
    bool importPointsWithAlarmsFromCSV(const String& csvData);
    
    // Individual exports (if needed)
    /**
     * @brief Export only sensor configuration to CSV
     * @return String CSV formatted string containing sensor configuration
     * @details Exports sensor-specific settings without alarm configuration
     */
    String exportSensorsToCSV();
    
    /**
     * @brief Import sensor configuration from CSV data
     * @param[in] csvData CSV formatted string containing sensor configuration
     * @return bool True if import successful
     * @details Parses and applies sensor configuration from CSV data
     */
    bool importSensorsFromCSV(const String& csvData);
    
    // File operations
    /**
     * @brief Save CSV data to file system
     * @param[in] filename Name of file to save (will be stored in LittleFS)
     * @param[in] csvData CSV data to write to file
     * @return bool True if file save successful
     * @details Writes CSV data to LittleFS file system with error handling
     */
    bool saveCSVToFile(const String& filename, const String& csvData);
    
    /**
     * @brief Load CSV data from file system
     * @param[in] filename Name of file to load from LittleFS
     * @return String CSV data from file, empty string if load failed
     * @details Reads CSV data from LittleFS with error handling
     */
    String loadCSVFromFile(const String& filename);
    
    // Validation
    /**
     * @brief Validate CSV data format and content
     * @param[in] csvData CSV data to validate
     * @return bool True if CSV data is valid and can be imported
     * @details Performs comprehensive validation of CSV structure, data types, and ranges
     */
    bool validatePointsCSV(const String& csvData);
    
    /**
     * @brief Get the last error message
     * @return String Description of the last error that occurred
     * @details Provides detailed error information for debugging import/export issues
     */
    String getLastError() const { return _lastError; }

private:
    TemperatureController& _controller;  ///< Reference to temperature controller
    String _lastError;                   ///< Last error message for debugging
    
    // Helper methods
    /**
     * @brief Escape special characters in CSV field
     * @param[in] field String field to escape
     * @return String Properly escaped CSV field
     * @details Handles quotes, commas, and newlines in CSV fields
     */
    String _escapeCSVField(const String& field);
    
    /**
     * @brief Unescape CSV field back to original string
     * @param[in] field Escaped CSV field
     * @return String Original unescaped string
     * @details Reverses CSV field escaping
     */
    String _unescapeCSVField(const String& field);
    
    /**
     * @brief Parse alarm configuration from CSV fields
     * @param[in] pointAddress Measurement point address
     * @param[in] alarmType Alarm type as string
     * @param[in] priority Alarm priority as string
     * @param[in] enabled Alarm enabled state as string
     * @param[in] hysteresis Hysteresis value as string
     * @return bool True if alarm parsing successful
     * @details Converts CSV string fields to alarm configuration
     */
    bool _parseAlarmFromCSV(int pointAddress, const String& alarmType, 
                           const String& priority, const String& enabled, 
                           const String& hysteresis);
    
    // Additional helper methods
    /**
     * @brief Convert alarm type enum to string
     * @param[in] type AlarmType enum value
     * @return String Human-readable alarm type string
     */
    String _getAlarmTypeString(AlarmType type);
    
    /**
     * @brief Parse alarm type string to enum
     * @param[in] typeStr Alarm type as string
     * @return AlarmType Corresponding enum value
     */
    AlarmType _parseAlarmType(const String& typeStr);
    
    /**
     * @brief Convert alarm priority enum to string
     * @param[in] priority AlarmPriority enum value
     * @return String Human-readable priority string
     */
    String _getPriorityString(AlarmPriority priority);
    
    /**
     * @brief Parse priority string to enum
     * @param[in] priorityStr Priority as string
     * @return AlarmPriority Corresponding enum value
     */
    AlarmPriority _parsePriority(const String& priorityStr);
    
    /**
     * @brief Export single measurement point to CSV format
     * @param[in,out] csv CSV string to append to
     * @param[in] point Measurement point to export
     * @param[in] pointType Type of measurement point (DS18B20, PT1000, etc.)
     * @details Appends measurement point configuration to CSV string
     */
    void _exportPointToCSV(String& csv, MeasurementPoint* point, const String& pointType);
    
    /**
     * @brief Parse single CSV line
     * @param[in] line CSV line to parse
     * @return bool True if line parsing successful
     * @details Parses individual CSV line and applies configuration
     */
    bool _parseCSVLine(const String& line);
    
    /**
     * @brief Get alarm priority for specific point and alarm type
     * @param[in] pointAddress Measurement point address
     * @param[in] alarmType Type of alarm
     * @return String Priority level as string
     * @details Retrieves current alarm priority configuration
     */
    String _getAlarmPriorityForPoint(int pointAddress, AlarmType alarmType);
};
