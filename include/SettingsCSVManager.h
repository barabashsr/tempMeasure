/**
 * @file SettingsCSVManager.h
 * @brief CSV import/export manager for system settings
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Provides functionality to export and import system configuration
 *          settings in CSV format for easy backup and configuration management.
 * 
 * @section dependencies Dependencies
 * - ConfigAssist for configuration management
 * - Arduino String for data handling
 * 
 * @section features Features
 * - Export all settings to CSV format
 * - Import settings from CSV with validation
 * - Handle special characters and escaping
 * - Support for acknowledged delay settings
 */

#pragma once

#include <Arduino.h>
#include "ConfigAssist.h"

/**
 * @class SettingsCSVManager
 * @brief Manages CSV export/import of system settings
 * @details Provides methods to convert system configuration to/from CSV format,
 *          with validation and error handling for safe configuration updates.
 */
class SettingsCSVManager {
public:
    /**
     * @brief Construct a new Settings CSV Manager object
     * @param[in] config Reference to ConfigAssist instance
     */
    SettingsCSVManager(ConfigAssist& config);
    
    // CSV export/import for settings
    /**
     * @brief Export all settings to CSV format
     * @return String CSV formatted settings
     * @details Generates CSV with key-value pairs for all configuration items
     */
    String exportSettingsToCSV();
    
    /**
     * @brief Import settings from CSV data
     * @param[in] csvData CSV formatted configuration data
     * @return true if import successful
     * @return false if CSV invalid or import failed
     * @details Validates and applies settings from CSV input
     */
    bool importSettingsFromCSV(const String& csvData);
    
    // Validation
    /**
     * @brief Validate CSV data without importing
     * @param[in] csvData CSV data to validate
     * @return true if CSV format is valid
     * @return false if CSV format is invalid
     * @details Checks CSV structure and field validity
     */
    bool validateSettingsCSV(const String& csvData);
    
    /**
     * @brief Get last error message
     * @return String Description of last error
     */
    String getLastError() const { return _lastError; }

private:
    ConfigAssist& _config;              ///< Reference to configuration manager
    String _lastError;                  ///< Last error message
    
    // Helper methods
    /**
     * @brief Escape special characters for CSV field
     * @param[in] field Field value to escape
     * @return String Escaped field value
     * @details Handles quotes, commas, and newlines
     */
    String _escapeCSVField(const String& field);
    
    /**
     * @brief Unescape CSV field value
     * @param[in] field Escaped field value
     * @return String Unescaped field value
     */
    String _unescapeCSVField(const String& field);
    
    /**
     * @brief Parse single CSV line
     * @param[in] line CSV line to parse
     * @return true if line parsed successfully
     * @return false if parsing failed
     */
    bool _parseCSVLine(const String& line);
    
    // Add these helper methods for acknowledged delays
    /**
     * @brief Export acknowledged delay settings to CSV
     * @param[out] csv String to append delay settings to
     * @details Exports alarm acknowledgment delay configurations
     */
    void _exportAcknowledgedDelays(String& csv);
    
    /**
     * @brief Import acknowledged delay settings
     * @param[in] key Configuration key
     * @param[in] value Configuration value
     * @return true if import successful
     * @return false if import failed
     */
    bool _importAcknowledgedDelays(const String& key, const String& value);
};
