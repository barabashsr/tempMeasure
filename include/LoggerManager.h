/**
 * @file LoggerManager.h
 * @brief Comprehensive data and event logging system for temperature monitoring
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file provides a complete logging solution including measurement data logging,
 *          event logging, and alarm state logging with CSV format and file management.
 * 
 * @section dependencies Dependencies
 * - FS.h and SD.h for file system operations
 * - TimeManager.h for timestamp management
 * - ArduinoJson for JSON data handling
 * 
 * @section hardware Hardware Requirements
 * - ESP32 with SD card or LittleFS support
 * - RTC module for accurate timestamps
 */

#ifndef LOGGERMANAGER_H
#define LOGGERMANAGER_H

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "TimeManager.h"
#include <vector>

// Forward declarations
class TemperatureController;  ///< Forward declaration to avoid circular includes
class MeasurementPoint;        ///< Forward declaration for measurement point class

/**
 * @brief Singleton logger manager for comprehensive system logging
 * @details Provides measurement data logging, event logging, and alarm state logging
 *          with automatic file management, CSV formatting, and date-based organization
 */
class LoggerManager {
private:
    static LoggerManager* _instance;     ///< Singleton instance pointer
    TemperatureController* _controller;  ///< Reference to temperature controller
    TimeManager* _timeManager;           ///< Reference to time manager
    fs::FS* _fs;                        ///< File system interface (SD or LittleFS)
    
    unsigned long _logFrequency;     ///< Logging frequency in milliseconds
    unsigned long _lastLogTime;      ///< Last time data was logged
    String _currentLogFile;          ///< Current log file name
    bool _headerWritten;             ///< Flag to track if header is written
    
    // Configuration
    bool _enabled;                   ///< Whether logging is enabled
    String _logDirectory;            ///< Base directory for log files
    bool _dailyFiles;                ///< Create new file each day
    String _lastLogDate;             ///< Track date for daily file creation
    
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
    /**
     * @brief Constructor for LoggerManager
     * @param[in] controller Reference to temperature controller
     * @param[in] timeManager Reference to time manager for timestamps
     * @param[in] filesystem Reference to file system (SD or LittleFS)
     * @details Initializes logging system with required dependencies
     */
    LoggerManager(TemperatureController& controller, TimeManager& timeManager, fs::FS& filesystem);
    
    /**
     * @brief Destructor for LoggerManager
     * @details Closes any open files and cleans up resources
     */
    ~LoggerManager();

    /**
     * @brief Get singleton instance
     * @return LoggerManager* Pointer to singleton instance
     * @details Provides access to the global logger instance
     */
    static LoggerManager* getInstance() { return _instance; }
    
    // Static convenience methods for global logging
    /**
     * @brief Log informational message (static convenience method)
     * @param[in] source Source component or module
     * @param[in] description Description of the event
     * @return bool True if logging successful
     */
    static bool info(const String& source, const String& description) {
        return _instance ? _instance->logInfo(source, description) : false;
    }
    
    /**
     * @brief Log warning message (static convenience method)
     * @param[in] source Source component or module
     * @param[in] description Description of the warning
     * @return bool True if logging successful
     */
    static bool warning(const String& source, const String& description) {
        return _instance ? _instance->logWarning(source, description) : false;
    }
    
    /**
     * @brief Log error message (static convenience method)
     * @param[in] source Source component or module
     * @param[in] description Description of the error
     * @return bool True if logging successful
     */
    static bool error(const String& source, const String& description) {
        return _instance ? _instance->logError(source, description) : false;
    }
    
    /**
     * @brief Log critical message (static convenience method)
     * @param[in] source Source component or module
     * @param[in] description Description of the critical event
     * @return bool True if logging successful
     */
    static bool critical(const String& source, const String& description) {
        return _instance ? _instance->logCritical(source, description) : false;
    }
    
    // Initialization
    /**
     * @brief Initialize the logging system
     * @return bool True if initialization successful
     * @details Sets up file system, directories, and initial configuration
     */
    bool init();
    
