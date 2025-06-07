#pragma once

#include <Arduino.h>
#include "ConfigAssist.h"

class SettingsCSVManager {
public:
    SettingsCSVManager(ConfigAssist& config);
    
    // CSV export/import for settings
    String exportSettingsToCSV();
    bool importSettingsFromCSV(const String& csvData);
    
    // Validation
    bool validateSettingsCSV(const String& csvData);
    String getLastError() const { return _lastError; }

private:
    ConfigAssist& _config;
    String _lastError;
    
    // Helper methods
    String _escapeCSVField(const String& field);
    String _unescapeCSVField(const String& field);
    bool _parseCSVLine(const String& line);
};
