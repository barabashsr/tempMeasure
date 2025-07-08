#ifndef LOGGERMANAGER_H
#define LOGGERMANAGER_H

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
//#include "TemperatureController.h"
#include "TimeManager.h"
#include <vector>
#include "LoggerManager.h"

// Add forward declaration instead
class TemperatureController;  // Forward declaration
class MeasurementPoint;

class LoggerManager {
private:
    static LoggerManager* _instance;
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
    

    bool _recoverFromExistingFiles();
    String _findLatestFileForDate(const String& dateStr);
    int _extractSequenceNumber(const String& filename);
    String _readHeaderFromFile(const String& filename);
    std::vector<String> _getFilesForDate(const String& dateStr);
    int _findHighestSequenceForDate(const String& dateStr);

    // Header change detection
    String _lastGeneratedHeader;
    bool _headerChanged;
    int _fileSequenceNumber;
    
    // New private methods
    bool _hasHeaderChanged();
    String _generateLogFileNameWithSequence();
    void _incrementSequenceNumber();

    // Event logging
    bool _eventLoggingEnabled;
    String _eventLogDirectory;
    String _currentEventLogFile;
    String _lastEventLogDate;
    
    // Event logging methods
    String _generateEventLogFileName();
    bool _writeEventHeader();
    bool _writeEventRow(const String& timestamp, const String& source, 
                       const String& description, const String& priority);
    bool _ensureEventLogExists();

    // Alarm state logging
    bool _alarmStateLoggingEnabled;
    String _alarmStateLogDirectory;
    String _currentAlarmStateLogFile;
    String _lastAlarmStateLogDate;
    
    // Alarm state logging methods
    String _generateAlarmStateLogFileName();
    bool _writeAlarmStateHeader();
    bool _ensureAlarmStateLogExists();
    bool _writeAlarmStateRow(const String& timestamp, int pointNumber, const String& pointName,
        const String& alarmType, const String& alarmPriority,
        const String& previousState, const String& newState,
        int16_t currentTemp, int16_t threshold);

    
    bool _isSDCardAvailable();

    
public:
    LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem);
    ~LoggerManager();

    static LoggerManager* getInstance() { return _instance; }
    
    // Add these static convenience methods
    static bool info(const String& source, const String& description) {
        return _instance ? _instance->logInfo(source, description) : false;
    }
    
    static bool warning(const String& source, const String& description) {
        return _instance ? _instance->logWarning(source, description) : false;
    }
    
    static bool error(const String& source, const String& description) {
        return _instance ? _instance->logError(source, description) : false;
    }
    
    static bool critical(const String& source, const String& description) {
        return _instance ? _instance->logCritical(source, description) : false;
    }
    
    // Initialization
    bool init(); 
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

    void forceNewFile();
    int getCurrentSequenceNumber() const;
    void resetSequenceNumber();

        // Event logging configuration
        void setEventLoggingEnabled(bool enabled);
        bool isEventLoggingEnabled() const;
        void setEventLogDirectory(const String& directory);
        String getEventLogDirectory() const;
        
        // Event logging methods
        bool logEvent(const String& source, const String& description, const String& priority = "INFO");
        bool logInfo(const String& source, const String& description);
        bool logWarning(const String& source, const String& description);
        bool logError(const String& source, const String& description);
        bool logCritical(const String& source, const String& description);
        
        // Event log management
        String getCurrentEventLogFile() const;
        std::vector<String> getEventLogFiles();
        bool deleteEventLogFile(const String& filename);


    // Alarm state logging configuration
    void setAlarmStateLoggingEnabled(bool enabled);
    bool isAlarmStateLoggingEnabled() const;
    void setAlarmStateLogDirectory(const String& directory);
    String getAlarmStateLogDirectory() const;
    
    // Static method for alarm state logging
    static bool logAlarmStateChange(int pointNumber, const String& pointName, 
                                   const String& alarmType, const String& alarmPriority,
                                   const String& previousState, const String& newState,
                                   int16_t currentTemp, int16_t threshold);
    
    // Instance method for alarm state logging
    bool logAlarmState(int pointNumber, const String& pointName, 
                      const String& alarmType, const String& alarmPriority,
                      const String& previousState, const String& newState,
                      int16_t currentTemp, int16_t threshold);
    
    // Alarm state log management
    String getCurrentAlarmStateLogFile() const;
    static std::vector<String> getAlarmStateLogFiles();
    bool deleteAlarmStateLogFile(const String& filename);

        // Static alarm history retrieval methods
    static String getAlarmHistoryJson(const String& startDate, const String& endDate);
    static String getAlarmHistoryCsv(const String& startDate, const String& endDate);
    
private:
    String _lastError;
    // Make helper methods static too
    static std::vector<String> _getAlarmLogFilesInRange(const String& startDate, const String& endDate);
    static bool _parseAlarmStateLogEntry(const String& line, DynamicJsonDocument& entry);
    static String _normalizeDate(const String& dateStr);
};

#endif // LOGGERMANAGER_H