    /**
     * @brief Begin logging operations
     * @return bool True if startup successful
     * @details Starts the logging system and opens initial log files
     */
    bool begin();
    
    // Configuration methods
    /**
     * @brief Set logging frequency
     * @param[in] frequencyMs Frequency in milliseconds between log entries
     */
    void setLogFrequency(unsigned long frequencyMs);
    
    /**
     * @brief Get current logging frequency
     * @return unsigned long Frequency in milliseconds
     */
    unsigned long getLogFrequency() const;
    
    /**
     * @brief Enable or disable logging
     * @param[in] enabled True to enable logging
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if logging is enabled
     * @return bool True if logging is enabled
     */
    bool isEnabled() const;
    
    /**
     * @brief Enable or disable daily file creation
     * @param[in] enabled True to create new files daily
     */
    void setDailyFiles(bool enabled);
    
    /**
     * @brief Check if daily files are enabled
     * @return bool True if daily files are enabled
     */
    bool isDailyFiles() const;
    
    /**
     * @brief Set base log directory
     * @param[in] directory Directory path for log files
     */
    void setLogDirectory(const String& directory);
    
    /**
     * @brief Get current log directory
     * @return String Current log directory path
     */
    static String getLogDirectory();
    
    // Logging methods
    void update();                  // Call this in main loop
    bool logDataNow();             // Force immediate logging
    bool createNewLogFile();       // Create new log file
    
    // File management
    String getCurrentLogFile() const;
    bool closeCurrentFile();
    static std::vector<String> getLogFiles();
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
    //static std::vector<String> getAlarmStateLogFiles();
    bool deleteAlarmStateLogFile(const String& filename);

        // Static alarm history retrieval methods
    // static String getAlarmHistoryJson(const String& startDate, const String& endDate);
    // static String getAlarmHistoryCsv(const String& startDate, const String& endDate);
    
    // Event log retrieval methods
    // static String getEventLogsJson(const String& startDate, const String& endDate);
    // static String getEventLogsCsv(const String& startDate, const String& endDate);

    // Static event log retrieval methods
    static String getEventLogsJson(const String& startDate, const String& endDate);
    static String getEventLogsCsv(const String& startDate, const String& endDate);
    static String getEventLogStatsJson(const String& startDate, const String& endDate);
    //static std::vector<String> getEventLogFilesStatic();
    
    // Static alarm history retrieval methods (already existing)
    static String getAlarmHistoryJson(const String& startDate, const String& endDate);
    static String getAlarmHistoryCsv(const String& startDate, const String& endDate);

    // Static methods for file operations
    //static std::vector<String> getLogFiles();
    static std::vector<String> getEventLogFilesStatic();
    static std::vector<String> getAlarmStateLogFiles();
    
    // Static methods for file information
    static bool getFileInfo(const String& filename, const String& type, size_t& fileSize, String& date);
    
    // Static methods for file streaming
    static File openLogFile(const String& filename, const String& type);
    static String getLogDirectoryPath(const String& type);
    
    
private:
    String _lastError;
    // Make helper methods static too
    // static std::vector<String> _getAlarmLogFilesInRange(const String& startDate, const String& endDate);
    // static bool _parseAlarmStateLogEntry(const String& line, DynamicJsonDocument& entry);
    // static String _normalizeDate(const String& dateStr);

    // Helper methods for event logs
    // static std::vector<String> _getEventLogFilesInRange(const String& startDate, const String& endDate);
    // static bool _parseEventLogEntry(const String& line, DynamicJsonDocument& entry);

    // Static helper methods for event logs
    static std::vector<String> _getEventLogFilesInRange(const String& startDate, const String& endDate);
    static bool _parseEventLogEntry(const String& line, DynamicJsonDocument& entry);
    
    // Make existing helper methods static too (if not already)
    static std::vector<String> _getAlarmLogFilesInRange(const String& startDate, const String& endDate);
    static bool _parseAlarmStateLogEntry(const String& line, DynamicJsonDocument& entry);
    static String _normalizeDate(const String& dateStr);
};

#endif // LOGGERMANAGER_H
