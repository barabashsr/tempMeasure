#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.h"
#include "MeasurementPoint.h"
#include "RegisterMap.h"
#include "IndicatorInterface.h"
#include "Alarm.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <algorithm>

class TemperatureController {
public:
    TemperatureController(uint8_t oneWirePin[4], uint8_t csPin[4], IndicatorInterface& indicator);

    

    ~TemperatureController();
    
    bool begin();
    
    // Measurement point management
    MeasurementPoint* getMeasurementPoint(uint8_t address);
    MeasurementPoint* getDS18B20Point(uint8_t idx);
    MeasurementPoint* getPT1000Point(uint8_t idx);
    
    // Sensor management
    bool addSensor(Sensor* sensor);
    bool removeSensorByRom(const String& romString);
    Sensor* findSensorByRom(const String& romString);
    Sensor* findSensorByChipSelect(uint8_t csPin);
    int getSensorCount() const { return sensors.size(); }
    Sensor* getSensorByIndex(int idx);
    
    // Sensor binding
    bool bindSensorToPointByRom(const String& romString, uint8_t pointAddress);
    bool bindSensorToPointByChipSelect(uint8_t csPin, uint8_t pointAddress);
    bool unbindSensorFromPoint(uint8_t pointAddress);
    Sensor* getBoundSensor(uint8_t pointAddress);
    bool unbindSensorFromPointBySensor(Sensor* sensor);
    
    // Main update and measurement
    void update();
    void readAllPoints();
    void updateRegisterMap();
    void applyConfigFromRegisterMap();
    void applyConfigToRegisterMap();
    
    // Sensor discovery
    bool discoverDS18B20Sensors();
    bool discoverPTSensors();
    
    // JSON output
    String getSensorsJson();
    String getPointsJson();
    String getSystemStatusJson();
    
    // Utility functions
    void resetMinMaxValues();
    RegisterMap& getRegisterMap() { return registerMap; }
    
    // Configuration
    void setDeviceId(uint16_t id);
    uint16_t getDeviceId() const;
    void setFirmwareVersion(uint16_t version);
    uint16_t getFirmwareVersion() const;
    void setMeasurementPeriod(uint16_t seconds);
    uint16_t getMeasurementPeriod() const;
    void setOneWireBusPin(uint8_t pin, size_t idx);
    uint8_t getOneWirePin(size_t bus);
    
    // Statistics
    int getDS18B20Count() const;
    int getPT1000Count() const;
    void updateAllSensors();
    int getSensorBus(Sensor* sensor);
    
    // New Alarm Management
    void updateAlarms();
    String getAlarmsJson();
    void handleAlarmDisplay();
    void handleAlarmOutputs();

    std::vector<Alarm*> getActiveAlarms() const;
    void createAlarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority);
    Alarm* getHighestPriorityAlarm() const;
    void acknowledgeHighestPriorityAlarm();
    void acknowledgeAllAlarms();
    void clearResolvedAlarms();
    void clearConfiguredAlarms();

    // Alarm management (similar to sensor management)
    bool addAlarm(AlarmType type, uint8_t pointAddress, AlarmPriority priority);
    bool removeAlarm(const String& configKey);
    bool updateAlarm(const String& configKey, AlarmPriority priority, bool enabled);
    Alarm* findAlarm(const String& configKey);
    Alarm* getAlarmByIndex(int idx);
    int getAlarmCount() const { return _configuredAlarms.size(); }
    std::vector<Alarm*> getConfiguredAlarms() const { return _configuredAlarms; }

    // JSON output (similar to getSensorsJson, getPointsJson)
    //String getAlarmsJson();

    // Alarm handling scenarios (placeholders)
    void handleCriticalAlarms();
    void handleHighPriorityAlarms();
    void handleMediumPriorityAlarms();
    void handleLowPriorityAlarms();

    bool bindSensorToPointByBusNumber(uint8_t busNumber, uint8_t pointAddress);


    // Setters for acknowledged delays
    void setAcknowledgedDelayCritical(unsigned long delay);
    void setAcknowledgedDelayHigh(unsigned long delay);
    void setAcknowledgedDelayMedium(unsigned long delay);
    void setAcknowledgedDelayLow(unsigned long delay);
    
    // Getters for acknowledged delays
    unsigned long getAcknowledgedDelayCritical() const;
    unsigned long getAcknowledgedDelayHigh() const;
    unsigned long getAcknowledgedDelayMedium() const;
    unsigned long getAcknowledgedDelayLow() const;
    
    // Method to apply delays to existing alarms
    void applyAcknowledgedDelaysToAlarms();

    // int getAlarmCount(AlarmPriority priority) const;
    // int getAlarmCount(AlarmStage stage) const;
    int getAlarmCount(AlarmPriority priority, AlarmStage stage) const;
    // Add these method declarations to TemperatureController.h
    int getAlarmCount(AlarmPriority priority, const String& comparison = "==") const;
    int getAlarmCount(AlarmStage stage, const String& comparison = "==") const;
    int getAlarmCount(AlarmPriority priority, AlarmStage stage, const String& priorityComparison = "==", const String& stageComparison = "==") const;






