#include "LoggerManager.h"

LoggerManager::LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem)
    : _controller(&controller), _timeManager(&timeManager), _fs(&filesystem),
      _logFrequency(60000), _lastLogTime(0), _headerWritten(false),
      _enabled(true), _logDirectory(""), _dailyFiles(true), _lastError("") {
}

LoggerManager::~LoggerManager() {
    closeCurrentFile();
}

bool LoggerManager::begin() {
    if (!_ensureDirectoryExists()) {
        _lastError = "Failed to create log directory";
        return false;
    }
    
    // Generate initial log file name
    _currentLogFile = _generateLogFileName();
    _lastLogDate = _getCurrentDateString();
    
    Serial.printf("LoggerManager initialized. Log file: %s\n", _currentLogFile.c_str());
    Serial.printf("Log frequency: %lu ms\n", _logFrequency);
    
    return true;
}

void LoggerManager::setLogFrequency(unsigned long frequencyMs) {
    if (frequencyMs < 1000) frequencyMs = 1000; // Minimum 1 second
    _logFrequency = frequencyMs;
    Serial.printf("Log frequency set to %lu ms\n", _logFrequency);
}

unsigned long LoggerManager::getLogFrequency() const {
    return _logFrequency;
}

void LoggerManager::setEnabled(bool enabled) {
    _enabled = enabled;
    Serial.printf("Logging %s\n", enabled ? "enabled" : "disabled");
}

bool LoggerManager::isEnabled() const {
    return _enabled;
}

void LoggerManager::setDailyFiles(bool enabled) {
    _dailyFiles = enabled;
}

bool LoggerManager::isDailyFiles() const {
    return _dailyFiles;
}

void LoggerManager::setLogDirectory(const String& directory) {
    _logDirectory = directory;
    if (!_logDirectory.startsWith("/")) {
        _logDirectory = "/" + _logDirectory;
    }
}

String LoggerManager::getLogDirectory() const {
    return _logDirectory;
}

void LoggerManager::update() {
    if (!_enabled) return;
    
    unsigned long currentTime = millis();
    
    // Check if it's time to log
    if (currentTime - _lastLogTime >= _logFrequency) {
        // Check if we need a new daily file
        if (_dailyFiles) {
            String currentDate = _getCurrentDateString();
            if (currentDate != _lastLogDate) {
                createNewLogFile();
                _lastLogDate = currentDate;
            }
        }
        
        logDataNow();
    }
}

bool LoggerManager::logDataNow() {
    if (!_enabled) return false;
    
    // Write header if this is a new file
    if (!_headerWritten) {
        if (!_writeHeader()) {
            return false;
        }
        _headerWritten = true;
    }
    
    // Write data row
    if (!_writeDataRow()) {
        return false;
    }
    
    _lastLogTime = millis();
    return true;
}

bool LoggerManager::createNewLogFile() {
    closeCurrentFile();
    _currentLogFile = _generateLogFileName();
    _headerWritten = false;
    
    Serial.printf("Created new log file: %s\n", _currentLogFile.c_str());
    return true;
}

String LoggerManager::getCurrentLogFile() const {
    return _currentLogFile;
}

bool LoggerManager::closeCurrentFile() {
    // SD library automatically handles file closing
    return true;
}

unsigned long LoggerManager::getLastLogTime() const {
    return _lastLogTime;
}

String LoggerManager::getLastError() const {
    return _lastError;
}

// Private methods implementation

String LoggerManager::_generateLogFileName() {
    String dateStr = _getCurrentDateString();
    String filename;
    
    if (_logDirectory.isEmpty()) {
        filename = "/temp_log_" + dateStr + ".csv";  // Root directory
    } else {
        filename = _logDirectory + "/temp_log_" + dateStr + ".csv";
    }
    
    return filename;
}

