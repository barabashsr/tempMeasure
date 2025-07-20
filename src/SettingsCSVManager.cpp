/**
 * @file SettingsCSVManager.cpp
 * @brief Implementation of CSV import/export for system settings
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements CSV serialization and deserialization of system configuration
 *          with support for special character escaping and validation.
 * 
 * @section dependencies Dependencies
 * - SettingsCSVManager.h for class definition
 * - ConfigAssist for configuration storage
 * 
 * @section format CSV Format
 * - Header: Setting,Value
 * - Fields: key,value pairs
 * - Escaping: Quotes and commas handled per RFC 4180
 */

#include "SettingsCSVManager.h"

SettingsCSVManager::SettingsCSVManager(ConfigAssist& config) 
    : _config(config), _lastError("") {
}

String SettingsCSVManager::exportSettingsToCSV() {
    String csv = "Setting,Value\n";
    
    // WiFi Settings
    csv += "st_ssid," + _escapeCSVField(_config("st_ssid")) + "\n";
    csv += "st_pass," + _escapeCSVField(_config("st_pass")) + "\n";
    csv += "host_name," + _escapeCSVField(_config("host_name")) + "\n";
    
    // Device Settings
    csv += "device_id," + _config("device_id") + "\n";
    csv += "firmware_version," + _escapeCSVField(_config("firmware_version")) + "\n";
    csv += "measurement_period," + _config("measurement_period") + "\n";
    
    // Acknowledged Delay Settings
    csv += "ack_delay_critical," + _config("ack_delay_critical") + "\n";
    csv += "ack_delay_high," + _config("ack_delay_high") + "\n";
    csv += "ack_delay_medium," + _config("ack_delay_medium") + "\n";
    csv += "ack_delay_low," + _config("ack_delay_low") + "\n";
    
    // Modbus Settings
    csv += "modbus_enabled," + _config("modbus_enabled") + "\n";
    csv += "modbus_address," + _config("modbus_address") + "\n";
    csv += "modbus_baud_rate," + _config("modbus_baud_rate") + "\n";
    
    return csv;
}

bool SettingsCSVManager::importSettingsFromCSV(const String& csvData) {
    if (!validateSettingsCSV(csvData)) {
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

bool SettingsCSVManager::validateSettingsCSV(const String& csvData) {
    if (csvData.length() == 0) {
        _lastError = "Empty CSV data";
        return false;
    }
    
    // Check for header line
    int headerEnd = csvData.indexOf('\n');
    if (headerEnd == -1) {
        _lastError = "No header line found";
        return false;
    }
    
    String header = csvData.substring(0, headerEnd);
    if (header.indexOf("Setting") == -1 || header.indexOf("Value") == -1) {
        _lastError = "Invalid header format. Expected 'Setting,Value'";
        return false;
    }
    
    return true;
}

bool SettingsCSVManager::_parseCSVLine(const String& line) {
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1) {
        _lastError = "Invalid CSV line format: " + line;
        return false;
    }
    
    String key = line.substring(0, commaIndex);
    String value = line.substring(commaIndex + 1);
    
    key.trim();
    value = _unescapeCSVField(value);
    
    // Validate and set configuration values
    if (key == "st_ssid" || key == "st_pass" || key == "host_name" || 
        key == "firmware_version" || key == "modbus_baud_rate") {
        // String values
        _config[key] = value;
    } else if (key == "device_id") {
        int deviceId = value.toInt();
        if (deviceId < 1 || deviceId > 9999) {
            _lastError = "Invalid device_id: " + value + " (must be 1-9999)";
            return false;
        }
        _config[key] = value;
    } else if (key == "measurement_period") {
        int period = value.toInt();
        if (period < 1 || period > 3600) {
            _lastError = "Invalid measurement_period: " + value + " (must be 1-3600)";
            return false;
        }
        _config[key] = value;
    } else if (key == "ack_delay_critical" || key == "ack_delay_high" || 
               key == "ack_delay_medium" || key == "ack_delay_low") {
        // Acknowledged delay validation
        int delay = value.toInt();
        if (delay < 1 || delay > 1440) {
            _lastError = "Invalid " + key + ": " + value + " (must be 1-1440 minutes)";
            return false;
        }
        _config[key] = value;
    } else if (key == "modbus_enabled") {
        if (value != "0" && value != "1") {
            String lowerValue = value;
            lowerValue.toLowerCase();
            if (lowerValue != "true" && lowerValue != "false") {
                _lastError = "Invalid modbus_enabled: " + value + " (must be 0/1 or true/false)";
                return false;
            }
            if (lowerValue == "true") value = "1";
            if (lowerValue == "false") value = "0";
        }
        _config[key] = value;
    } else if (key == "modbus_address") {
        int address = value.toInt();
        if (address < 1 || address > 247) {
            _lastError = "Invalid modbus_address: " + value + " (must be 1-247)";
            return false;
        }
        _config[key] = value;
    } else {
        // Unknown setting - log warning but don't fail
        Serial.println("Warning: Unknown setting in CSV: " + key);
        _config[key] = value;  // Still import it in case it's a future setting
    }
    
    return true;
}


String SettingsCSVManager::_escapeCSVField(const String& field) {
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

String SettingsCSVManager::_unescapeCSVField(const String& field) {
    String trimmed = field;
    trimmed.trim();
    
    if (trimmed.startsWith("\"") && trimmed.endsWith("\"")) {
        String unescaped = trimmed.substring(1, trimmed.length() - 1);
        unescaped.replace("\"\"", "\"");
        return unescaped;
    }
    return trimmed;
}
