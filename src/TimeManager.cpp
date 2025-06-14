#include "TimeManager.h"
#include <LittleFS.h>

TimeManager::TimeManager(int sdaPin, int sclPin) 
    : _sdaPin(sdaPin), _sclPin(sclPin), _timezoneOffset(0), 
      _ntpServer("pool.ntp.org"), _ntpUpdateInterval(3600000), // 1 hour
      _ntpSyncEnabled(true), _rtcConnected(false), _timeSet(false),
      _lastNTPSync(0), _lastNTPAttempt(0), _timeClient(nullptr) {
}

TimeManager::~TimeManager() {
    if (_timeClient) {
        delete _timeClient;
    }
}

bool TimeManager::begin() {
    // Initialize I2C
    Wire.begin(_sdaPin, _sclPin);
    
    // Initialize RTC
    if (!_rtc.begin()) {
        Serial.println("TimeManager: Couldn't find RTC");
        _rtcConnected = false;
        return false;
    }
    
    _rtcConnected = true;
    
    // Check if RTC lost power and set compile time if needed
    if (_rtc.lostPower()) {
        Serial.println("TimeManager: RTC lost power, setting compile time");
        setTimeFromCompileTime();
    } else {
        _timeSet = true;
    }
    
    // Initialize NTP
    _initializeNTP();
    
    // Load saved configuration
    loadConfig();
    
    Serial.println("TimeManager: Initialized successfully");
    return true;
}

void TimeManager::_initializeNTP() {
    if (_timeClient) {
        delete _timeClient;
    }
    
    _timeClient = new NTPClient(_ntpUDP, _ntpServer.c_str(), _timezoneOffset, _ntpUpdateInterval);
    _timeClient->begin();
}

bool TimeManager::setTimeFromNTP(const char* ntpServer) {
    if (ntpServer) {
        _ntpServer = String(ntpServer);
        _initializeNTP();
    }
    
    if (!_isWiFiConnected()) {
        Serial.println("TimeManager: WiFi not connected for NTP sync");
        return false;
    }
    
    if (_timeClient->update()) {
        unsigned long epochTime = _timeClient->getEpochTime();
        DateTime ntpTime = DateTime(epochTime);
        
        if (_rtcConnected) {
            _rtc.adjust(ntpTime);
        }
        
        _timeSet = true;
        _lastNTPSync = millis();
        
        Serial.printf("TimeManager: Time synchronized with NTP: %s\n", 
                     getFormattedTime().c_str());
        return true;
    }
    
    Serial.println("TimeManager: Failed to get time from NTP");
    return false;
}

bool TimeManager::setTime(int year, int month, int day, int hour, int minute, int second) {
    DateTime newTime(year, month, day, hour, minute, second);
    return setTime(newTime);
}

bool TimeManager::setTime(DateTime dateTime) {
    if (_rtcConnected) {
        _rtc.adjust(dateTime);
        _timeSet = true;
        Serial.printf("TimeManager: Time set to: %s\n", 
                     _formatDateTime(dateTime, "YYYY-MM-DD hh:mm:ss").c_str());
        return true;
    }
    return false;
}

bool TimeManager::setTimeFromUnix(uint32_t unixTime) {
    DateTime newTime(unixTime);
    return setTime(newTime);
}

bool TimeManager::setTimeFromCompileTime() {
    if (_rtcConnected) {
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        _timeSet = true;
        Serial.println("TimeManager: Time set to compile time");
        return true;
    }
    return false;
}

DateTime TimeManager::getCurrentTime() {
    if (_rtcConnected) {
        DateTime utcTime = _rtc.now();
        return _applyTimezone(utcTime);
    }
    return DateTime((uint32_t)0); // Explicitly cast to uint32_t
}


String TimeManager::getFormattedTime(const String& format) {
    DateTime current = getCurrentTime();
    return _formatDateTime(current, format);
}

