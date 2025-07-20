/**
 * @file LoggerManager.cpp
 * @brief Implementation of data logging and event management system
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements LoggerManager for recording temperature data, system events,
 *          and alarm states to CSV files with configurable intervals and formats.
 * 
 * @section dependencies Dependencies
 * - LoggerManager.h for class definition
 * - TemperatureController for temperature data access
 * - TimeManager for timestamps
 * - LittleFS/SPIFFS for file storage
 * 
 * @section features Features
 * - Periodic temperature data logging to CSV
 * - Event logging with timestamps
 * - Alarm state change logging
 * - Daily file rotation
 * - Configurable logging intervals
 * - Dynamic header generation based on active sensors
 */

#include "LoggerManager.h"
#include "TemperatureController.h"  // Full include in .cpp file
#include "MeasurementPoint.h"       // Add this include


LoggerManager* LoggerManager::_instance = nullptr;

LoggerManager::LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem)
    : _controller(&controller), _timeManager(&timeManager), _fs(&filesystem),
      _logFrequency(60000), _lastLogTime(0), _headerWritten(false),
      _enabled(true), _logDirectory(""), _dailyFiles(true), _lastError(""),
      _lastGeneratedHeader(""), _headerChanged(false), _fileSequenceNumber(0),
      _eventLoggingEnabled(true), _eventLogDirectory(""), _currentEventLogFile(""), _lastEventLogDate(""),
      _alarmStateLoggingEnabled(true), _alarmStateLogDirectory(""), _currentAlarmStateLogFile(""), _lastAlarmStateLogDate("") {
        _instance = this;
}


LoggerManager::~LoggerManager() {
    closeCurrentFile();
}

bool LoggerManager::begin() {
    if (!_enabled) return false;
    if(_enabled) {
    if (!_ensureDirectoryExists()) {
        _lastError = "Failed to create log directory";
        return false;
    }
    
    // Recover from existing files after reboot
    if (!_recoverFromExistingFiles()) {
        Serial.println("Warning: Could not recover from existing files, starting fresh");
        // Initialize fresh
        _fileSequenceNumber = 0;
        _headerWritten = false;
    }
    
    // Generate current header for comparison
    _lastGeneratedHeader = _generateCSVHeader();
    
    // Generate log file name with recovered sequence number
    _currentLogFile = _generateLogFileNameWithSequence();
    _lastLogDate = _getCurrentDateString();
   
    // Initialize alarm state logging
    if (_alarmStateLoggingEnabled) {
        _lastAlarmStateLogDate = _getCurrentDateString();
        _currentAlarmStateLogFile = _generateAlarmStateLogFileName();
        
        if (!_ensureAlarmStateLogExists()) {
            Serial.println("Warning: Could not initialize alarm state log file");
        } else {
            Serial.printf("Alarm state logging initialized. Log file: %s\n", _currentAlarmStateLogFile.c_str());
            logInfo("SYSTEM", "LoggerManager alarm state logging initialized successfully");
        }
    }
    
    Serial.printf("LoggerManager initialized. Log file: %s\n", _currentLogFile.c_str());
    Serial.printf("File sequence number: %d\n", _fileSequenceNumber);
    Serial.printf("Log frequency: %lu ms\n", _logFrequency);
    } else {
        Serial.println("No SD card");
        _enabled = false;
        return false;
    }
    
    return true;
}

