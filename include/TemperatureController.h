/**
 * @file TemperatureController.h
 * @brief Central temperature monitoring and control system
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details This class manages all temperature sensors, measurement points, alarms,
 *          and system configuration for the temperature monitoring system. It provides
 *          interfaces for sensor discovery, data collection, alarm management, and
 *          Modbus communication through register mapping.
 * 
 * @section dependencies Dependencies
 * - OneWire library for DS18B20 sensors
 * - DallasTemperature for DS18B20 communication
 * - ArduinoJson for JSON serialization
 * - Custom classes: Sensor, MeasurementPoint, RegisterMap, IndicatorInterface, Alarm
 * 
 * @section hardware Hardware Requirements
 * - Up to 4 OneWire buses for DS18B20 sensors
 * - Up to 4 SPI chip select pins for PT1000 sensors
 * - Support for up to 50 DS18B20 sensors
 * - Support for up to 10 PT1000 sensors
 * - LED indicators and relay outputs for alarm signaling
 */

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

/**
 * @class TemperatureController
 * @brief Main controller for temperature monitoring system
 * @details Manages temperature sensors, measurement points, alarms, and system configuration.
 *          Provides unified interface for sensor operations, alarm handling, and data access.
 *          Supports both DS18B20 and PT1000 temperature sensors.
 */
class TemperatureController {
public:
    /**
     * @brief Construct a new Temperature Controller object
     * @param[in] oneWirePin Array of 4 OneWire bus pins for DS18B20 sensors
     * @param[in] csPin Array of 4 chip select pins for PT1000 sensors
     * @param[in] indicator Reference to the indicator interface for display/LED control
     */
    TemperatureController(uint8_t oneWirePin[4], uint8_t csPin[4], IndicatorInterface& indicator);

    /**
     * @brief Destroy the Temperature Controller object
     * @details Cleans up dynamically allocated sensors and OneWire buses
     */
    ~TemperatureController();
    
    /**
     * @brief Initialize the temperature controller system
     * @return true if initialization successful
     * @return false if initialization failed
     * @details Initializes OneWire buses, discovers sensors, loads configuration
     */
    bool begin();
    
    // Measurement point management
    /**
     * @brief Get measurement point by address
     * @param[in] address Point address (0-49 for DS18B20, 50-59 for PT1000)
     * @return MeasurementPoint* Pointer to measurement point or nullptr if invalid address
     */
    MeasurementPoint* getMeasurementPoint(uint8_t address);
    
    /**
     * @brief Get DS18B20 measurement point by index
     * @param[in] idx Index in DS18B20 array (0-49)
     * @return MeasurementPoint* Pointer to DS18B20 point or nullptr if invalid index
     */
    MeasurementPoint* getDS18B20Point(uint8_t idx);
    
    /**
     * @brief Get PT1000 measurement point by index
     * @param[in] idx Index in PT1000 array (0-9)
     * @return MeasurementPoint* Pointer to PT1000 point or nullptr if invalid index
     */
    MeasurementPoint* getPT1000Point(uint8_t idx);
    
    // Sensor management
    /**
     * @brief Add a sensor to the controller
     * @param[in] sensor Pointer to sensor object to add
     * @return true if sensor added successfully
     * @return false if sensor already exists or nullptr
     */
    bool addSensor(Sensor* sensor);
    
    /**
     * @brief Remove sensor by ROM address string
     * @param[in] romString ROM address as hex string (e.g., "28FF123456789ABC")
     * @return true if sensor removed successfully
     * @return false if sensor not found
     */
    bool removeSensorByRom(const String& romString);
    
    /**
     * @brief Find sensor by ROM address string
     * @param[in] romString ROM address as hex string
     * @return Sensor* Pointer to sensor or nullptr if not found
     */
    Sensor* findSensorByRom(const String& romString);
    
    /**
     * @brief Find PT1000 sensor by chip select pin
     * @param[in] csPin Chip select pin number
     * @return Sensor* Pointer to sensor or nullptr if not found
     */
    Sensor* findSensorByChipSelect(uint8_t csPin);
    
    /**
     * @brief Get total number of sensors
     * @return int Number of sensors registered
     */
    int getSensorCount() const { return sensors.size(); }
    
