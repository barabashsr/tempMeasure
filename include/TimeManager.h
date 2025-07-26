/**
 * @file TimeManager.h
 * @brief Real-time clock and time synchronization management
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Manages system time using DS3231 RTC with NTP synchronization support.
 *          Provides timezone handling, alarms, and time formatting capabilities.
 * 
 * @section dependencies Dependencies
 * - RTClib for DS3231 RTC interface
 * - NTPClient for network time synchronization
 * - WiFi libraries for NTP connectivity
 * - ArduinoJson for JSON output
 * 
 * @section hardware Hardware Requirements
 * - DS3231 RTC module on I2C bus
 * - WiFi connectivity for NTP synchronization
 * - Default I2C pins: SDA=21, SCL=22
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

/**
 * @class TimeManager
 * @brief Manages system time with RTC and NTP synchronization
 * @details Provides comprehensive time management including RTC control,
 *          NTP synchronization, timezone handling, alarms, and formatting.
 */
class TimeManager {
public:
    /**
     * @brief Construct a new Time Manager object
     * @param[in] sdaPin I2C SDA pin (default: 21)
     * @param[in] sclPin I2C SCL pin (default: 22)
     */
    TimeManager(int sdaPin = 21, int sclPin = 22);
    
    /**
     * @brief Destroy the Time Manager object
     * @details Cleans up NTP client resources
     */
    ~TimeManager();
    
    // Initialization
    /**
     * @brief Initialize time manager and RTC
     * @return true if initialization successful
     * @return false if RTC not found or initialization failed
     */
    bool begin();
    
    /**
     * @brief Alternative initialization method
     * @return true if initialization successful
     * @return false if initialization failed
     * @see begin()
     */
    bool init();
    
    // Time setting methods
    /**
     * @brief Set time from NTP server
     * @param[in] ntpServer NTP server address (default: "pool.ntp.org")
     * @return true if time set successfully
     * @return false if NTP sync failed
     */
    bool setTimeFromNTP(const char* ntpServer = "pool.ntp.org");
    
    /**
     * @brief Sync ESP32 system time from RTC
     * @return true if sync successful
     * @return false if RTC not connected
     */
    bool syncSystemTimeFromRTC();
    
    /**
     * @brief Set time manually
     * @param[in] year Year (2000-2099)
     * @param[in] month Month (1-12)
     * @param[in] day Day (1-31)
     * @param[in] hour Hour (0-23)
     * @param[in] minute Minute (0-59)
     * @param[in] second Second (0-59)
     * @return true if time set successfully
     * @return false if invalid parameters
     */
    bool setTime(int year, int month, int day, int hour, int minute, int second);
    
    /**
     * @brief Set time from DateTime object
     * @param[in] dateTime DateTime object with time to set
     * @return true if time set successfully
     * @return false if RTC communication failed
     */
    bool setTime(DateTime dateTime);
    
    /**
     * @brief Set time from Unix timestamp
     * @param[in] unixTime Unix timestamp (seconds since 1970)
     * @return true if time set successfully
     * @return false if RTC communication failed
     */
    bool setTimeFromUnix(uint32_t unixTime);
    
    /**
     * @brief Set time from compile time
     * @return true if time set successfully
     * @return false if RTC communication failed
     * @note Uses __DATE__ and __TIME__ macros
     */
    bool setTimeFromCompileTime();
    
    // Time getting methods
    /**
     * @brief Get current time from RTC
     * @return DateTime Current date and time
     */
    DateTime getCurrentTime();
    
    /**
     * @brief Get formatted time string
     * @param[in] format Format string (default: "YYYY-MM-DD hh:mm:ss")
     * @return String Formatted time string
     * @note Format: YYYY=year, MM=month, DD=day, hh=hour, mm=minute, ss=second
     */
    String getFormattedTime(const String& format = "YYYY-MM-DD hh:mm:ss");
    
    /**
     * @brief Get time as string (HH:MM:SS)
     * @return String Time in HH:MM:SS format
     */
    String getTimeString();
    