String LoggerManager::_generateCSVHeader() {
    String header = "Date,Time";
    
    // Add all 60 measurement points (0-59)
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = _controller->getMeasurementPoint(i);
        if (point) {
            String pointName = point->getName();
            if (pointName.isEmpty()) {
                pointName = "Point_" + String(i);
            }
            header += "," + String(i) + "." + _escapeCSVField(pointName);
        } else {
            header += "," + String(i) + ".Point_" + String(i);
        }
    }
    
    header += "\n";
    return header;
}

bool LoggerManager::_writeHeader() {
    File file = _fs->open(_currentLogFile.c_str(), FILE_WRITE);
    if (!file) {
        _lastError = "Failed to open log file for header writing: " + _currentLogFile;
        return false;
    }
    
    String header = _generateCSVHeader();
    size_t written = file.print(header);
    file.close();
    
    if (written != header.length()) {
        _lastError = "Failed to write complete header";
        return false;
    }
    
    Serial.printf("Header written to %s\n", _currentLogFile.c_str());
    return true;
}

bool LoggerManager::_writeDataRow() {
    File file = _fs->open(_currentLogFile.c_str(), FILE_APPEND);
    if (!file) {
        _lastError = "Failed to open log file for data writing: " + _currentLogFile;
        return false;
    }
    
    // Build data row
    String dataRow = _getCurrentDateString() + "," + _getCurrentTimeString();
    
    // Add temperature data for all 60 points
    for (int i = 0; i < 60; i++) {
        MeasurementPoint* point = _controller->getMeasurementPoint(i);
        if (point && point->getBoundSensor()) {
            // Point has a bound sensor, log actual temperature
            dataRow += "," + String(point->getCurrentTemp());
        } else {
            // No bound sensor, log empty value
            dataRow += ",";
        }
    }
    
    dataRow += "\n";
    
    size_t written = file.print(dataRow);
    file.close();
    
    if (written != dataRow.length()) {
        _lastError = "Failed to write complete data row";
        return false;
    }
    
    return true;
}

String LoggerManager::_escapeCSVField(const String& field) {
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

bool LoggerManager::_ensureDirectoryExists() {
    // For SD card, always use root directory
    if (_logDirectory.isEmpty() || _logDirectory == "/") {
        return true; // Root directory always exists
    }
    
    // Try to check if custom directory exists, but don't fail if it doesn't
    File dir = _fs->open(_logDirectory.c_str());
    if (!dir) {
        Serial.printf("Directory %s does not exist, using root directory\n", _logDirectory.c_str());
        _logDirectory = ""; // Fall back to root
        return true;
    }
    
    if (!dir.isDirectory()) {
        dir.close();
        Serial.printf("Path %s exists but is not a directory, using root\n", _logDirectory.c_str());
        _logDirectory = ""; // Fall back to root
        return true;
    }
    
    dir.close();
    return true;
}

String LoggerManager::_getCurrentDateString() {
    if (_timeManager && _timeManager->isTimeSet()) {
        return _timeManager->getDateString(); // Assuming this method exists
    } else {
        // Fallback to millis-based date
        unsigned long days = millis() / (24UL * 60UL * 60UL * 1000UL);
        return "Day_" + String(days);
    }
}

String LoggerManager::_getCurrentTimeString() {
    if (_timeManager && _timeManager->isTimeSet()) {
        return _timeManager->getTimeString(); // Assuming this method exists
    } else {
        // Fallback to millis-based time
        unsigned long totalSeconds = (millis() / 1000) % (24 * 60 * 60);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
        return String(timeStr);
    }
}

std::vector<String> LoggerManager::getLogFiles() {
    std::vector<String> files;
    
    File dir = _fs->open(_logDirectory.c_str());
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".csv")) {
            files.push_back(String(file.name()));
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    return files;
}

bool LoggerManager::deleteLogFile(const String& filename) {
    String fullPath = _logDirectory + "/" + filename;
    return _fs->remove(fullPath.c_str());
}

size_t LoggerManager::getLogFileSize() const {
    File file = _fs->open(_currentLogFile.c_str(), FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}