    /**
     * @brief Get sensor by index
     * @param[in] idx Sensor index
     * @return Sensor* Pointer to sensor or nullptr if invalid index
     */
    Sensor* getSensorByIndex(int idx);
    
    // Sensor binding
    /**
     * @brief Bind sensor to measurement point by ROM address
     * @param[in] romString ROM address of sensor to bind
     * @param[in] pointAddress Measurement point address
     * @return true if binding successful
     * @return false if sensor or point not found
     */
    bool bindSensorToPointByRom(const String& romString, uint8_t pointAddress);
    
    /**
     * @brief Bind PT1000 sensor to measurement point by chip select pin
     * @param[in] csPin Chip select pin of PT1000 sensor
     * @param[in] pointAddress Measurement point address
     * @return true if binding successful
     * @return false if sensor or point not found
     */
    bool bindSensorToPointByChipSelect(uint8_t csPin, uint8_t pointAddress);
    
    /**
     * @brief Unbind sensor from measurement point
     * @param[in] pointAddress Measurement point address to unbind
     * @return true if unbinding successful
     * @return false if point not found or not bound
     */
    bool unbindSensorFromPoint(uint8_t pointAddress);
    
    /**
     * @brief Get sensor bound to measurement point
     * @param[in] pointAddress Measurement point address
     * @return Sensor* Pointer to bound sensor or nullptr if not bound
     */
    Sensor* getBoundSensor(uint8_t pointAddress);
    
    /**
     * @brief Unbind sensor from any measurement point it's bound to
     * @param[in] sensor Pointer to sensor to unbind
     * @return true if unbinding successful
     * @return false if sensor not bound to any point
     */
    bool unbindSensorFromPointBySensor(Sensor* sensor);
    
    // Main update and measurement
    /**
     * @brief Main update function - must be called in loop()
     * @details Reads sensors, updates alarms, handles display and outputs
     * @note Call this regularly for proper system operation
     */
    void update();
    
    /**
     * @brief Read temperature from all configured measurement points
     * @details Updates temperature values for all points with bound sensors
     */
    void readAllPoints();
    
    /**
     * @brief Update register map with current values from measurement points
     * @details Synchronizes Modbus registers with current temperature data
     */
    void updateRegisterMap();
    
    /**
     * @brief Apply configuration from register map to system
     * @details Updates system settings from Modbus register values
     */
    void applyConfigFromRegisterMap();
    
    /**
     * @brief Apply current system configuration to register map
     * @details Updates Modbus registers with current system settings
     */
    void applyConfigToRegisterMap();
    
    // Sensor discovery
    /**
     * @brief Discover DS18B20 sensors on all OneWire buses
     * @return true if any sensors discovered
     * @return false if no sensors found
     * @details Scans all 4 OneWire buses for DS18B20 sensors
     */
    bool discoverDS18B20Sensors();
    
    /**
     * @brief Discover PT1000 sensors on SPI bus
     * @return true if any sensors discovered
     * @return false if no sensors found
     * @details Checks all 4 chip select pins for PT1000 sensors
     */
    bool discoverPTSensors();
    
    // JSON output
    /**
     * @brief Get JSON representation of all sensors
     * @return String JSON array of sensor objects
     * @details Includes sensor type, ROM/CS, binding status, and current value
     */
    String getSensorsJson();
    
    /**
     * @brief Get JSON representation of all measurement points
     * @return String JSON array of measurement point objects
     * @details Includes point address, name, value, limits, and alarm status
     */
    String getPointsJson();
    
    /**
     * @brief Get JSON representation of system status
     * @return String JSON object with system information
     * @details Includes device ID, firmware version, sensor counts, alarm status
     */
    String getSystemStatusJson();
    
    // Utility functions
    /**
     * @brief Reset min/max values for all measurement points
     * @details Clears historical minimum and maximum temperature records
     */
    void resetMinMaxValues();
    
    /**
     * @brief Get reference to register map
     * @return RegisterMap& Reference to the Modbus register map
     */
    RegisterMap& getRegisterMap() { return registerMap; }
    
    // Configuration
    /**
     * @brief Set device ID
     * @param[in] id Device identifier (0-65535)
     */
    void setDeviceId(uint16_t id);
    
