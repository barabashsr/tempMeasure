#include "SettingsCSVManager.h"

SettingsCSVManager::SettingsCSVManager(ConfigAssist& config) 
    : _config(config), _lastError("") {
}

String SettingsCSVManager::exportSettingsToCSV() {
    String csv = "Setting,Value,Description\n";
    
    // Device Configuration
    csv += "device_id," + _escapeCSVField(_config("device_id")) + ",Modbus device identifier (1-9999)\n";
    csv += "firmware_version," + _escapeCSVField(_config("firmware_version")) + ",Firmware version (read-only)\n";
    csv += "measurement_period," + _escapeCSVField(_config("measurement_period")) + ",Measurement period in seconds (1-3600)\n";
    csv += "host_name," + _escapeCSVField(_config("host_name")) + ",Device network hostname\n";
    
    // WiFi Settings
    csv += "st_ssid," + _escapeCSVField(_config("st_ssid")) + ",WiFi network name\n";
    csv += "st_pass," + _escapeCSVField(_config("st_pass")) + ",WiFi network password\n";
    
    // Modbus Settings
    csv += "modbus_enabled," + _escapeCSVField(_config("modbus_enabled")) + ",Enable/disable Modbus RTU\n";
    csv += "modbus_address," + _escapeCSVField(_config("modbus_address")) + ",Modbus device address (1-247)\n";
    csv += "modbus_baud_rate," + _escapeCSVField(_config("modbus_baud_rate")) + ",Modbus communication speed\n";
    
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

bool SettingsCSVManager::_parseCSVLine(const String& line) {
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    
    if (firstComma == -1 || secondComma == -1) {
        _lastError = "Invalid CSV line format";
        return false;
    }
    
    String setting = line.substring(0, firstComma);
    String value = line.substring(firstComma + 1, secondComma);
    
    setting.trim();
    value = _unescapeCSVField(value);
    
    // Skip firmware_version as it's read-only
    if (setting == "firmware_version") {
        return true;
    }
    
    // Validate settings exist in config
    String validSettings[] = {
        "device_id", "measurement_period", "host_name",
        "st_ssid", "st_pass", "modbus_enabled", 
        "modbus_address", "modbus_baud_rate"
    };
    
    bool isValid = false;
    for (const String& validSetting : validSettings) {
        if (setting == validSetting) {
            isValid = true;
            break;
        }
    }
    
    if (!isValid) {
        _lastError = "Unknown setting: " + setting;
        return false;
    }
    
    // Set the configuration value
    _config[setting] = value;
    
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
    String result = field;
    result.trim();
    if (result.startsWith("\"") && result.endsWith("\"")) {
        result = result.substring(1, result.length() - 1);
        result.replace("\"\"", "\"");
    }
    return result;
}

bool SettingsCSVManager::validateSettingsCSV(const String& csvData) {
    if (csvData.length() == 0) {
        _lastError = "Empty CSV data";
        return false;
    }
    
    // Check for header line
    int headerLine = csvData.indexOf('\n');
    if (headerLine == -1) {
        _lastError = "No header line found";
        return false;
    }
    
    String header = csvData.substring(0, headerLine);
    if (header.indexOf("Setting") == -1 || header.indexOf("Value") == -1) {
        _lastError = "Invalid header format";
        return false;
    }
    
    return true;
}