private:
    // Hardware components
    IndicatorInterface& indicator;
    OneWire* oneWireBuses[4];
    DallasTemperature* dallasSensors[4];
    
    // Measurement points and sensors
    MeasurementPoint dsPoints[50];
    MeasurementPoint ptPoints[10];
    std::vector<Sensor*> sensors;
    
    // System configuration
    RegisterMap registerMap;
    uint16_t measurementPeriodSeconds;
    uint16_t deviceId;
    uint16_t firmwareVersion;
    unsigned long lastMeasurementTime;
    bool systemInitialized;
    uint8_t oneWireBusPin[4];
    uint8_t chipSelectPin[4];
    
    // Alarm system
    //std::vector<Alarm*> _alarms;
    std::vector<Alarm*> _configuredAlarms; 
    unsigned long _lastAlarmCheck;
    const unsigned long _alarmCheckInterval = 1000; // Check every second
    bool _lastButtonState;
    unsigned long _lastButtonPressTime;
    const unsigned long _buttonDebounceDelay = 200;
    
    // Display management
    Alarm* _currentDisplayedAlarm;
    unsigned long _okDisplayStartTime;
    bool _showingOK;
    
    // Internal methods
    bool isDS18B20Address(uint8_t address) const { return address < 50; }
    bool isPT1000Address(uint8_t address) const { return address >= 50 && address < 60; }
    
    // Alarm helper methods
    void _checkPointForAlarms(MeasurementPoint* point);
    bool _hasAlarmForPoint(MeasurementPoint* point, AlarmType type);
    void _checkButtonPress();
    void _updateNormalDisplay();
    void _showOKAndTurnOffOLED();

    unsigned long _acknowledgedDelayCritical;
    unsigned long _acknowledgedDelayHigh;
    unsigned long _acknowledgedDelayMedium;
    unsigned long _acknowledgedDelayLow;

    bool _relay1State = false;
    bool _relay2State = false;
    bool _redLedState = false;
    bool _yellowLedState = false;
    bool _blueLedState = false;

    bool _comparePriority(AlarmPriority alarmPriority, AlarmPriority targetPriority, const String& comparison) const;
    bool _compareStage(AlarmStage alarmStage, AlarmStage targetStage, const String& comparison) const;

    // Blinking control for low priority alarms
    bool _lowPriorityBlinkState = false;
    unsigned long _lastLowPriorityBlinkTime = 0;
    const unsigned long _blinkOnTime = 2000;   // 2 seconds on
    const unsigned long _blinkOffTime = 30000; // 30 seconds off
    void _handleLowPriorityBlinking();


    // Display management for alarms
    std::vector<Alarm*> _activeAlarmsQueue;
    std::vector<Alarm*> _acknowledgedAlarmsQueue;
    int _currentActiveAlarmIndex;
    int _currentAcknowledgedAlarmIndex;
    unsigned long _lastAlarmDisplayTime;
    unsigned long _acknowledgedAlarmDisplayDelay;
    bool _displayingActiveAlarm;
    
    // Helper methods for alarm display
    void _updateAlarmQueues();
    void _displayNextActiveAlarm();
    void _displayNextAcknowledgedAlarm();
    void _handleAlarmDisplayRotation();

    String _getPriorityString(AlarmPriority priority) const; 


    String _getAlarmTypeString(AlarmType type) const;
    
    //String _getPriorityString(AlarmPriority priority) const;

    
};