    /**
     * @brief Get device ID
     * @return uint16_t Current device identifier
     */
    uint16_t getDeviceId() const;
    
    /**
     * @brief Set firmware version
     * @param[in] version Firmware version number
     */
    void setFirmwareVersion(uint16_t version);
    
    /**
     * @brief Get firmware version
     * @return uint16_t Current firmware version
     */
    uint16_t getFirmwareVersion() const;
    
    /**
     * @brief Set measurement period
     * @param[in] seconds Time between measurements in seconds
     */
    void setMeasurementPeriod(uint16_t seconds);
    
    /**
     * @brief Get measurement period
     * @return uint16_t Current measurement period in seconds
     */
    uint16_t getMeasurementPeriod() const;
    
    /**
     * @brief Set OneWire bus pin
     * @param[in] pin GPIO pin number
     * @param[in] idx Bus index (0-3)
     */
    void setOneWireBusPin(uint8_t pin, size_t idx);
    
    /**
     * @brief Get OneWire bus pin
     * @param[in] bus Bus index (0-3)
     * @return uint8_t GPIO pin number or 0xFF if invalid bus
     */
    uint8_t getOneWirePin(size_t bus);
    
    // Statistics
    /**
     * @brief Get count of discovered DS18B20 sensors
     * @return int Number of DS18B20 sensors
     */
    int getDS18B20Count() const;
    
    /**
     * @brief Get count of discovered PT1000 sensors
     * @return int Number of PT1000 sensors
     */
    int getPT1000Count() const;
    
    /**
     * @brief Update temperature readings for all sensors
     * @details Forces immediate temperature reading from all sensors
     */
    void updateAllSensors();
    
    /**
     * @brief Get bus number for a sensor
     * @param[in] sensor Pointer to sensor
     * @return int Bus number (0-3) or -1 if not found
     */
    int getSensorBus(Sensor* sensor);
    
    // New Alarm Management
    /**
     * @brief Update alarm states for all measurement points
     * @details Checks alarm conditions and updates alarm states
     */
    void updateAlarms();
    
    /**
     * @brief Get JSON representation of all alarms
     * @return String JSON array of alarm objects
     * @details Includes alarm type, priority, state, and associated point
     */
    String getAlarmsJson();
    
    /**
     * @brief Handle alarm display on OLED/LEDs
     * @details Updates display with highest priority active alarm
     */
    void handleAlarmDisplay();
    
    /**
     * @brief Handle alarm output signals
     * @details Controls relays and LEDs based on alarm states
     */
    void handleAlarmOutputs();

    /**
     * @brief Get list of active alarms
     * @return std::vector<Alarm*> Vector of active alarm pointers
     */
    std::vector<Alarm*> getActiveAlarms() const;
    
    /**
     * @brief Create new alarm for measurement point
     * @param[in] type Alarm type (HIGH_TEMP, LOW_TEMP, etc.)
     * @param[in] source Measurement point that triggered alarm
     * @param[in] priority Alarm priority level
     */
    void createAlarm(AlarmType type, MeasurementPoint* source, AlarmPriority priority);
    
    /**
     * @brief Get highest priority active alarm
     * @return Alarm* Pointer to highest priority alarm or nullptr
     */
    Alarm* getHighestPriorityAlarm() const;
    
    /**
     * @brief Acknowledge highest priority alarm
     * @details Moves alarm from active to acknowledged state
     */
    void acknowledgeHighestPriorityAlarm();
    
    /**
     * @brief Acknowledge all active alarms
     * @details Moves all alarms to acknowledged state
     */
    void acknowledgeAllAlarms();
    
    /**
     * @brief Clear resolved alarms from system
     * @details Removes alarms that are no longer active
     */
    void clearResolvedAlarms();
    
    /**
     * @brief Clear all configured alarms
     * @details Removes all alarm configurations
     * @warning This deletes all alarm settings
     */
    void clearConfiguredAlarms();
    
    /**
     * @brief Ensure all 3 alarm types exist for a measurement point
     * @param[in] point Measurement point to create alarms for
     * @details Creates LOW_TEMPERATURE, HIGH_TEMPERATURE, and SENSOR_ERROR alarms if they don't exist
     */
    void ensureAlarmsForPoint(MeasurementPoint* point);
    
