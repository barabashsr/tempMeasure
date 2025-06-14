#include "LoggerManager.h"

LoggerManager::LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem)
    : _controller(&controller), _timeManager(&timeManager), _fs(&filesystem),
      _logFrequency(60000), _lastLogTime(0), _headerWritten(false),
      _enabled(true), _logDirectory(""), _dailyFiles(true), _lastError(""),
      _lastGeneratedHeader(""), _headerChanged(false), _fileSequenceNumber(0) {
}

LoggerManager::~LoggerManager() {
    closeCurrentFile();
}

bool LoggerManager::begin() {
    if (!_ensureDirectoryExists()) {
        _lastError = "Failed to create log directory";
        return false;
    }
    
    // Recover from existing files after reboot
    if (!_recoverFromExistingFiles()) {
        Serial.println("Warning: Could not recover from existing files, starting fresh");
    }
    
    // Generate current header for comparison
    _lastGeneratedHeader = _generateCSVHeader();
    
    // Generate log file name with recovered sequence number
    _currentLogFile = _generateLogFileNameWithSequence();
    _lastLogDate = _getCurrentDateString();
    
    Serial.printf("LoggerManager initialized. Log file: %s\n", _currentLogFile.c_str());
    Serial.printf("File sequence number: %d\n", _fileSequenceNumber);
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
                // New day - recover from existing files for new date
                _recoverFromExistingFiles();
                _lastLogDate = currentDate;
            }
        }
        
        // Check if header has changed (point names changed)
        if (_hasHeaderChanged()) {
            Serial.println("Header changed - creating new log file");
            _incrementSequenceNumber();
            createNewLogFile();
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
    
    if (_logDirectory.isEmpty()) {
        filename = "/temp_log_" + dateStr + "_" + String(_fileSequenceNumber) + ".csv";
    } else {
        filename = _logDirectory + "/temp_log_" + dateStr + "_" + String(_fileSequenceNumber) + ".csv";
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