String TimeManager::getTimeString() {
    DateTime current = getCurrentTime();
    return _formatDateTime(current, "hh:mm:ss");
}

String TimeManager::getDateString() {
    DateTime current = getCurrentTime();
    return _formatDateTime(current, "YYYY-MM-DD");
}

uint32_t TimeManager::getUnixTime() {
    if (_rtcConnected) {
        return _rtc.now().unixtime();
    }
    return 0;
}

void TimeManager::setTimezone(int offsetHours, int offsetMinutes) {
    _timezoneOffset = (offsetHours * 3600) + (offsetMinutes * 60);
    if (_timeClient) {
        _timeClient->setTimeOffset(_timezoneOffset);
    }
    Serial.printf("TimeManager: Timezone set to GMT%+d:%02d\n", offsetHours, abs(offsetMinutes));
}

void TimeManager::setTimezoneOffset(long offsetSeconds) {
    _timezoneOffset = offsetSeconds;
    if (_timeClient) {
        _timeClient->setTimeOffset(_timezoneOffset);
    }
}

int TimeManager::getTimezoneHours() {
    return _timezoneOffset / 3600;
}

int TimeManager::getTimezoneMinutes() {
    return (_timezoneOffset % 3600) / 60;
}

long TimeManager::getTimezoneOffset() {
    return _timezoneOffset;
}

void TimeManager::setNTPServer(const String& server) {
    _ntpServer = server;
    _initializeNTP();
}

void TimeManager::setNTPUpdateInterval(unsigned long intervalMs) {
    _ntpUpdateInterval = intervalMs;
    if (_timeClient) {
        _timeClient->setUpdateInterval(intervalMs);
    }
}

String TimeManager::getNTPServer() {
    return _ntpServer;
}

void TimeManager::update() {
    // Auto-sync with NTP if enabled and interval elapsed
    if (_ntpSyncEnabled && _isWiFiConnected() && 
        (millis() - _lastNTPSync) > _ntpUpdateInterval &&
        (millis() - _lastNTPAttempt) > 60000) { // Don't attempt more than once per minute
        
        _lastNTPAttempt = millis();
        if (setTimeFromNTP()) {
            Serial.println("TimeManager: Automatic NTP sync successful");
        }
    }
}

bool TimeManager::syncWithNTP() {
    return setTimeFromNTP();
}

bool TimeManager::isNTPSyncEnabled() {
    return _ntpSyncEnabled;
}

void TimeManager::enableNTPSync(bool enable) {
    _ntpSyncEnabled = enable;
}

bool TimeManager::isRTCConnected() {
    return _rtcConnected;
}

bool TimeManager::isTimeSet() {
    return _timeSet;
}

bool TimeManager::hasLostPower() {
    return _rtcConnected ? _rtc.lostPower() : true;
}

unsigned long TimeManager::getLastNTPSync() {
    return _lastNTPSync;
}

float TimeManager::getTemperature() {
    if (_rtcConnected) {
        return _rtc.getTemperature();
    }
    return NAN;
}

bool TimeManager::setAlarm1(DateTime alarmTime, Ds3231Alarm1Mode mode) {
    if (_rtcConnected) {
        return _rtc.setAlarm1(alarmTime, mode);
    }
    return false;
}

bool TimeManager::setAlarm2(DateTime alarmTime, Ds3231Alarm2Mode mode) {
    if (_rtcConnected) {
        return _rtc.setAlarm2(alarmTime, mode);
    }
    return false;
}

bool TimeManager::clearAlarm1() {
    if (_rtcConnected) {
        _rtc.clearAlarm(1);
        return true; // Just return true after calling
    }
    return false;
}

bool TimeManager::clearAlarm2() {
    if (_rtcConnected) {
        _rtc.clearAlarm(2);
        return true; // Just return true after calling
    }
    return false;
}


bool TimeManager::isAlarm1Triggered() {
    if (_rtcConnected) {
        return _rtc.alarmFired(1);
    }
    return false;
}

