/**
 * @file ConfigManager.h
 * @brief Configuration management system for temperature monitoring device
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file provides comprehensive configuration management including WiFi setup,
 *          web interface, measurement point configuration, alarm settings, and CSV import/export.
 * 
 * @section dependencies Dependencies
 * - ConfigAssist.h for web-based configuration
 * - WebServer.h for HTTP API endpoints
 * - LittleFS.h for file system operations
 * - TemperatureController.h for device control
 * 
 * @section hardware Hardware Requirements
 * - ESP32 with WiFi capability
 * - LittleFS file system support
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ConfigAssist.h>
#include <ConfigAssistHelper.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "TemperatureController.h"
#include "CSVConfigManager.h"
#include "SettingsCSVManager.h"
#include "LoggerManager.h" 

/// YAML configuration definition for ConfigAssist
extern const char* VARIABLES_DEF_YAML;

/**
 * @brief Main configuration management class
 * @details Handles all aspects of system configuration including WiFi setup,
 *          web interface management, CSV import/export, and API endpoints
 */
class ConfigManager {
private:
    SettingsCSVManager settingsCSVManager;  ///< Manager for settings CSV operations
    CSVConfigManager csvManager;            ///< Manager for measurement point CSV operations
    ConfigAssist conf;                      ///< ConfigAssist instance for web configuration
    ConfigAssistHelper* confHelper;         ///< Helper for ConfigAssist operations
    TemperatureController& controller;      ///< Reference to temperature controller
    WebServer* server;                      ///< HTTP web server instance
    bool portalActive;                      ///< Flag indicating if configuration portal is active
    
    /**
     * @brief Static callback function for configuration changes
     * @param[in] key Configuration key that was changed
     * @details Called by ConfigAssist when any configuration value is modified
     */
    static void onConfigChanged(String key);
    
    static ConfigManager* instance;  ///< Static instance pointer for callbacks
    
    /**
     * @brief Apply configuration settings without device restart
     * @details Updates runtime parameters when configuration changes
     */
    void _applySettingsWithoutRestart();

    /**
     * @brief Setup basic API endpoints
     * @details Configures fundamental device information and status endpoints
     */
    void basicAPI();
    
    /**
     * @brief Setup sensor-related API endpoints
     * @details Configures endpoints for sensor discovery and management
     */
    void sensorAPI();
    
    /**
     * @brief Setup CSV import/export API endpoints
     * @details Configures endpoints for CSV file upload and download operations
     */
    void csvImportExportAPI();
    
    /**
     * @brief Setup measurement points API endpoints
     * @details Configures endpoints for measurement point configuration and status
     */
    void pointsAPI();
    
    /**
     * @brief Setup alarms API endpoints
     * @details Configures endpoints for alarm management and history
     */
    void alarmsAPI();
    
    /**
     * @brief Setup logging API endpoints
     * @details Configures endpoints for log file access and management
     */
    void logsAPI();
    
    /**
     * @brief Setup file download API endpoints
     * @details Configures endpoints for downloading log files and configurations
     */
    void downloadAPI();

    
    // Save sensor configuration to file
    //void saveSensorConfig();
    
    // Load sensor configuration from file
    //void loadSensorConfig();

public:
    /**
     * @brief Constructor for ConfigManager
     * @param[in] tempController Reference to the temperature controller instance
     * @details Initializes configuration system with temperature controller integration
     */
    ConfigManager(TemperatureController& tempController);
    
    /**
     * @brief Destructor for ConfigManager
     * @details Cleans up web server and configuration resources
     */
    ~ConfigManager();
    
    /**
     * @brief Initialize the configuration system
     * @return bool True if initialization successful
     * @details Sets up file system, configuration, web server and all API endpoints
     */
    bool begin();
    
    /**
     * @brief Update configuration system (call in main loop)
     * @details Handles web server requests and configuration portal updates
     */
    void update();
    
    /**
     * @brief Connect to WiFi network
     * @param[in] timeoutMs Connection timeout in milliseconds (default: 15000)
     * @return bool True if WiFi connection successful
     * @details Attempts connection using configured SSID and password
     */
    bool connectWiFi(int timeoutMs = 15000);
    
    // Add sensor to configuration
    // bool addSensorToConfig(SensorType type, uint8_t address, const String& name, 
    //                       const uint8_t* romAddress = nullptr);
    
    // Remove sensor from configuration
    //bool removeSensorFromConfig(uint8_t address);
    
    // Update sensor in configuration
    // bool updateSensorInConfig(uint8_t address, const String& name, 
    //                          int16_t lowAlarm, int16_t highAlarm);
    
    /**
     * @brief Get the web server instance
     * @return WebServer* Pointer to the HTTP web server
     * @details Provides access to the web server for additional endpoint configuration
     */
    WebServer* getWebServer() { return server; }
    
    /**
     * @brief Check if configuration portal is active
     * @return bool True if portal is currently running
     */
    bool isPortalActive() { return portalActive; }
    