bool LoggerManager::init() {
    //if (!_enabled) return false;
    if(_enabled) {

    if (!_ensureDirectoryExists()) {
        _lastError = "Failed to create log directory";
        return false;
    }
    
    // Initialize event logging
    if (_eventLoggingEnabled) {
        _lastEventLogDate = _getCurrentDateString();
        _currentEventLogFile = _generateEventLogFileName();
        
        if (!_ensureEventLogExists()) {
            Serial.println("Warning: Could not initialize event log file");
        } else {
            Serial.printf("Event logging initialized. Event log file: %s\n", _currentEventLogFile.c_str());
            
            // Log system startup event
            logInfo("SYSTEM", "LoggerManager event logging initialized successfully");
        }
    }
    
    Serial.printf("LoggerManager event logging initialized. Log file: %s\n", _currentLogFile.c_str());
    } else {
        Serial.println("No SD card");
        _enabled = false;
        return false;
        

    }
    
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

String LoggerManager::getLogDirectory() {
    return _instance->_logDirectory;
}

void LoggerManager::update() {
    if (!_enabled) return;
    
    unsigned long currentTime = millis();
    
    // Check if it's time to log temperature data
    if (currentTime - _lastLogTime >= _logFrequency) {
        // Check if we need new daily files
        if (_dailyFiles) {
            String currentDate = _getCurrentDateString();
            if (currentDate != _lastLogDate) {
                // New day - recover from existing files for new date
                _recoverFromExistingFiles();
                _lastLogDate = currentDate;
                
                // Also update event log file for new day
                if (_eventLoggingEnabled && currentDate != _lastEventLogDate) {
                    _lastEventLogDate = currentDate;
                    _currentEventLogFile = _generateEventLogFileName();
                    logInfo("SYSTEM", "New day - event log file created: " + _currentEventLogFile);
                }
            }
        }
        
        // Check if header has changed (point names changed)
        if (_hasHeaderChanged()) {
            Serial.println("Header changed - creating new log file");
            _incrementSequenceNumber();
            createNewLogFile();
            
            // Log the configuration change event
            if (_eventLoggingEnabled) {
                logWarning("CONFIG", "Measurement point configuration changed - new data log file created");
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
    return _generateLogFileNameWithSequence();
}

String LoggerManager::_generateLogFileNameWithSequence() {
    
    String dateStr = _getCurrentDateString();
    String filename;
    
    // Always ensure we have a valid path starting with /
    if (_logDirectory.isEmpty()) {
        filename = "/temp_log_" + dateStr + "_" + String(_fileSequenceNumber) + ".csv";
    } else {
        // Ensure _logDirectory starts with /
        String dir = _logDirectory;
        if (!dir.startsWith("/")) {
            dir = "/" + dir;
        }
        filename = dir + "/temp_log_" + dateStr + "_" + String(_fileSequenceNumber) + ".csv";
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
    if (!_enabled) return false;
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
    if (!_enabled) return false;
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
    if (!_enabled) return false;
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



bool LoggerManager::deleteLogFile(const String& filename) {
    if (!_enabled) return false;
    String fullPath = _logDirectory + "/" + filename;
    return _fs->remove(fullPath.c_str());
}

size_t LoggerManager::getLogFileSize() const {
    if (!_enabled) return -1;
    File file = _fs->open(_currentLogFile.c_str(), FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}


bool LoggerManager::_hasHeaderChanged() {
    String currentHeader = _generateCSVHeader();
    
    // If this is the first time, store the header
    if (_lastGeneratedHeader.isEmpty()) {
        _lastGeneratedHeader = currentHeader;
        return false;
    }
    
    // Compare with last generated header
    if (currentHeader != _lastGeneratedHeader) {
        Serial.println("Header change detected:");
        Serial.println("Old header: " + _lastGeneratedHeader);
        Serial.println("New header: " + currentHeader);
        
        _lastGeneratedHeader = currentHeader;
        return true;
    }
    
    return false;
}

void LoggerManager::_incrementSequenceNumber() {
    _fileSequenceNumber++;
    Serial.printf("File sequence number incremented to: %d\n", _fileSequenceNumber);
}





void LoggerManager::forceNewFile() {
    if (!_enabled) return;
    _incrementSequenceNumber();
    createNewLogFile();
    Serial.println("Manually forced new log file creation");
}

int LoggerManager::getCurrentSequenceNumber() const {
    return _fileSequenceNumber;
}

void LoggerManager::resetSequenceNumber() {
    _fileSequenceNumber = 0;
    Serial.println("File sequence number reset to 0");
}

bool LoggerManager::_recoverFromExistingFiles() {
    if (!_enabled) return false;
    String currentDate = _getCurrentDateString();
    
    // Find all files for today
    std::vector<String> todaysFiles = _getFilesForDate(currentDate);
    
    if (todaysFiles.empty()) {
        // No files for today, start fresh
        _fileSequenceNumber = 0;
        _headerWritten = false;
        Serial.printf("No existing files for date %s, starting with sequence 0\n", currentDate.c_str());
        return true;
    }
    
    // Find the highest sequence number for today
    int highestSequence = _findHighestSequenceForDate(currentDate);
    
    // Find the latest file (highest sequence number)
    String latestFile = _findLatestFileForDate(currentDate);
    
    if (latestFile.isEmpty()) {
        // Couldn't find latest file, start fresh
        _fileSequenceNumber = 0;
        _headerWritten = false;
        Serial.println("Could not determine latest file, starting fresh");
        return false;
    }
    
    // Read header from latest file
    String existingHeader = _readHeaderFromFile(latestFile);
    String currentHeader = _generateCSVHeader();
    
    if (existingHeader.isEmpty()) {
        // Could not read header, assume file is incomplete
        _fileSequenceNumber = highestSequence;
        _headerWritten = false;
        Serial.printf("Could not read header from %s, will rewrite\n", latestFile.c_str());
        return true;
    }
    
    // Compare headers
    if (existingHeader == currentHeader) {
        // Headers match, continue with existing file
        _fileSequenceNumber = highestSequence;
        _headerWritten = true;
        _currentLogFile = latestFile;
        Serial.printf("Recovered: Using existing file %s (sequence %d)\n", 
                     latestFile.c_str(), _fileSequenceNumber);
        return true;
    } else {
        // Headers don't match, need new file
        _fileSequenceNumber = highestSequence + 1;
        _headerWritten = false;
        Serial.printf("Header changed, creating new file with sequence %d\n", _fileSequenceNumber);
        Serial.println("Old header: " + existingHeader);
        Serial.println("New header: " + currentHeader);
        return true;
    }
}

std::vector<String> LoggerManager::_getFilesForDate(const String& dateStr) {

    std::vector<String> files;
    
    // Open directory (root if _logDirectory is empty)
    String dirPath = _logDirectory.isEmpty() ? "/" : _logDirectory;
    File dir = _fs->open(dirPath.c_str());
    
    if (!dir || !dir.isDirectory()) {
        Serial.printf("Could not open directory: %s\n", dirPath.c_str());
        return files;
    }
    
    // Pattern to match: temp_log_YYYY-MM-DD_N.csv
    String pattern = "temp_log_" + dateStr + "_";
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        
        // Check if filename matches our pattern and is a CSV file
        if (filename.startsWith(pattern) && filename.endsWith(".csv")) {
            String fullPath = dirPath;
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += filename;
            files.push_back(fullPath);
        }
        
        file = dir.openNextFile();
    }
    
    dir.close();
    
    Serial.printf("Found %d files for date %s\n", files.size(), dateStr.c_str());
    return files;
}

int LoggerManager::_findHighestSequenceForDate(const String& dateStr) {
    std::vector<String> files = _getFilesForDate(dateStr);
    int highestSequence = -1;
    
    for (const String& filepath : files) {
        int sequence = _extractSequenceNumber(filepath);
        if (sequence > highestSequence) {
            highestSequence = sequence;
        }
    }
    
    return highestSequence >= 0 ? highestSequence : 0;
}

String LoggerManager::_findLatestFileForDate(const String& dateStr) {
    std::vector<String> files = _getFilesForDate(dateStr);
    String latestFile = "";
    int highestSequence = -1;
    
    for (const String& filepath : files) {
        int sequence = _extractSequenceNumber(filepath);
        if (sequence > highestSequence) {
            highestSequence = sequence;
            latestFile = filepath;
        }
    }
    
    return latestFile;
}

int LoggerManager::_extractSequenceNumber(const String& filename) {
    // Extract sequence number from filename like: temp_log_2025-06-15_3.csv
    int lastUnderscore = filename.lastIndexOf('_');
    int dotIndex = filename.lastIndexOf('.');
    
    if (lastUnderscore == -1 || dotIndex == -1 || lastUnderscore >= dotIndex) {
        return -1; // Invalid format
    }
    
    String sequenceStr = filename.substring(lastUnderscore + 1, dotIndex);
    return sequenceStr.toInt();
}

String LoggerManager::_readHeaderFromFile(const String& filename) {

    File file = _fs->open(filename.c_str(), FILE_READ);
    if (!file) {
        Serial.printf("Could not open file for header reading: %s\n", filename.c_str());
        return "";
    }
    
    // Read first line (header)
    String header = "";
    while (file.available()) {
        char c = file.read();
        if (c == '\n') {
            break;
        }
        header += c;
    }
    
    file.close();
    
    // Add newline back for comparison
    if (!header.isEmpty()) {
        header += "\n";
    }
    
    return header;
}


bool LoggerManager::createNewLogFile() {
    if (!_enabled) return false;
    closeCurrentFile();
    
    // Generate new filename with current sequence number
    _currentLogFile = _generateLogFileNameWithSequence();
    _headerWritten = false;
    
    // Update the stored header to current state
    _lastGeneratedHeader = _generateCSVHeader();
    
    Serial.printf("Created new log file: %s (sequence: %d)\n", 
                 _currentLogFile.c_str(), _fileSequenceNumber);
    return true;
}



// Event logging configuration methods
void LoggerManager::setEventLoggingEnabled(bool enabled) {
    _eventLoggingEnabled = enabled;
    Serial.printf("Event logging %s\n", enabled ? "enabled" : "disabled");
}

bool LoggerManager::isEventLoggingEnabled() const {
    return _eventLoggingEnabled;
}

void LoggerManager::setEventLogDirectory(const String& directory) {
    _eventLogDirectory = directory;
    if (!_eventLogDirectory.startsWith("/")) {
        _eventLogDirectory = "/" + _eventLogDirectory;
    }
}

String LoggerManager::getEventLogDirectory() const {
    return _eventLogDirectory;
}

// Main event logging method
bool LoggerManager::logEvent(const String& source, const String& description, const String& priority) {
    if (!_enabled) return false;
    if (!_eventLoggingEnabled) return false;
    
    // Check if we need a new event log file for today
    String currentDate = _getCurrentDateString();
    if (currentDate != _lastEventLogDate) {
        _lastEventLogDate = currentDate;
        _currentEventLogFile = _generateEventLogFileName();
    }
    
    // Ensure event log file exists
    if (!_ensureEventLogExists()) {
        return false;
    }
    
    // Generate timestamp
    String timestamp = _getCurrentDateString() + " " + _getCurrentTimeString();
    
    // Write event row
    return _writeEventRow(timestamp, source, description, priority);
}

// Convenience methods for different priority levels
bool LoggerManager::logInfo(const String& source, const String& description) {
    if (!_enabled) return false;
    return logEvent(source, description, "INFO");
}

bool LoggerManager::logWarning(const String& source, const String& description) {
    if (!_enabled) return false;
    return logEvent(source, description, "WARNING");
}

bool LoggerManager::logError(const String& source, const String& description) {
    if (!_enabled) return false;
    return logEvent(source, description, "ERROR");
}

bool LoggerManager::logCritical(const String& source, const String& description) {
    if (!_enabled) return false;
    return logEvent(source, description, "CRITICAL");
}

// Event log file management
String LoggerManager::getCurrentEventLogFile() const {
    return _currentEventLogFile;
}

std::vector<String> LoggerManager::getEventLogFiles() {

    std::vector<String> files;
    
    String dirPath = _eventLogDirectory.isEmpty() ? "/" : _eventLogDirectory;
    File dir = _fs->open(dirPath.c_str());
    
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        if (filename.startsWith("events_") && filename.endsWith(".csv")) {
            String fullPath = dirPath;
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += filename;
            files.push_back(fullPath);
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    return files;
}

bool LoggerManager::deleteEventLogFile(const String& filename) {
    if (!_enabled) return false;
    String fullPath = _eventLogDirectory.isEmpty() ? "/" : _eventLogDirectory;
    if (!fullPath.endsWith("/")) fullPath += "/";
    fullPath += filename;
    return _fs->remove(fullPath.c_str());
}

// Private event logging methods
String LoggerManager::_generateEventLogFileName() {
    String dateStr = _getCurrentDateString();
    String filename;
    
    if (_eventLogDirectory.isEmpty()) {
        filename = "/events_" + dateStr + ".csv";  // Always start with /
    } else {
        String dir = _eventLogDirectory;
        if (!dir.startsWith("/")) {
            dir = "/" + dir;
        }
        filename = dir + "/events_" + dateStr + ".csv";
    }
    
    return filename;
}


bool LoggerManager::_ensureEventLogExists() {
    if (!_enabled) return false;
    // Check if event log file exists
    File file = _fs->open(_currentEventLogFile.c_str(), FILE_READ);
    if (file) {
        file.close();
        return true; // File exists
    }
    
    // File doesn't exist, create it with header
    return _writeEventHeader();
}

bool LoggerManager::_writeEventHeader() {
    if (!_enabled) return false;
    File file = _fs->open(_currentEventLogFile.c_str(), FILE_WRITE);
    if (!file) {
        _lastError = "Failed to open event log file for header writing: " + _currentEventLogFile;
        return false;
    }
    
    String header = "Timestamp,Source,Description,Priority\n";
    size_t written = file.print(header);
    file.close();
    
    if (written != header.length()) {
        _lastError = "Failed to write complete event log header";
        return false;
    }
    
    Serial.printf("Event log header written to %s\n", _currentEventLogFile.c_str());
    return true;
}

bool LoggerManager::_writeEventRow(const String& timestamp, const String& source, 
                                  const String& description, const String& priority) {
    if (!_enabled) return false;
    File file = _fs->open(_currentEventLogFile.c_str(), FILE_APPEND);
    if (!file) {
        _lastError = "Failed to open event log file for writing: " + _currentEventLogFile;
        return false;
    }
    
    // Build event row with proper CSV escaping
    String eventRow = _escapeCSVField(timestamp) + "," + 
                     _escapeCSVField(source) + "," + 
                     _escapeCSVField(description) + "," + 
                     _escapeCSVField(priority) + "\n";
    
    size_t written = file.print(eventRow);
    file.close();
    
    if (written != eventRow.length()) {
        _lastError = "Failed to write complete event log row";
        return false;
    }
    
    // Also print to serial for debugging
    Serial.printf("[%s] %s: %s (%s)\n", timestamp.c_str(), source.c_str(), description.c_str(), priority.c_str());
    
    return true;
}


// Alarm state logging configuration methods
void LoggerManager::setAlarmStateLoggingEnabled(bool enabled) {
    _alarmStateLoggingEnabled = enabled;
    Serial.printf("Alarm state logging %s\n", enabled ? "enabled" : "disabled");
}

bool LoggerManager::isAlarmStateLoggingEnabled() const {
    return _alarmStateLoggingEnabled;
}

void LoggerManager::setAlarmStateLogDirectory(const String& directory) {
    _alarmStateLogDirectory = directory;
    if (!_alarmStateLogDirectory.startsWith("/") && !_alarmStateLogDirectory.isEmpty()) {
        _alarmStateLogDirectory = "/" + _alarmStateLogDirectory;
    }
}

String LoggerManager::getAlarmStateLogDirectory() const {
    return _alarmStateLogDirectory;
}

// Static method for alarm state logging
bool LoggerManager::logAlarmStateChange(int pointNumber, const String& pointName, 
                                       const String& alarmType, const String& alarmPriority,
                                       const String& previousState, const String& newState,
                                       int16_t currentTemp, int16_t threshold) {
    
    return _instance ? _instance->logAlarmState(pointNumber, pointName, alarmType, alarmPriority,
                                               previousState, newState, currentTemp, threshold) : false;
}

// Instance method for alarm state logging
bool LoggerManager::logAlarmState(int pointNumber, const String& pointName, 
                                 const String& alarmType, const String& alarmPriority,
                                 const String& previousState, const String& newState,
                                 int16_t currentTemp, int16_t threshold) {
    if (!_enabled) return false;
    if (!_alarmStateLoggingEnabled) return false;
    
    // Check if we need a new alarm state log file for today
    String currentDate = _getCurrentDateString();
    if (currentDate != _lastAlarmStateLogDate) {
        _lastAlarmStateLogDate = currentDate;
        _currentAlarmStateLogFile = _generateAlarmStateLogFileName();
    }
    
    // Ensure alarm state log file exists
    if (!_ensureAlarmStateLogExists()) {
        return false;
    }
    
    // Generate timestamp
    String timestamp = _getCurrentDateString() + " " + _getCurrentTimeString();
    
    // Write alarm state row
    return _writeAlarmStateRow(timestamp, pointNumber, pointName, alarmType, alarmPriority,
                              previousState, newState, currentTemp, threshold);
}

// Private methods for alarm state logging
String LoggerManager::_generateAlarmStateLogFileName() {

    String dateStr = _getCurrentDateString();
    String filename;
    
    if (_alarmStateLogDirectory.isEmpty()) {
        filename = "/alarm_states_" + dateStr + ".csv";
    } else {
        String dir = _alarmStateLogDirectory;
        if (!dir.startsWith("/")) {
            dir = "/" + dir;
        }
        filename = dir + "/alarm_states_" + dateStr + ".csv";
    }
    
    return filename;
}

bool LoggerManager::_ensureAlarmStateLogExists() {
    if (!_enabled) return false;
    // Check if alarm state log file exists
    File file = _fs->open(_currentAlarmStateLogFile.c_str(), FILE_READ);
    if (file) {
        file.close();
        return true; // File exists
    }
    
    // File doesn't exist, create it with header
    return _writeAlarmStateHeader();
}

bool LoggerManager::_writeAlarmStateHeader() {
    if (!_enabled) return false;
    File file = _fs->open(_currentAlarmStateLogFile.c_str(), FILE_WRITE);
    if (!file) {
        _lastError = "Failed to open alarm state log file for header writing: " + _currentAlarmStateLogFile;
        return false;
    }
    
    String header = "Timestamp,PointNumber,PointName,AlarmType,AlarmPriority,PreviousState,NewState,CurrentTemperature,Threshold\n";
    size_t written = file.print(header);
    file.close();
    
    if (written != header.length()) {
        _lastError = "Failed to write complete alarm state log header";
        return false;
    }
    
    Serial.printf("Alarm state log header written to %s\n", _currentAlarmStateLogFile.c_str());
    return true;
}

bool LoggerManager::_writeAlarmStateRow(const String& timestamp, int pointNumber, const String& pointName,
                                       const String& alarmType, const String& alarmPriority,
                                       const String& previousState, const String& newState,
                                       int16_t currentTemp, int16_t threshold) {
    if (!_enabled) return false;
    File file = _fs->open(_currentAlarmStateLogFile.c_str(), FILE_APPEND);
    if (!file) {
        _lastError = "Failed to open alarm state log file for writing: " + _currentAlarmStateLogFile;
        return false;
    }
    
    // Build alarm state row with proper CSV escaping
    String alarmStateRow = _escapeCSVField(timestamp) + "," + 
                          String(pointNumber) + "," +
                          _escapeCSVField(pointName) + "," + 
                          _escapeCSVField(alarmType) + "," + 
                          _escapeCSVField(alarmPriority) + "," + 
                          _escapeCSVField(previousState) + "," + 
                          _escapeCSVField(newState) + "," + 
                          String(currentTemp) + "," + 
                          String(threshold) + "\n";
    
    size_t written = file.print(alarmStateRow);
    file.close();
    
    if (written != alarmStateRow.length()) {
        _lastError = "Failed to write complete alarm state log row";
        return false;
    }
    
    // Also print to serial for debugging
    Serial.printf("[ALARM_STATE] %s: Point %d (%s) %s %s: %s -> %s (Temp: %d, Threshold: %d)\n", 
                 timestamp.c_str(), pointNumber, pointName.c_str(), alarmType.c_str(), 
                 alarmPriority.c_str(), previousState.c_str(), newState.c_str(), currentTemp, threshold);
    
    return true;
}

// Alarm state log management
String LoggerManager::getCurrentAlarmStateLogFile() const {
    return _currentAlarmStateLogFile;
}

// std::vector<String> LoggerManager::getAlarmStateLogFiles() {

//     std::vector<String> files;
    
//     String dirPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
//     File dir = _instance->_fs->open(dirPath.c_str());
    
//     if (!dir || !dir.isDirectory()) {
//         return files;
//     }
    
//     File file = dir.openNextFile();
//     while (file) {
//         String filename = String(file.name());
//         if (filename.startsWith("alarm_states_") && filename.endsWith(".csv")) {
//             String fullPath = dirPath;
//             if (!fullPath.endsWith("/")) fullPath += "/";
//             fullPath += filename;
//             files.push_back(fullPath);
//         }
//         file = dir.openNextFile();
//     }
    
//     dir.close();
//     return files;
// }

bool LoggerManager::deleteAlarmStateLogFile(const String& filename) {
    String fullPath = _alarmStateLogDirectory.isEmpty() ? "/" : _alarmStateLogDirectory;
    if (!fullPath.endsWith("/")) fullPath += "/";
    fullPath += filename;
    return _fs->remove(fullPath.c_str());
}


// Add to LoggerManager.cpp:
bool LoggerManager::_isSDCardAvailable() {
    // Try to open the filesystem
    File testFile = _fs->open("/");
    if (!testFile) {
        return false;
    }
    testFile.close();
    return true;
}


String LoggerManager::getAlarmHistoryJson(const String& startDate, const String& endDate) {
    if (!_instance) {
        return "{\"success\":false,\"error\":\"LoggerManager not initialized\"}";
    }
    
    DynamicJsonDocument doc(16384); // Large document for history
    doc["success"] = true;
    JsonArray historyArray = doc.createNestedArray("history");
    
    // Get all alarm log files in the date range
    std::vector<String> files = _getAlarmLogFilesInRange(startDate, endDate);
    
    if (files.empty()) {
        doc["success"] = false;
        doc["error"] = "No alarm log files found in the specified date range";
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Read and parse each file
    // for (const String& filename : files) {
    //     String fullPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
    //     if (!fullPath.endsWith("/")) fullPath += "/";
    //     fullPath += filename;
        
    //     File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
    //     if (!file) {
    //         continue;
    //     }

    for (const String& filename : files) {
        String fullPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        
        Serial.printf("Opening file: %s\n", fullPath.c_str());
        
        File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
        if (!file) {
            Serial.printf("Failed to open file: %s\n", fullPath.c_str());
            continue;
        }
        
        // Skip header line
        if (file.available()) {
            file.readStringUntil('\n');
        }
        
        // Read data lines
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.isEmpty()) continue;
            
            DynamicJsonDocument entryDoc(512);
            if (_parseAlarmStateLogEntry(line, entryDoc)) {
                JsonObject entry = historyArray.createNestedObject();
                entry["timestamp"] = entryDoc["timestamp"];
                entry["pointNumber"] = entryDoc["pointNumber"];
                entry["pointName"] = entryDoc["pointName"];
                entry["alarmType"] = entryDoc["alarmType"];
                entry["alarmPriority"] = entryDoc["alarmPriority"];
                entry["previousState"] = entryDoc["previousState"];
                entry["newState"] = entryDoc["newState"];
                entry["currentTemperature"] = entryDoc["currentTemperature"];
                entry["threshold"] = entryDoc["threshold"];
            }
        }
        
        file.close();
    }
    
    doc["totalEntries"] = historyArray.size();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String LoggerManager::getAlarmHistoryCsv(const String& startDate, const String& endDate) {
    if (!_instance) {
        return "";
    }
    
    String csv = "Timestamp,PointNumber,PointName,AlarmType,AlarmPriority,PreviousState,NewState,CurrentTemperature,Threshold\n";
    
    // Get all alarm log files in the date range
    std::vector<String> files = _getAlarmLogFilesInRange(startDate, endDate);
    
    if (files.empty()) {
        return "";
    }
    
    // Read and merge data from all files
    for (const String& filename : files) {
        String fullPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        
        File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
        if (!file) {
            continue;
        }
        
        // Skip header line
        if (file.available()) {
            file.readStringUntil('\n');
        }
        
        // Copy data lines
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (!line.isEmpty()) {
                csv += line + "\n";
            }
        }
        
        file.close();
    }
    
    return csv;
}

std::vector<String> LoggerManager::_getAlarmLogFilesInRange(const String& startDate, const String& endDate) {
    std::vector<String> matchingFiles;
    
    if (!_instance) {
        return matchingFiles;
    }
    
    String dirPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
    Serial.printf("Searching for alarm log files in directory: %s\n", dirPath.c_str());
    File dir = _instance->_fs->open(dirPath.c_str());
    
    if (!dir || !dir.isDirectory()) {
        Serial.printf("Could not open directory: %s\n", dirPath.c_str());
        return matchingFiles;
    }
    
    String normalizedStart = _normalizeDate(startDate);
    Serial.printf("Normalized start date: %s\n", normalizedStart.c_str());
    String normalizedEnd = _normalizeDate(endDate);
    Serial.printf("Normalized end date: %s\n", normalizedEnd.c_str());
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        Serial.printf("Checking file: %s\n", filename.c_str());
        
        // Check if it's an alarm state log file
        if (filename.startsWith("alarm_states_") && filename.endsWith(".csv")) {
            // Extract date from filename (alarm_states_YYYY-MM-DD.csv)
            // The date starts after "alarm_states_" which is 13 characters
            String fileDate = filename.substring(13); // Remove "alarm_states_" prefix
            fileDate = fileDate.substring(0, fileDate.lastIndexOf('.')); // Remove .csv extension
            
            Serial.printf("Extracted file date: %s\n", fileDate.c_str());
            
            // Check if date is within range
            if (fileDate >= normalizedStart && fileDate <= normalizedEnd) {
                Serial.printf("File date %s is within range [%s, %s]\n", fileDate.c_str(), normalizedStart.c_str(), normalizedEnd.c_str());
                matchingFiles.push_back(filename);
            } else {
                Serial.printf("File date %s is outside range [%s, %s]\n", fileDate.c_str(), normalizedStart.c_str(), normalizedEnd.c_str());
            }
        }
        
        file = dir.openNextFile();
    }
    
    dir.close();
    
    Serial.printf("Found %d matching files\n", matchingFiles.size());
    
    // Sort files by date
    std::sort(matchingFiles.begin(), matchingFiles.end());
    
    return matchingFiles;
}
bool LoggerManager::_parseAlarmStateLogEntry(const String& line, DynamicJsonDocument& entry) {
    // This method doesn't need instance access, so it can remain unchanged
    // Parse CSV line: Timestamp,PointNumber,PointName,AlarmType,AlarmPriority,PreviousState,NewState,CurrentTemperature,Threshold
    int fieldIndex = 0;
    int startPos = 0;
    String fields[9];
    bool inQuotes = false;
    
    for (int i = 0; i <= line.length(); i++) {
        char c = (i < line.length()) ? line.charAt(i) : ',';
        
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            if (fieldIndex < 9) {
                fields[fieldIndex] = line.substring(startPos, i);
                // Remove quotes if present
                if (fields[fieldIndex].startsWith("\"") && fields[fieldIndex].endsWith("\"")) {
                    fields[fieldIndex] = fields[fieldIndex].substring(1, fields[fieldIndex].length() - 1);
                }
                fields[fieldIndex].trim();
            }
            fieldIndex++;
            startPos = i + 1;
        }
    }
    
    if (fieldIndex < 8) { // We need at least 9 fields (0-8)
        return false;
    }
    
    // Parse fields into JSON
    entry["timestamp"] = fields[0];
    entry["pointNumber"] = fields[1].toInt();
    entry["pointName"] = fields[2];
    entry["alarmType"] = fields[3];
    entry["alarmPriority"] = fields[4];
    entry["previousState"] = fields[5];
    entry["newState"] = fields[6];
    entry["currentTemperature"] = fields[7].toInt();
    entry["threshold"] = fields[8].toInt();
    
    return true;
}

String LoggerManager::_normalizeDate(const String& dateStr) {
    // This method doesn't need instance access, so it can remain unchanged
    // Ensure date is in YYYY-MM-DD format
    String normalized = dateStr;
    normalized.trim();
    
    // If date contains slashes, replace with dashes
    normalized.replace("/", "-");
    
    // Ensure proper formatting with leading zeros
    int firstDash = normalized.indexOf('-');
    int secondDash = normalized.lastIndexOf('-');
    
    if (firstDash > 0 && secondDash > firstDash) {
        String year = normalized.substring(0, firstDash);
        String month = normalized.substring(firstDash + 1, secondDash);
        String day = normalized.substring(secondDash + 1);
        
        // Pad with zeros if needed
        if (month.length() == 1) month = "0" + month;
        if (day.length() == 1) day = "0" + day;
        
        normalized = year + "-" + month + "-" + day;
    }
    
    return normalized;
}

// String LoggerManager::getEventLogsJson(const String& startDate, const String& endDate) {
//     if (!_instance) {
//         return "{\"success\":false,\"error\":\"LoggerManager not initialized\"}";
//     }
    
//     DynamicJsonDocument doc(16384); // Large document for logs
//     doc["success"] = true;
//     JsonArray logsArray = doc.createNestedArray("logs");
    
//     // Get all event log files in the date range
//     std::vector<String> files = _getEventLogFilesInRange(startDate, endDate);
    
//     if (files.empty()) {
//         doc["success"] = false;
//         doc["error"] = "No event log files found in the specified date range";
//         String output;
//         serializeJson(doc, output);
//         return output;
//     }
    
//     // Read and parse each file
//     for (const String& filename : files) {
//         String fullPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
//         if (!fullPath.endsWith("/")) fullPath += "/";
//         fullPath += filename;
        
//         Serial.printf("Opening event log file: %s\n", fullPath.c_str());
        
//         File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
//         if (!file) {
//             Serial.printf("Failed to open event log file: %s\n", fullPath.c_str());
//             continue;
//         }
        
//         // Skip header line
//         if (file.available()) {
//             file.readStringUntil('\n');
//         }
        
//         // Read data lines
//         while (file.available()) {
//             String line = file.readStringUntil('\n');
//             line.trim();
//             if (line.isEmpty()) continue;
            
//             DynamicJsonDocument entryDoc(512);
//             if (_parseEventLogEntry(line, entryDoc)) {
//                 JsonObject entry = logsArray.createNestedObject();
//                 entry["timestamp"] = entryDoc["timestamp"];
//                 entry["source"] = entryDoc["source"];
//                 entry["description"] = entryDoc["description"];
//                 entry["priority"] = entryDoc["priority"];
//             }
//         }
        
//         file.close();
//     }
    
//     doc["totalEntries"] = logsArray.size();
    
//     String output;
//     serializeJson(doc, output);
//     return output;
// }

// String LoggerManager::getEventLogsCsv(const String& startDate, const String& endDate) {
//     if (!_instance) {
//         return "";
//     }
    
//     String csv = "Timestamp,Source,Description,Priority\n";
    
//     // Get all event log files in the date range
//     std::vector<String> files = _getEventLogFilesInRange(startDate, endDate);
    
//     if (files.empty()) {
//         return "";
//     }
    
//     // Read and merge data from all files
//     for (const String& filename : files) {
//         String fullPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
//         if (!fullPath.endsWith("/")) fullPath += "/";
//         fullPath += filename;
        
//         File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
//         if (!file) {
//             continue;
//         }
        
//         // Skip header line
//         if (file.available()) {
//             file.readStringUntil('\n');
//         }
        
//         // Copy data lines
//         while (file.available()) {
//             String line = file.readStringUntil('\n');
//             line.trim();
//             if (!line.isEmpty()) {
//                 csv += line + "\n";
//             }
//         }
        
//         file.close();
//     }
    
//     return csv;
// }

// std::vector<String> LoggerManager::_getEventLogFilesInRange(const String& startDate, const String& endDate) {
//     std::vector<String> matchingFiles;
    
//     if (!_instance) {
//         return matchingFiles;
//     }
    
//     String dirPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
//     Serial.printf("Searching for event log files in directory: %s\n", dirPath.c_str());
//     File dir = _instance->_fs->open(dirPath.c_str());
    
//     if (!dir || !dir.isDirectory()) {
//         Serial.printf("Could not open directory: %s\n", dirPath.c_str());
//         return matchingFiles;
//     }
    
//     String normalizedStart = _normalizeDate(startDate);
//     Serial.printf("Normalized start date: %s\n", normalizedStart.c_str());
//     String normalizedEnd = _normalizeDate(endDate);
//     Serial.printf("Normalized end date: %s\n", normalizedEnd.c_str());
    
//     File file = dir.openNextFile();
//     while (file) {
//         String filename = String(file.name());
//         Serial.printf("Checking file: %s\n", filename.c_str());
        
//         // Check if it's an event log file
//         if (filename.startsWith("events_") && filename.endsWith(".csv")) {
//             // Extract date from filename (events_YYYY-MM-DD.csv)
//             String fileDate = filename.substring(7); // Remove "events_" prefix
//             fileDate = fileDate.substring(0, fileDate.lastIndexOf('.')); // Remove .csv extension
            
//             Serial.printf("Extracted file date: %s\n", fileDate.c_str());
            
//             // Check if date is within range
//             if (fileDate >= normalizedStart && fileDate <= normalizedEnd) {
//                 Serial.printf("File date %s is within range [%s, %s]\n", fileDate.c_str(), normalizedStart.c_str(), normalizedEnd.c_str());
//                 matchingFiles.push_back(filename);
//             }
//         }
        
//         file = dir.openNextFile();
//     }
    
//     dir.close();
    
//     Serial.printf("Found %d matching event log files\n", matchingFiles.size());
    
//     // Sort files by date
//     std::sort(matchingFiles.begin(), matchingFiles.end());
    
//     return matchingFiles;
// }

// bool LoggerManager::_parseEventLogEntry(const String& line, DynamicJsonDocument& entry) {
//     // Parse CSV line: Timestamp,Source,Description,Priority
//     int fieldIndex = 0;
//     int startPos = 0;
//     String fields[4];
//     bool inQuotes = false;
    
//     for (int i = 0; i <= line.length(); i++) {
//         char c = (i < line.length()) ? line.charAt(i) : ',';
        
//         if (c == '"') {
//             inQuotes = !inQuotes;
//         } else if (c == ',' && !inQuotes) {
//             if (fieldIndex < 4) {
//                 fields[fieldIndex] = line.substring(startPos, i);
//                 // Remove quotes if present
//                 if (fields[fieldIndex].startsWith("\"") && fields[fieldIndex].endsWith("\"")) {
//                     fields[fieldIndex] = fields[fieldIndex].substring(1, fields[fieldIndex].length() - 1);
//                 }
//                 fields[fieldIndex].trim();
//             }
//             fieldIndex++;
//             startPos = i + 1;
//         }
//     }
    
//     if (fieldIndex < 3) { // We need at least 4 fields (0-3)
//         return false;
//     }
    
//     // Parse fields into JSON
//     entry["timestamp"] = fields[0];
//     entry["source"] = fields[1];
//     entry["description"] = fields[2];
//     entry["priority"] = fields[3];
    
//     return true;
// }


// Add these methods to your LoggerManager.cpp file

// Static method to get event logs as JSON
String LoggerManager::getEventLogsJson(const String& startDate, const String& endDate) {
    if (!_instance) {
        return "{\"success\":false,\"error\":\"LoggerManager not initialized\"}";
    }
    
    DynamicJsonDocument doc(16384); // Large document for logs
    doc["success"] = true;
    JsonArray logsArray = doc.createNestedArray("logs");
    
    // Get all event log files in the date range
    std::vector<String> files = _getEventLogFilesInRange(startDate, endDate);
    
    if (files.empty()) {
        doc["success"] = false;
        doc["error"] = "No event log files found in the specified date range";
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Read and parse each file
    for (const String& filename : files) {
        String fullPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        
        Serial.printf("Opening event log file: %s\n", fullPath.c_str());
        
        File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
        if (!file) {
            Serial.printf("Failed to open event log file: %s\n", fullPath.c_str());
            continue;
        }
        
        // Skip header line
        if (file.available()) {
            file.readStringUntil('\n');
        }
        
        // Read data lines
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.isEmpty()) continue;
            
            DynamicJsonDocument entryDoc(512);
            if (_parseEventLogEntry(line, entryDoc)) {
                JsonObject entry = logsArray.createNestedObject();
                entry["timestamp"] = entryDoc["timestamp"];
                entry["source"] = entryDoc["source"];
                entry["description"] = entryDoc["description"];
                entry["priority"] = entryDoc["priority"];
            }
        }
        
        file.close();
    }
    
    doc["totalEntries"] = logsArray.size();
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Static method to get event logs as CSV
String LoggerManager::getEventLogsCsv(const String& startDate, const String& endDate) {
    if (!_instance) {
        return "";
    }
    
    String csv = "Timestamp,Source,Description,Priority\n";
    
    // Get all event log files in the date range
    std::vector<String> files = _getEventLogFilesInRange(startDate, endDate);
    
    if (files.empty()) {
        return "";
    }
    
    // Read and merge data from all files
    for (const String& filename : files) {
        String fullPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        
        File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
        if (!file) {
            continue;
        }
        
        // Skip header line
        if (file.available()) {
            file.readStringUntil('\n');
        }
        
        // Copy data lines
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (!line.isEmpty()) {
                csv += line + "\n";
            }
        }
        
        file.close();
    }
    
    return csv;
}

// Static method to get event log files in range
std::vector<String> LoggerManager::_getEventLogFilesInRange(const String& startDate, const String& endDate) {
    std::vector<String> matchingFiles;
    
    if (!_instance) {
        return matchingFiles;
    }
    
    String dirPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
    Serial.printf("Searching for event log files in directory: %s\n", dirPath.c_str());
    File dir = _instance->_fs->open(dirPath.c_str());
    
    if (!dir || !dir.isDirectory()) {
        Serial.printf("Could not open directory: %s\n", dirPath.c_str());
        return matchingFiles;
    }
    
    String normalizedStart = _normalizeDate(startDate);
    Serial.printf("Normalized start date: %s\n", normalizedStart.c_str());
    String normalizedEnd = _normalizeDate(endDate);
    Serial.printf("Normalized end date: %s\n", normalizedEnd.c_str());
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        Serial.printf("Checking file: %s\n", filename.c_str());
        
        // Check if it's an event log file
        if (filename.startsWith("events_") && filename.endsWith(".csv")) {
            // Extract date from filename (events_YYYY-MM-DD.csv)
            String fileDate = filename.substring(7); // Remove "events_" prefix
            fileDate = fileDate.substring(0, fileDate.lastIndexOf('.')); // Remove .csv extension
            
            Serial.printf("Extracted file date: %s\n", fileDate.c_str());
            
            // Check if date is within range
            if (fileDate >= normalizedStart && fileDate <= normalizedEnd) {
                Serial.printf("File date %s is within range [%s, %s]\n", fileDate.c_str(), normalizedStart.c_str(), normalizedEnd.c_str());
                matchingFiles.push_back(filename);
            }
        }
        
        file = dir.openNextFile();
    }
    
    dir.close();
    
    Serial.printf("Found %d matching event log files\n", matchingFiles.size());
    
    // Sort files by date
    std::sort(matchingFiles.begin(), matchingFiles.end());
    
    return matchingFiles;
}

// Static method to parse event log entry
bool LoggerManager::_parseEventLogEntry(const String& line, DynamicJsonDocument& entry) {
    // Parse CSV line: Timestamp,Source,Description,Priority
    int fieldIndex = 0;
    int startPos = 0;
    String fields[4];
    bool inQuotes = false;
    
    for (int i = 0; i <= line.length(); i++) {
        char c = (i < line.length()) ? line.charAt(i) : ',';
        
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            if (fieldIndex < 4) {
                fields[fieldIndex] = line.substring(startPos, i);
                // Remove quotes if present
                if (fields[fieldIndex].startsWith("\"") && fields[fieldIndex].endsWith("\"")) {
                    fields[fieldIndex] = fields[fieldIndex].substring(1, fields[fieldIndex].length() - 1);
                }
                fields[fieldIndex].trim();
            }
            fieldIndex++;
            startPos = i + 1;
        }
    }
    
    if (fieldIndex < 3) { // We need at least 4 fields (0-3)
        return false;
    }
    
    // Parse fields into JSON
    entry["timestamp"] = fields[0];
    entry["source"] = fields[1];
    entry["description"] = fields[2];
    entry["priority"] = fields[3];
    
    return true;
}

// Static method to get event log statistics
String LoggerManager::getEventLogStatsJson(const String& startDate, const String& endDate) {
    if (!_instance) {
        return "{\"success\":false,\"error\":\"LoggerManager not initialized\"}";
    }
    
    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    
    // Initialize counters
    int totalEntries = 0;
    int criticalCount = 0;
    int errorCount = 0;
    int warningCount = 0;
    int infoCount = 0;
    
    // Get all event log files in the date range
    std::vector<String> files = _getEventLogFilesInRange(startDate, endDate);
    
    // Count entries by priority
    for (const String& filename : files) {
        String fullPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        
        File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
        if (!file) {
            continue;
        }
        
        // Skip header line
        if (file.available()) {
            file.readStringUntil('\n');
        }
        
        // Read data lines
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.isEmpty()) continue;
            
            totalEntries++;
            
            // Quick parse to get priority (last field)
            int lastComma = line.lastIndexOf(',');
            if (lastComma != -1) {
                String priority = line.substring(lastComma + 1);
                priority.trim();
                
                // Remove quotes if present
                if (priority.startsWith("\"") && priority.endsWith("\"")) {
                    priority = priority.substring(1, priority.length() - 1);
                }
                
                if (priority == "CRITICAL") {
                    criticalCount++;
                } else if (priority == "ERROR") {
                    errorCount++;
                } else if (priority == "WARNING") {
                    warningCount++;
                } else if (priority == "INFO") {
                    infoCount++;
                }
            }
        }
        
        file.close();
    }
    
    // Build statistics JSON
    doc["totalEntries"] = totalEntries;
    doc["dateRange"]["start"] = startDate;
    doc["dateRange"]["end"] = endDate;
    doc["filesFound"] = files.size();
    
    JsonObject priorityStats = doc.createNestedObject("priorityStats");
    priorityStats["critical"] = criticalCount;
    priorityStats["error"] = errorCount;
    priorityStats["warning"] = warningCount;
    priorityStats["info"] = infoCount;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Static method to get event log files
