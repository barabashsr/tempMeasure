#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

class TimeManager {
public:
    // Constructor
    TimeManager(int sdaPin = 21, int sclPin = 22);
    
    // Destructor
    ~TimeManager();
    
    // Initialization
    bool begin();
    bool init();
    
    // Time setting methods
    bool setTimeFromNTP(const char* ntpServer = "pool.ntp.org");
    bool setTime(int year, int month, int day, int hour, int minute, int second);
    bool setTime(DateTime dateTime);
    bool setTimeFromUnix(uint32_t unixTime);
    bool setTimeFromCompileTime();
    
    // Time getting methods
    DateTime getCurrentTime();
    String getFormattedTime(const String& format = "YYYY-MM-DD hh:mm:ss");
    String getTimeString();
    String getDateString();
    uint32_t getUnixTime();
    
    // Timezone management
    void setTimezone(int offsetHours, int offsetMinutes = 0);
    void setTimezoneOffset(long offsetSeconds);
    int getTimezoneHours();
    int getTimezoneMinutes();
    long getTimezoneOffset();
    
    // WiFi and NTP configuration
    void setNTPServer(const String& server);
    void setNTPUpdateInterval(unsigned long intervalMs);
    String getNTPServer();
    
    // Update methods (call in loop)
    void update();
    bool syncWithNTP();
    bool isNTPSyncEnabled();
    void enableNTPSync(bool enable);
    
    // Status methods
    bool isRTCConnected();
    bool isTimeSet();
    bool hasLostPower();
    unsigned long getLastNTPSync();
    float getTemperature(); // From DS3231 built-in sensor
    
    // Alarm functionality
    bool setAlarm1(DateTime alarmTime, Ds3231Alarm1Mode mode = DS3231_A1_Hour);
    bool setAlarm2(DateTime alarmTime, Ds3231Alarm2Mode mode = DS3231_A2_Hour);
    bool clearAlarm1();
    bool clearAlarm2();
    bool isAlarm1Triggered();
    bool isAlarm2Triggered();
    
    // Square wave output
    void enableSquareWave(Ds3231SqwPinMode mode = DS3231_SquareWave1Hz);
    void disableSquareWave();
    
    // JSON output for web interface
    String getTimeJSON();
    String getStatusJSON();
    
    // Configuration save/load
    void saveConfig();
    void loadConfig();

private:
    // Hardware
    RTC_DS3231 _rtc;
    WiFiUDP _ntpUDP;
    NTPClient* _timeClient;
    
    // Configuration
    int _sdaPin;
    int _sclPin;
    long _timezoneOffset;  // Offset in seconds
    String _ntpServer;
    unsigned long _ntpUpdateInterval;
    bool _ntpSyncEnabled;
    
    // Status tracking
    bool _rtcConnected;
    bool _timeSet;
    unsigned long _lastNTPSync;
    unsigned long _lastNTPAttempt;
    
    // Internal methods
    void _initializeNTP();
    bool _isWiFiConnected();
    void _updateFromNTP();
    DateTime _applyTimezone(DateTime utcTime);
    DateTime _removeTimezone(DateTime localTime);
    String _formatDateTime(DateTime dt, const String& format);
};