    /**
     * @brief Get date as string (YYYY-MM-DD)
     * @return String Date in YYYY-MM-DD format
     */
    String getDateString();
    
    /**
     * @brief Get Unix timestamp
     * @return uint32_t Seconds since 1970-01-01 00:00:00
     */
    uint32_t getUnixTime();
    
    // Timezone management
    /**
     * @brief Set timezone offset
     * @param[in] offsetHours Hours offset from UTC (-12 to +14)
     * @param[in] offsetMinutes Minutes offset (default: 0)
     */
    void setTimezone(int offsetHours, int offsetMinutes = 0);
    
    /**
     * @brief Set timezone offset in seconds
     * @param[in] offsetSeconds Seconds offset from UTC
     */
    void setTimezoneOffset(long offsetSeconds);
    
    /**
     * @brief Get timezone hours offset
     * @return int Hours offset from UTC
     */
    int getTimezoneHours();
    
    /**
     * @brief Get timezone minutes offset
     * @return int Minutes offset within hour
     */
    int getTimezoneMinutes();
    
    /**
     * @brief Get total timezone offset in seconds
     * @return long Total seconds offset from UTC
     */
    long getTimezoneOffset();
    
    // WiFi and NTP configuration
    /**
     * @brief Set NTP server address
     * @param[in] server NTP server hostname or IP
     */
    void setNTPServer(const String& server);
    
    /**
     * @brief Set NTP update interval
     * @param[in] intervalMs Update interval in milliseconds
     */
    void setNTPUpdateInterval(unsigned long intervalMs);
    
    /**
     * @brief Get current NTP server
     * @return String NTP server address
     */
    String getNTPServer();
    
    // Update methods (call in loop)
    /**
     * @brief Update time from NTP if needed
     * @details Call this in loop() for automatic NTP synchronization
     */
    void update();
    
    /**
     * @brief Force immediate NTP synchronization
     * @return true if sync successful
     * @return false if sync failed
     */
    bool syncWithNTP();
    
    /**
     * @brief Check if NTP sync is enabled
     * @return true if automatic NTP sync enabled
     * @return false if NTP sync disabled
     */
    bool isNTPSyncEnabled();
    
    /**
     * @brief Enable/disable automatic NTP sync
     * @param[in] enable True to enable, false to disable
     */
    void enableNTPSync(bool enable);
    
    // Status methods
    /**
     * @brief Check if RTC is connected
     * @return true if RTC communication successful
     * @return false if RTC not responding
     */
    bool isRTCConnected();
    
    /**
     * @brief Check if time has been set
     * @return true if RTC time is valid
     * @return false if time not set
     */
    bool isTimeSet();
    
    /**
     * @brief Check if RTC lost power
     * @return true if RTC lost power and time invalid
     * @return false if RTC maintained time
     */
    bool hasLostPower();
    
    /**
     * @brief Get timestamp of last NTP sync
     * @return unsigned long Milliseconds since last sync
     */
    unsigned long getLastNTPSync();
    
    /**
     * @brief Get temperature from DS3231
     * @return float Temperature in Celsius
     * @note DS3231 has built-in temperature sensor
     */
    float getTemperature();
    
    // Alarm functionality
    /**
     * @brief Set Alarm 1 on DS3231
     * @param[in] alarmTime DateTime when alarm should trigger
     * @param[in] mode Alarm mode (default: match hours)
     * @return true if alarm set successfully
     * @return false if RTC communication failed
     * @see Ds3231Alarm1Mode
     */
    bool setAlarm1(DateTime alarmTime, Ds3231Alarm1Mode mode = DS3231_A1_Hour);
    
    /**
     * @brief Set Alarm 2 on DS3231
     * @param[in] alarmTime DateTime when alarm should trigger
     * @param[in] mode Alarm mode (default: match hours)
     * @return true if alarm set successfully
     * @return false if RTC communication failed
     * @see Ds3231Alarm2Mode
     */
    bool setAlarm2(DateTime alarmTime, Ds3231Alarm2Mode mode = DS3231_A2_Hour);
    