std::vector<String> LoggerManager::getEventLogFilesStatic() {
    std::vector<String> files;
    
    if (!_instance) {
        return files;
    }
    
    String dirPath = _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
    File dir = _instance->_fs->open(dirPath.c_str());
    
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        if (filename.startsWith("events_") && filename.endsWith(".csv")) {
            files.push_back(filename);
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    
    // Sort files by date
    std::sort(files.begin(), files.end());
    
    return files;
}



// Static method to get temperature data log files
std::vector<String> LoggerManager::getLogFiles() {
    std::vector<String> files;
    if (!_instance) {
        return files;
    }
    
    String dirPath = _instance->_logDirectory.isEmpty() ? "/" : _instance->_logDirectory;
    File dir = _instance->_fs->open(dirPath.c_str());
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        if (filename.startsWith("temp_log_") && filename.endsWith(".csv")) {
            files.push_back(filename);
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    std::sort(files.begin(), files.end());
    return files;
}

// Static method to get alarm state log files
std::vector<String> LoggerManager::getAlarmStateLogFiles() {
    std::vector<String> files;
    if (!_instance) {
        return files;
    }
    
    String dirPath = _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
    File dir = _instance->_fs->open(dirPath.c_str());
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        if (filename.startsWith("alarm_states_") && filename.endsWith(".csv")) {
            files.push_back(filename);
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    std::sort(files.begin(), files.end());
    return files;
}

// Static method to get file information
bool LoggerManager::getFileInfo(const String& filename, const String& type, size_t& fileSize, String& date) {
    if (!_instance) {
        return false;
    }
    
    String dirPath = getLogDirectoryPath(type);
    String fullPath = dirPath;
    if (!fullPath.endsWith("/") && !fullPath.isEmpty()) fullPath += "/";
    fullPath += filename;
    
    File file = _instance->_fs->open(fullPath.c_str(), FILE_READ);
    if (!file) {
        return false;
    }
    
    fileSize = file.size();
    file.close();
    
    // Extract date from filename based on type
    if (type == "data" && filename.startsWith("temp_log_") && filename.endsWith(".csv")) {
        int firstUnderscore = filename.indexOf('_', 5); // Skip "temp_"
        int secondUnderscore = filename.indexOf('_', firstUnderscore + 1);
        if (firstUnderscore > 0 && secondUnderscore > firstUnderscore) {
            date = filename.substring(firstUnderscore + 1, secondUnderscore);
        }
    } else if (type == "event" && filename.startsWith("events_") && filename.endsWith(".csv")) {
        date = filename.substring(7, filename.length() - 4);
    } else if (type == "alarm" && filename.startsWith("alarm_states_") && filename.endsWith(".csv")) {
        date = filename.substring(13, filename.length() - 4);
    }
    
    return true;
}

// Static method to open log files
File LoggerManager::openLogFile(const String& filename, const String& type) {
    if (!_instance) {
        return File();
    }
    
    String dirPath = getLogDirectoryPath(type);
    String fullPath = dirPath;
    if (!fullPath.endsWith("/") && !fullPath.isEmpty()) fullPath += "/";
    fullPath += filename;
    
    return _instance->_fs->open(fullPath.c_str(), FILE_READ);
}

// Static method to get directory path for different log types
String LoggerManager::getLogDirectoryPath(const String& type) {
    if (!_instance) {
        return "/";
    }
    
    if (type == "data") {
        return _instance->_logDirectory.isEmpty() ? "/" : _instance->_logDirectory;
    } else if (type == "event") {
        return _instance->_eventLogDirectory.isEmpty() ? "/" : _instance->_eventLogDirectory;
    } else if (type == "alarm") {
        return _instance->_alarmStateLogDirectory.isEmpty() ? "/" : _instance->_alarmStateLogDirectory;
    }
    
    return "/";
}


// std::vector<String> LoggerManager::getLogFiles() {

//     std::vector<String> files;
    
//     File dir = _fs->open(_logDirectory.c_str());
//     if (!dir || !dir.isDirectory()) {
//         return files;
//     }
    
//     File file = dir.openNextFile();
//     while (file) {
//         if (!file.isDirectory() && String(file.name()).endsWith(".csv")) {
//             files.push_back(String(file.name()));
//         }
//         file = dir.openNextFile();
//     }
    
//     dir.close();
//     return files;
// }