bool TimeManager::isAlarm2Triggered() {
    if (_rtcConnected) {
        return _rtc.alarmFired(2);
    }
    return false;
}

void TimeManager::enableSquareWave(Ds3231SqwPinMode mode) {
    if (_rtcConnected) {
        _rtc.writeSqwPinMode(mode);
    }
}

void TimeManager::disableSquareWave() {
    if (_rtcConnected) {
        _rtc.writeSqwPinMode(DS3231_OFF);
    }
}

String TimeManager::getTimeJSON() {
    DynamicJsonDocument doc(512);
    
    DateTime current = getCurrentTime();
    doc["timestamp"] = current.unixtime();
    doc["formatted"] = getFormattedTime();
    doc["date"] = getDateString();
    doc["time"] = getTimeString();
    doc["timezone_offset"] = _timezoneOffset;
    doc["timezone_hours"] = getTimezoneHours();
    doc["timezone_minutes"] = getTimezoneMinutes();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String TimeManager::getStatusJSON() {
    DynamicJsonDocument doc(512);
    
    doc["rtc_connected"] = _rtcConnected;
    doc["time_set"] = _timeSet;
    doc["has_lost_power"] = hasLostPower();
    doc["ntp_enabled"] = _ntpSyncEnabled;
    doc["ntp_server"] = _ntpServer;
    doc["last_ntp_sync"] = _lastNTPSync;
    doc["wifi_connected"] = _isWiFiConnected();
    
    if (_rtcConnected) {
        doc["temperature"] = _rtc.getTemperature();
        doc["alarm1_triggered"] = isAlarm1Triggered();
        doc["alarm2_triggered"] = isAlarm2Triggered();
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void TimeManager::saveConfig() {
    DynamicJsonDocument doc(256);
    doc["timezone_offset"] = _timezoneOffset;
    doc["ntp_server"] = _ntpServer;
    doc["ntp_update_interval"] = _ntpUpdateInterval;
    doc["ntp_sync_enabled"] = _ntpSyncEnabled;
    
    File file = LittleFS.open("/time_config.json", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("TimeManager: Configuration saved");
    }
}

void TimeManager::loadConfig() {
    File file = LittleFS.open("/time_config.json", "r");
    if (file) {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (!error) {
            _timezoneOffset = doc["timezone_offset"] | 0;
            _ntpServer = doc["ntp_server"] | "pool.ntp.org";
            _ntpUpdateInterval = doc["ntp_update_interval"] | 3600000;
            _ntpSyncEnabled = doc["ntp_sync_enabled"] | true;
            
            // Reinitialize NTP with loaded settings
            _initializeNTP();
            
            Serial.println("TimeManager: Configuration loaded");
        }
    }
}

// Private helper methods
bool TimeManager::_isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

DateTime TimeManager::_applyTimezone(DateTime utcTime) {
    return DateTime(utcTime.unixtime() + _timezoneOffset);
}

DateTime TimeManager::_removeTimezone(DateTime localTime) {
    return DateTime(localTime.unixtime() - _timezoneOffset);
}

String TimeManager::_formatDateTime(DateTime dt, const String& format) {
    String result = format;
    
    // Replace format tokens
    result.replace("YYYY", String(dt.year()));
    result.replace("MM", String(dt.month()).length() == 1 ? "0" + String(dt.month()) : String(dt.month()));
    result.replace("DD", String(dt.day()).length() == 1 ? "0" + String(dt.day()) : String(dt.day()));
    result.replace("hh", String(dt.hour()).length() == 1 ? "0" + String(dt.hour()) : String(dt.hour()));
    result.replace("mm", String(dt.minute()).length() == 1 ? "0" + String(dt.minute()) : String(dt.minute()));
    result.replace("ss", String(dt.second()).length() == 1 ? "0" + String(dt.second()) : String(dt.second()));
    
    return result;
}