    /**
     * @brief Get all alarms associated with a measurement point
     * @param[in] point Measurement point to get alarms for
     * @return std::vector<Alarm*> Vector of alarms for the point
     */
    std::vector<Alarm*> getAlarmsForPoint(MeasurementPoint* point);

    // Alarm management (similar to sensor management)
    /**
     * @brief Add alarm configuration
     * @param[in] type Alarm type
     * @param[in] pointAddress Measurement point address
     * @param[in] priority Alarm priority
     * @return true if alarm added successfully
     * @return false if alarm already exists or invalid parameters
     */
    bool addAlarm(AlarmType type, uint8_t pointAddress, AlarmPriority priority);
    
    /**
     * @brief Remove alarm by configuration key
     * @param[in] configKey Alarm configuration key
     * @return true if alarm removed successfully
     * @return false if alarm not found
     */
    bool removeAlarm(const String& configKey);
    
    /**
     * @brief Update alarm configuration
     * @param[in] configKey Alarm configuration key
     * @param[in] priority New priority level
     * @param[in] enabled Enable/disable alarm
     * @return true if update successful
     * @return false if alarm not found
     */
    bool updateAlarm(const String& configKey, AlarmPriority priority, bool enabled);
    
    /**
     * @brief Find alarm by configuration key
     * @param[in] configKey Alarm configuration key
     * @return Alarm* Pointer to alarm or nullptr if not found
     */
    Alarm* findAlarm(const String& configKey);
    
    /**
     * @brief Get alarm by index
     * @param[in] idx Alarm index
     * @return Alarm* Pointer to alarm or nullptr if invalid index
     */
    Alarm* getAlarmByIndex(int idx);
    
    /**
     * @brief Get total number of configured alarms
     * @return int Number of configured alarms
     */
    int getAlarmCount() const { return _configuredAlarms.size(); }
    
    /**
     * @brief Get list of configured alarms
     * @return std::vector<Alarm*> Vector of configured alarm pointers
     */
    std::vector<Alarm*> getConfiguredAlarms() const { return _configuredAlarms; }

    // Alarm handling scenarios
    /**
     * @brief Handle critical priority alarms
     * @details Implements specific behavior for critical alarms
     */
    void handleCriticalAlarms();
    
    /**
     * @brief Handle high priority alarms
     * @details Implements specific behavior for high priority alarms
     */
    void handleHighPriorityAlarms();
    
    /**
     * @brief Handle medium priority alarms
     * @details Implements specific behavior for medium priority alarms
     */
    void handleMediumPriorityAlarms();
    
    /**
     * @brief Handle low priority alarms
     * @details Implements specific behavior for low priority alarms
     */
    void handleLowPriorityAlarms();

    /**
     * @brief Bind sensor to measurement point by bus number
     * @param[in] busNumber OneWire bus number (0-3)
     * @param[in] pointAddress Measurement point address
     * @return true if binding successful
     * @return false if invalid bus or point
     */
    bool bindSensorToPointByBusNumber(uint8_t busNumber, uint8_t pointAddress);

    // Setters for acknowledged delays
    /**
     * @brief Set acknowledge delay for critical alarms
     * @param[in] delay Delay in milliseconds
     */
    void setAcknowledgedDelayCritical(unsigned long delay);
    
    /**
     * @brief Set acknowledge delay for high priority alarms
     * @param[in] delay Delay in milliseconds
     */
    void setAcknowledgedDelayHigh(unsigned long delay);
    
    /**
     * @brief Set acknowledge delay for medium priority alarms
     * @param[in] delay Delay in milliseconds
     */
    void setAcknowledgedDelayMedium(unsigned long delay);
    
    /**
     * @brief Set acknowledge delay for low priority alarms
     * @param[in] delay Delay in milliseconds
     */
    void setAcknowledgedDelayLow(unsigned long delay);
    
    // Getters for acknowledged delays
    /**
     * @brief Get acknowledge delay for critical alarms
     * @return unsigned long Delay in milliseconds
     */
    unsigned long getAcknowledgedDelayCritical() const;
    
    /**
     * @brief Get acknowledge delay for high priority alarms
     * @return unsigned long Delay in milliseconds
     */
    unsigned long getAcknowledgedDelayHigh() const;
    
