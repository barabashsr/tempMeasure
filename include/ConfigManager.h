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

// YAML configuration definition
extern const char* VARIABLES_DEF_YAML;

class ConfigManager {
private:
    SettingsCSVManager settingsCSVManager;
    CSVConfigManager csvManager;
    ConfigAssist conf;
    ConfigAssistHelper* confHelper;
    TemperatureController& controller;
    WebServer* server;
    bool portalActive;
    
    // Callback function for ConfigAssist
    static void onConfigChanged(String key);
    
    // Pointer to instance for callback functions
    static ConfigManager* instance;
    void _applySettingsWithoutRestart();
    
    // Save sensor configuration to file
    //void saveSensorConfig();
    
    // Load sensor configuration from file
    //void loadSensorConfig();

public:
    ConfigManager(TemperatureController& tempController);
    ~ConfigManager();
    
    // Initialize configuration
    bool begin();
    
    // Update configuration (call in loop)
    void update();
    
    // Connect to WiFi
    bool connectWiFi(int timeoutMs = 15000);
    
    // Add sensor to configuration
    // bool addSensorToConfig(SensorType type, uint8_t address, const String& name, 
    //                       const uint8_t* romAddress = nullptr);
    
    // Remove sensor from configuration
    //bool removeSensorFromConfig(uint8_t address);
    
    // Update sensor in configuration
    // bool updateSensorInConfig(uint8_t address, const String& name, 
    //                          int16_t lowAlarm, int16_t highAlarm);
    
    // Get web server instance
    WebServer* getWebServer() { return server; }
    
    // Check if portal is active
    bool isPortalActive() { return portalActive; }
    
    // Get configuration values
    String getWifiSSID() { return conf("st_ssid"); }
    String getWifiPassword() { return conf("st_pass"); }
    String getHostname() { return conf("host_name"); }
    uint16_t getDeviceId() { return conf("device_id").toInt(); }
    uint16_t getMeasurementPeriod() { return conf("measurement_period").toInt(); }
    bool isModbusEnabled() { return conf("modbus_enabled").toInt() == 1; }
    uint8_t getModbusAddress() { return conf("modbus_address").toInt(); }
    uint32_t getModbusBaudRate() { return conf("modbus_baud_rate").toInt(); }
    uint8_t getRxPin() { return conf("rs485_rx_pin").toInt(); }
    uint8_t getTxPin() { return conf("rs485_tx_pin").toInt(); }
    //uint8_t getDePin() { return conf("rs485_de_pin").toInt(); }
    //uint8_t getOneWirePin() { return conf("onewire_pin").toInt(); }
    bool getAutoDiscover() { return conf("auto_discover").toInt() == 1; }
    
    // Reset min/max values
    void resetMinMaxValues();
    //void updateSensorInConfig(Sensor* sensor);
    // Remove:
// void saveSensorConfig();
// void loadSensorConfig();
// bool addSensorToConfig(...);
// bool removeSensorFromConfig(...);
// bool updateSensorInConfig(...);
// void updateSensorInConfig(Sensor* sensor);

    // Add:
    void savePointsConfig();
    void loadPointsConfig();
    bool updatePointInConfig(uint8_t address, const String& name, int16_t lowAlarm, int16_t highAlarm,
                            const String& ds18b20RomString = "", int pt1000ChipSelect = -1);
    
    void saveAlarmsConfig();
    void loadAlarmsConfig();

    CSVConfigManager& getCSVManager() { return csvManager; }
};

// Initialize static member
//ConfigManager* ConfigManager::instance = nullptr;

#endif // CONFIG_MANAGER_H
