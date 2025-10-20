# Comprehensive Class Analysis Summary

## Core Configuration Classes

### 1. ConfigManager (ConfigManager.h/cpp)
**File Location**: `include/ConfigManager.h`, `src/ConfigManager.cpp`

**Primary Responsibility**: Main configuration management hub that handles WiFi setup, web interface management, CSV import/export, and API endpoints

**Key Methods**:
- `begin()` - Initialize configuration system
- `connectWiFi()` - Connect to WiFi network
- `getWebServer()` - Get web server instance
- `savePointsConfig()/loadPointsConfig()` - Persist/load measurement point configuration
- `saveAlarmsConfig()/loadAlarmsConfig()` - Persist/load alarm configuration
- `updatePointInConfig()` - Update individual measurement point
- `getCSVManager()` - Access CSV configuration manager
- Various getter methods for configuration values (deviceId, hostname, modbus settings, etc.)

**Important Member Variables**:
- `settingsCSVManager` - Manager for settings CSV operations
- `csvManager` - Manager for measurement point CSV operations
- `conf` - ConfigAssist instance for web configuration
- `server` - HTTP web server instance
- `controller` - Reference to temperature controller

**Notable Design Patterns**:
- Uses ConfigAssist library for web-based configuration
- Implements static callback pattern for configuration changes
- Contains nested AlarmEvent struct for alarm history tracking
- Extensive API endpoint setup through private methods (basicAPI, sensorAPI, csvImportExportAPI, etc.)

### 2. CSVConfigManager (CSVConfigManager.h/cpp)
**File Location**: `include/CSVConfigManager.h`, `src/CSVConfigManager.cpp`

**Primary Responsibility**: Handles CSV-based import/export for measurement points and alarm configuration

**Key Methods**:
- `exportPointsWithAlarmsToCSV()` - Export complete configuration to CSV
- `importPointsWithAlarmsFromCSV()` - Import configuration from CSV
- `exportSensorsToCSV()` - Export only sensor configuration
- `importSensorsFromCSV()` - Import sensor configuration
- `saveCSVToFile()` - Save CSV to filesystem
- `loadCSVFromFile()` - Load CSV from filesystem
- `validatePointsCSV()` - Validate CSV format and content

**Important Member Variables**:
- `_controller` - Reference to temperature controller
- `_lastError` - Error message storage

**Notable Design Patterns**:
- CSV field escaping/unescaping for proper data handling
- Comprehensive validation before import
- Uses CSV_Parser library for parsing
- Separates parsing logic from business logic

### 3. SettingsCSVManager (SettingsCSVManager.h/cpp)
**File Location**: `include/SettingsCSVManager.h`, `src/SettingsCSVManager.cpp`

**Primary Responsibility**: Manages CSV export/import of system settings (as opposed to measurement point configuration)

**Key Methods**:
- `exportSettingsToCSV()` - Export all system settings to CSV
- `importSettingsFromCSV()` - Import settings from CSV
- `validateSettingsCSV()` - Validate CSV data without importing
- `_exportAcknowledgedDelays()` - Export alarm acknowledgment delays
- `_importAcknowledgedDelays()` - Import alarm acknowledgment delays

**Important Member Variables**:
- `_config` - Reference to ConfigAssist configuration
- `_lastError` - Error message storage

**Notable Design Patterns**:
- Works with ConfigAssist for setting storage
- Handles special acknowledged delay settings
- Similar CSV escaping/parsing patterns as CSVConfigManager

## Logging and Data Storage Classes

### 4. LoggerManager (LoggerManager.h/cpp)
**File Location**: `include/LoggerManager.h`, `src/LoggerManager.cpp`

**Primary Responsibility**: Comprehensive data and event logging system with three distinct logging types

**Key Methods**:
- **Measurement Data Logging**:
  - `logDataNow()` - Force immediate data logging
  - `createNewLogFile()` - Create new measurement log file
  - `update()` - Main loop update for periodic logging
  
- **Event Logging**:
  - `logEvent()` - General event logging
  - `logInfo/Warning/Error/Critical()` - Priority-specific logging
  - Static convenience methods: `info()`, `warning()`, `error()`, `critical()`
  
- **Alarm State Logging**:
  - `logAlarmStateChange()` - Log alarm state transitions
  - `logAlarmState()` - Instance method for alarm logging

- **Data Retrieval**:
  - `getEventLogsJson()` - Retrieve event logs in JSON format
  - `getEventLogsCsv()` - Retrieve event logs in CSV format
  - `getAlarmHistoryJson()` - Retrieve alarm history in JSON
  - `getAlarmHistoryCsv()` - Retrieve alarm history in CSV