    // Configuration value getters
    /**
     * @brief Get WiFi SSID from configuration
     * @return String Configured WiFi network name
     */
    String getWifiSSID() { return conf("st_ssid"); }
    
    /**
     * @brief Get WiFi password from configuration
     * @return String Configured WiFi password
     */
    String getWifiPassword() { return conf("st_pass"); }
    
    /**
     * @brief Get device hostname from configuration
     * @return String Configured device hostname
     */
    String getHostname() { return conf("host_name"); }
    
    /**
     * @brief Get device ID from configuration
     * @return uint16_t Unique device identifier
     */
    uint16_t getDeviceId() { return conf("device_id").toInt(); }
    
    /**
     * @brief Get measurement period from configuration
     * @return uint16_t Measurement interval in seconds
     */
    uint16_t getMeasurementPeriod() { return conf("measurement_period").toInt(); }
    
    /**
     * @brief Check if Modbus is enabled
     * @return bool True if Modbus communication is enabled
     */
    bool isModbusEnabled() { return conf("modbus_enabled").toInt() == 1; }
    
    /**
     * @brief Get Modbus slave address
     * @return uint8_t Configured Modbus device address
     */
    uint8_t getModbusAddress() { return conf("modbus_address").toInt(); }
    
    /**
     * @brief Get Modbus baud rate
     * @return uint32_t Configured Modbus communication speed
     */
    uint32_t getModbusBaudRate() { return conf("modbus_baud_rate").toInt(); }
    
    /**
     * @brief Get RS485 RX pin number
     * @return uint8_t GPIO pin for RS485 receive
     */
    uint8_t getRxPin() { return conf("rs485_rx_pin").toInt(); }
    
    /**
     * @brief Get RS485 TX pin number
     * @return uint8_t GPIO pin for RS485 transmit
     */
    uint8_t getTxPin() { return conf("rs485_tx_pin").toInt(); }
    
    /**
     * @brief Check if auto-discovery is enabled
     * @return bool True if automatic sensor discovery is enabled
     */
    bool getAutoDiscover() { return conf("auto_discover").toInt() == 1; }
    
    /**
     * @brief Reset minimum and maximum recorded values
     * @details Clears historical min/max temperature records for all measurement points
     */
    void resetMinMaxValues();
    //void updateSensorInConfig(Sensor* sensor);
    // Remove:
// void saveSensorConfig();
// void loadSensorConfig();
// bool addSensorToConfig(...);
// bool removeSensorFromConfig(...);
// bool updateSensorInConfig(...);
// void updateSensorInConfig(Sensor* sensor);

    /**
     * @brief Save measurement points configuration to file
     * @details Persists current measurement point settings to LittleFS
     */
    void savePointsConfig();
    
    /**
     * @brief Load measurement points configuration from file
     * @details Restores measurement point settings from LittleFS
     */
    void loadPointsConfig();
    
    /**
     * @brief Update a measurement point in configuration
     * @param[in] address Measurement point address (0-255)
     * @param[in] name Human-readable name for the measurement point
     * @param[in] lowAlarm Low temperature alarm threshold
     * @param[in] highAlarm High temperature alarm threshold
     * @param[in] ds18b20RomString DS18B20 ROM address as string (optional)
     * @param[in] pt1000ChipSelect PT1000 chip select pin (optional, -1 if not used)
     * @return bool True if update successful
     */
    bool updatePointInConfig(uint8_t address, const String& name, int16_t lowAlarm, int16_t highAlarm,
                            const String& ds18b20RomString = "", int pt1000ChipSelect = -1);
    
    /**
     * @brief Save alarms configuration to file
     * @details Persists current alarm settings to LittleFS
     */
    void saveAlarmsConfig();
    
    /**
     * @brief Load alarms configuration from file
     * @details Restores alarm settings from LittleFS
     */
    void loadAlarmsConfig();

    /**
     * @brief Get reference to CSV configuration manager
     * @return CSVConfigManager& Reference to the CSV manager instance
     */
    CSVConfigManager& getCSVManager() { return csvManager; }

    /**
     * @brief Get acknowledged delay for critical priority alarms
     * @return uint16_t Delay time in seconds for critical alarms
     */
    uint16_t getAcknowledgedDelayCritical() { return conf("ack_delay_critical").toInt(); }
    
    /**
     * @brief Get acknowledged delay for high priority alarms
     * @return uint16_t Delay time in seconds for high priority alarms
     */
    uint16_t getAcknowledgedDelayHigh() { return conf("ack_delay_high").toInt(); }
    
    /**
     * @brief Get acknowledged delay for medium priority alarms
     * @return uint16_t Delay time in seconds for medium priority alarms
     */
    uint16_t getAcknowledgedDelayMedium() { return conf("ack_delay_medium").toInt(); }
    
    /**
     * @brief Get acknowledged delay for low priority alarms
     * @return uint16_t Delay time in seconds for low priority alarms
     */
    uint16_t getAcknowledgedDelayLow() { return conf("ack_delay_low").toInt(); }
    };

// Initialize static member
//ConfigManager* ConfigManager::instance = nullptr;

#endif // CONFIG_MANAGER_H