    /**
     * @brief Get acknowledge delay for medium priority alarms
     * @return unsigned long Delay in milliseconds
     */
    unsigned long getAcknowledgedDelayMedium() const;
    
    /**
     * @brief Get acknowledge delay for low priority alarms
     * @return unsigned long Delay in milliseconds
     */
    unsigned long getAcknowledgedDelayLow() const;
    
    /**
     * @brief Apply acknowledged delays to existing alarms
     * @details Updates all existing alarms with current delay settings
     */
    void applyAcknowledgedDelaysToAlarms();

    /**
     * @brief Get count of alarms by priority
     * @param[in] priority Priority level to count
     * @param[in] comparison Comparison operator ("==", ">=", "<=", ">", "<")
     * @return int Number of alarms matching criteria
     */
    int getAlarmCount(AlarmPriority priority, const String& comparison = "==") const;
    
    /**
     * @brief Get count of alarms by stage
     * @param[in] stage Alarm stage to count
     * @param[in] comparison Comparison operator ("==", "!=")
     * @return int Number of alarms matching criteria
     */
    int getAlarmCount(AlarmStage stage, const String& comparison = "==") const;
    
    /**
     * @brief Get count of alarms by priority and stage
     * @param[in] priority Priority level to filter
     * @param[in] stage Alarm stage to filter
     * @param[in] priorityComparison Priority comparison operator
     * @param[in] stageComparison Stage comparison operator
     * @return int Number of alarms matching both criteria
     */
    int getAlarmCount(AlarmPriority priority, AlarmStage stage, const String& priorityComparison = "==", const String& stageComparison = "==") const;






private:
    // Hardware components
    IndicatorInterface& indicator;              ///< Reference to indicator interface for display/LED control
    OneWire* oneWireBuses[4];                  ///< Array of OneWire bus objects
    DallasTemperature* dallasSensors[4];       ///< Array of Dallas temperature sensor objects
    
    // Measurement points and sensors
    MeasurementPoint dsPoints[50];             ///< Array of DS18B20 measurement points
    MeasurementPoint ptPoints[10];             ///< Array of PT1000 measurement points
    std::vector<Sensor*> sensors;              ///< Vector of all discovered sensors
    
    // System configuration
    RegisterMap registerMap;                   ///< Modbus register map for communication
    uint16_t measurementPeriodSeconds;         ///< Time between measurements in seconds
    uint16_t deviceId;                         ///< Unique device identifier
    uint16_t firmwareVersion;                  ///< Firmware version number
    unsigned long lastMeasurementTime;         ///< Timestamp of last measurement
    bool systemInitialized;                    ///< System initialization flag
    uint8_t oneWireBusPin[4];                 ///< GPIO pins for OneWire buses
    uint8_t chipSelectPin[4];                 ///< Chip select pins for PT1000 sensors
    
    // Alarm system
    std::vector<Alarm*> _configuredAlarms;         ///< Vector of configured alarm objects
    unsigned long _lastAlarmCheck;                 ///< Timestamp of last alarm check
    const unsigned long _alarmCheckInterval = 1000; ///< Alarm check interval in milliseconds
    bool _lastButtonState;                         ///< Previous button state for edge detection
    unsigned long _lastButtonPressTime;            ///< Timestamp of last button press
    const unsigned long _buttonDebounceDelay = 200; ///< Button debounce delay in milliseconds
    
    // Display management
    Alarm* _currentDisplayedAlarm;                 ///< Currently displayed alarm on OLED
    unsigned long _okDisplayStartTime;             ///< Timestamp when OK display started
    bool _showingOK;                               ///< Flag indicating OK status display
    
    // Internal methods
    /**
     * @brief Check if address is for DS18B20 sensor
     * @param[in] address Point address to check
     * @return true if DS18B20 address (0-49)
     */
    bool isDS18B20Address(uint8_t address) const { return address < 50; }
    
    /**
     * @brief Check if address is for PT1000 sensor
     * @param[in] address Point address to check
     * @return true if PT1000 address (50-59)
     */
    bool isPT1000Address(uint8_t address) const { return address >= 50 && address < 60; }
    