**Important Member Variables**:
- `_instance` - Singleton instance pointer
- `_controller` - Reference to temperature controller
- `_timeManager` - Reference for timestamp management
- `_fs` - File system interface (SD or LittleFS)
- `_logFrequency` - Logging frequency in milliseconds
- Separate directories for each log type

**Notable Design Patterns**:
- Singleton pattern for global access
- Three separate logging subsystems (measurement, event, alarm)
- Daily file rotation with sequence numbers
- Header change detection for dynamic configuration
- CSV format for all log types
- Extensive static methods for global access

## Communication Classes

### 5. MQTTManager (MQTTManager.h/cpp)
**File Location**: `include/MQTTManager.h`, `src/MQTTManager.cpp`

**Primary Responsibility**: MQTT communication using 256dpi/MQTT library with TLS support

**Key Methods**:
- `begin()` - Initialize MQTT system
- `update()` - Main update loop (handles connection, publishing)
- `publish()` - Publish message to topic
- `loadConfig()/saveConfig()` - Persist MQTT configuration
- `setConfig()/getConfig()` - Get/set configuration struct
- `buildTopic()` - Build topic based on ISA-95 hierarchy
- `testPublish()` - Test publishing functionality
- `getConfigJson()/setConfigJson()` - JSON configuration interface

**Important Member Variables** (all static):
- `config` - MQTTConfig struct with all settings
- `mqttClient` - MQTT client instance
- `wifiClient/wifiClientSecure` - WiFi clients for plain/TLS
- `publishIntervalMs` - Publishing interval
- `lastTelemetryPublish` - Timestamp tracking

**Notable Design Patterns**:
- Static class design (no instances)
- Comprehensive MQTTConfig struct for all settings
- ISA-95 topic hierarchy support
- TLS/SSL support with secure client
- LWT (Last Will and Testament) support
- QoS levels per message type
- Auto-reconnection with backoff

## System Control Classes

### 6. TemperatureController (TemperatureController.h/cpp)
**File Location**: `include/TemperatureController.h`, `src/TemperatureController.cpp`

**Primary Responsibility**: Main controller coordinating all temperature monitoring operations

**Key Methods**:
- `begin()` - Initialize controller system
- `getMeasurementPoint()` - Get point by address
- `addSensor()/removeSensorByRom()` - Sensor management
- `bindSensorToPointByRom()` - Bind sensors to measurement points
- `update()` - Main update loop
- `discoverDS18B20Sensors()` - Auto-discover OneWire sensors
- `readAllSensors()` - Read all sensor values
- `checkAllAlarms()` - Process alarm conditions
- Various alarm management methods

**Important Member Variables**:
- Arrays of measurement points (DS18B20 and PT1000)
- Vector of sensors
- OneWire buses
- Alarm priority queue
- Reference to indicator interface

**Notable Design Patterns**:
- Central coordinator pattern
- Manages both DS18B20 and PT1000 sensor types
- Priority-based alarm queue
- Separation between sensors and measurement points
- Extensive alarm acknowledgment system

## Additional Key Classes to Consider

### 7. MeasurementPoint (MeasurementPoint.h)
**Primary Responsibility**: Logical measurement point abstraction

**Key Features**:
- Temperature value storage with min/max tracking
- Alarm configuration (high/low thresholds)
- Sensor binding
- Point naming and addressing
- Valid/invalid state tracking

### 8. Alarm (Alarm.h)
**Primary Responsibility**: Individual alarm instance management

**Key Features**:
- Alarm state machine (normal, active, acknowledged)
- Priority levels
- Hysteresis handling
- Timestamp tracking
- Link to measurement point

### 9. RegisterMap (RegisterMap.h)
**Primary Responsibility**: Modbus register mapping

**Key Features**:
- Maps measurement points to Modbus registers
- Handles different data types
- Register allocation and management

### 10. IndicatorInterface (IndicatorInterface.h)
**Primary Responsibility**: Hardware indicator control (LEDs, display)

**Key Features**:
- PCF8575 I/O expander control
- OLED display management
- LED blinking patterns
- Port state management

## Key Observations for New Feature Implementation

Based on the PRD requirements for enhanced logging and CSV operations, these classes will need modifications:

1. **LoggerManager** - Already has comprehensive logging infrastructure but may need:
   - Enhanced MQTT log forwarding integration
   - Additional log filtering capabilities
   - Bulk export methods for web interface

2. **CSVConfigManager** - May need:
   - Additional validation rules
   - Template generation methods
   - Batch import/export improvements

3. **ConfigManager** - Will need:
   - New API endpoints for enhanced CSV operations
   - Integration points for MQTT log configuration
   - Additional web UI handlers

4. **MQTTManager** - May need:
   - Log forwarding capabilities
   - Subscription handlers for configuration updates
   - Enhanced topic management for logging

The architecture is well-structured with clear separation of concerns, making it suitable for the planned enhancements in the PRD.