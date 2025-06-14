#ifndef LOGGERMANAGER_H
#define LOGGERMANAGER_H

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "TemperatureController.h"
#include "TimeManager.h"

class LoggerManager {
private:
    TemperatureController* _controller;
    TimeManager* _timeManager;
    fs::FS* _fs;
    
    unsigned long _logFrequency;     // Logging frequency in milliseconds
    unsigned long _lastLogTime;     // Last time data was logged
    String _currentLogFile;         // Current log file name
    bool _headerWritten;            // Flag to track if header is written
    
    // Configuration
    bool _enabled;
    String _logDirectory;
    bool _dailyFiles;               // Create new file each day
    String _lastLogDate;            // Track date for daily file creation
    
    // Private methods
    String _generateLogFileName();
    String _generateCSVHeader();
    bool _writeHeader();
    bool _writeDataRow();
    String _escapeCSVField(const String& field);
    bool _ensureDirectoryExists();
    String _getCurrentDateString();
    String _getCurrentTimeString();
    
public:
    LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem);
    ~LoggerManager();
    
    // Initialization
    bool begin();
    
    // Configuration methods
    void setLogFrequency(unsigned long frequencyMs);
    unsigned long getLogFrequency() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setDailyFiles(bool enabled);
    bool isDailyFiles() const;
    void setLogDirectory(const String& directory);
    String getLogDirectory() const;
    
    // Logging methods
    void update();                  // Call this in main loop
    bool logDataNow();             // Force immediate logging
    bool createNewLogFile();       // Create new log file
    
    // File management
    String getCurrentLogFile() const;
    bool closeCurrentFile();
    std::vector<String> getLogFiles();
    bool deleteLogFile(const String& filename);
    
    // Statistics
    unsigned long getLastLogTime() const;
    size_t getLogFileSize() const;
    
    // Error handling
    String getLastError() const;
    
private:
    String _lastError;
};

#endif // LOGGERMANAGER_H