    // Alarm helper methods
    /**
     * @brief Check measurement point for alarm conditions
     * @param[in] point Measurement point to check
     */
    void _checkPointForAlarms(MeasurementPoint* point);
    
    /**
     * @brief Check if alarm exists for point and type
     * @param[in] point Measurement point to check
     * @param[in] type Alarm type to check
     * @return true if alarm exists for this point/type combination
     */
    bool _hasAlarmForPoint(MeasurementPoint* point, AlarmType type);
    
    /**
     * @brief Check for button press and handle acknowledgment
     */
    void _checkButtonPress();
    
    /**
     * @brief Update normal temperature display
     */
    void _updateNormalDisplay();
    
    /**
     * @brief Show OK status and turn off OLED after delay
     */
    void _showOKAndTurnOffOLED();

    unsigned long _acknowledgedDelayCritical;     ///< Acknowledge delay for critical alarms
    unsigned long _acknowledgedDelayHigh;          ///< Acknowledge delay for high priority alarms
    unsigned long _acknowledgedDelayMedium;        ///< Acknowledge delay for medium priority alarms
    unsigned long _acknowledgedDelayLow;           ///< Acknowledge delay for low priority alarms

    bool _relay1State = false;                     ///< Current state of relay 1
    bool _relay2State = false;                     ///< Current state of relay 2
    bool _redLedState = false;                     ///< Current state of red LED
    bool _yellowLedState = false;                  ///< Current state of yellow LED
    bool _blueLedState = false;                    ///< Current state of blue LED

    /**
     * @brief Compare alarm priority with comparison operator
     * @param[in] alarmPriority Priority to compare
     * @param[in] targetPriority Target priority
     * @param[in] comparison Comparison operator string
     * @return true if comparison matches
     */
    bool _comparePriority(AlarmPriority alarmPriority, AlarmPriority targetPriority, const String& comparison) const;
    
    /**
     * @brief Compare alarm stage with comparison operator
     * @param[in] alarmStage Stage to compare
     * @param[in] targetStage Target stage
     * @param[in] comparison Comparison operator string
     * @return true if comparison matches
     */
    bool _compareStage(AlarmStage alarmStage, AlarmStage targetStage, const String& comparison) const;

    // Blinking control for low priority alarms
    bool _lowPriorityBlinkState = false;           ///< Current blink state for low priority alarms
    unsigned long _lastLowPriorityBlinkTime = 0;   ///< Last blink state change timestamp
    const unsigned long _blinkOnTime = 2000;       ///< Blink on duration (2 seconds)
    const unsigned long _blinkOffTime = 30000;     ///< Blink off duration (30 seconds)
    
    /**
     * @brief Handle LED blinking for low priority alarms
     */
    void _handleLowPriorityBlinking();

    // Display management for alarms
    std::vector<Alarm*> _activeAlarmsQueue;        ///< Queue of active alarms for display rotation
    std::vector<Alarm*> _acknowledgedAlarmsQueue;  ///< Queue of acknowledged alarms for display
    int _currentActiveAlarmIndex;                  ///< Current index in active alarms queue
    int _currentAcknowledgedAlarmIndex;            ///< Current index in acknowledged alarms queue
    unsigned long _lastAlarmDisplayTime;           ///< Last alarm display update timestamp
    unsigned long _acknowledgedAlarmDisplayDelay;  ///< Delay before showing acknowledged alarms
    bool _displayingActiveAlarm;                   ///< Flag indicating if displaying active alarm
    
    // Helper methods for alarm display
    /**
     * @brief Update alarm display queues
     */
    void _updateAlarmQueues();
    
    /**
     * @brief Display next active alarm in rotation
     */
    void _displayNextActiveAlarm();
    
    /**
     * @brief Display next acknowledged alarm
     */
    void _displayNextAcknowledgedAlarm();
    
    /**
     * @brief Handle alarm display rotation timing
     */
    void _handleAlarmDisplayRotation();

    /**
     * @brief Get string representation of alarm priority
     * @param[in] priority Alarm priority enum value
     * @return String Priority as string
     */
    String _getPriorityString(AlarmPriority priority) const;

    /**
     * @brief Get string representation of alarm type
     * @param[in] type Alarm type enum value
     * @return String Alarm type as string
     */
    String _getAlarmTypeString(AlarmType type) const;
};