    /**
     * @brief Clear Alarm 1 flag
     * @return true if alarm cleared successfully
     * @return false if RTC communication failed
     */
    bool clearAlarm1();
    
    /**
     * @brief Clear Alarm 2 flag
     * @return true if alarm cleared successfully
     * @return false if RTC communication failed
     */
    bool clearAlarm2();
    
    /**
     * @brief Check if Alarm 1 has triggered
     * @return true if alarm triggered
     * @return false if alarm not triggered
     */
    bool isAlarm1Triggered();
    
    /**
     * @brief Check if Alarm 2 has triggered
     * @return true if alarm triggered
     * @return false if alarm not triggered
     */
    bool isAlarm2Triggered();
    
    // Square wave output
    /**
     * @brief Enable square wave output on SQW pin
     * @param[in] mode Square wave frequency (default: 1Hz)
     * @see Ds3231SqwPinMode
     */
    void enableSquareWave(Ds3231SqwPinMode mode = DS3231_SquareWave1Hz);
    
    /**
     * @brief Disable square wave output
     */
    void disableSquareWave();
    
    // JSON output for web interface
    /**
     * @brief Get current time as JSON string
     * @return String JSON with time, date, timezone info
     * @note Format: {"time":"HH:MM:SS","date":"YYYY-MM-DD","unix":timestamp,"timezone":offset}
     */
    String getTimeJSON();
    
    /**
     * @brief Get time manager status as JSON
     * @return String JSON with RTC status, NTP sync info, temperature
     * @note Includes RTC connection, time set status, last sync time
     */
    String getStatusJSON();
    
    // Configuration save/load
    /**
     * @brief Save time configuration to persistent storage
     * @details Saves timezone, NTP server, and sync settings
     */
    void saveConfig();
    
    /**
     * @brief Load time configuration from persistent storage
     * @details Restores timezone, NTP server, and sync settings
     */
    void loadConfig();

private:
    // Hardware
    RTC_DS3231 _rtc;                    ///< DS3231 RTC instance
    WiFiUDP _ntpUDP;                    ///< UDP client for NTP communication
    NTPClient* _timeClient;             ///< NTP client instance
    
    // Configuration
    int _sdaPin;                        ///< I2C SDA pin number
    int _sclPin;                        ///< I2C SCL pin number
    long _timezoneOffset;               ///< Timezone offset in seconds from UTC
    String _ntpServer;                  ///< NTP server hostname
    unsigned long _ntpUpdateInterval;   ///< NTP sync interval in milliseconds
    bool _ntpSyncEnabled;               ///< Flag for automatic NTP synchronization
    
    // Status tracking
    bool _rtcConnected;                 ///< RTC connection status
    bool _timeSet;                      ///< Time validity flag
    unsigned long _lastNTPSync;         ///< Timestamp of last successful NTP sync
    unsigned long _lastNTPAttempt;      ///< Timestamp of last NTP sync attempt
    
    // Internal methods
    /**
     * @brief Initialize NTP client with current settings
     */
    void _initializeNTP();
    
    /**
     * @brief Check if WiFi is connected
     * @return true if WiFi connected
     * @return false if WiFi disconnected
     */
    bool _isWiFiConnected();
    
    /**
     * @brief Update RTC from NTP server
     * @details Internal method called by update()
     */
    void _updateFromNTP();
    
    /**
     * @brief Apply timezone offset to UTC time
     * @param[in] utcTime UTC DateTime
     * @return DateTime Local time with timezone applied
     */
    DateTime _applyTimezone(DateTime utcTime);
    
    /**
     * @brief Remove timezone offset from local time
     * @param[in] localTime Local DateTime
     * @return DateTime UTC time
     */
    DateTime _removeTimezone(DateTime localTime);
    
    /**
     * @brief Format DateTime according to format string
     * @param[in] dt DateTime to format
     * @param[in] format Format string with placeholders
     * @return String Formatted date/time string
     */
    String _formatDateTime(DateTime dt, const String& format);
};
