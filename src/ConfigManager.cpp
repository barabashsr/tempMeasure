/**
 * @file ConfigManager.cpp
 * @brief Implementation of web-based configuration management system
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements ConfigManager singleton for handling system configuration
 *          through web interface, including WiFi settings, device parameters,
 *          and temperature measurement point configuration.
 * 
 * @section dependencies Dependencies
 * - ConfigManager.h for class definition
 * - ConfigAssist for web configuration interface
 * - ArduinoJson for JSON processing
 * - LittleFS for configuration persistence
 * 
 * @section features Features
 * - Web-based configuration interface
 * - Persistent storage of settings
 * - Measurement point configuration
 * - WiFi credentials management
 * - Alarm threshold configuration
 */

#include "ConfigManager.h"
#include <ArduinoJson.h>
#include <time.h>


ConfigManager* ConfigManager::instance = nullptr;

    //       label: WiFi SSID
    //       default: Tenda_B3E6F0_EXT
    //   - st_pass:
    //       label: WiFi Password
    //       default: a111222333

    //       - st_ssid:
    //       label: WiFi SSID
    //       default: Polza
    //   - st_pass:
    //       label: WiFi Password
    //       default: 1122334455667788

    // - st_ssid:
    //       label: WiFi SSID
    //       default: RT-WiFi-D58A
    //   - st_pass:
    //       label: WiFi Password
    //       default: QpPuggeV

    //       - st_ssid:
    //       label: WiFi SSID
    //       default: Beeline_5G_F13F37
    //   - st_pass:
    //       label: WiFi Password
    //       default: 1122334455667788


    //       - st_ssid:
    //       label: WiFi SSID
    //       default: Beeline_2G_F13F37
    //   - st_pass:
    //       label: WiFi Password
    //       default: 1122334455667788

// YAML configuration definition
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
    Wifi settings:
      - st_ssid:
          label: WiFi SSID
          default: Tenda_B3E6F0
      - st_pass:
          label: WiFi Password
          default: a111222333
      - host_name:
          label: Device Hostname
          default: 'temp-monitor-{mac}'
    
    Device settings:
      - device_id:
          label: Device ID
          type: number
          min: 1
          max: 9999
          default: 1000
      - firmware_version:
          label: Firmware Version
          default: '1.0'
          readonly: true
      - measurement_period:
          label: Measurement Period (seconds)
          type: number
          min: 1
          max: 3600
          default: 10
    
    Alarm Acknowledged Delays:
      - ack_delay_critical:
          label: Critical Alarm Acknowledged Delay (minutes)
          type: number
          min: 1
          max: 1440
          default: 5
      - ack_delay_high:
          label: High Priority Alarm Acknowledged Delay (minutes)
          type: number
          min: 1
          max: 1440
          default: 10
      - ack_delay_medium:
          label: Medium Priority Alarm Acknowledged Delay (minutes)
          type: number
          min: 1
          max: 1440
          default: 15
      - ack_delay_low:
          label: Low Priority Alarm Acknowledged Delay (minutes)
          type: number
          min: 1
          max: 1440
          default: 30
    
    Modbus settings:
      - modbus_enabled:
          label: Enable Modbus RTU
          checked: true
      - modbus_address:
          label: Modbus Device Address
          type: number
          min: 1
          max: 247
          default: 1
      - modbus_baud_rate:
          label: Baud Rate
          options: '4800', '9600', '19200', '38400', '57600', '115200'
          default: '9600'
    )~";

ConfigManager::ConfigManager(TemperatureController& tempController)
    : conf("/config.ini", VARIABLES_DEF_YAML),
      controller(tempController), 
      csvManager(controller),
      settingsCSVManager(conf),
      portalActive(false) {
    
    instance = this;
    server = new WebServer(80);
    confHelper = new ConfigAssistHelper(conf);
}

ConfigManager::~ConfigManager() {
    if (server) {
        delete server;
    }
    
    if (confHelper) {
        delete confHelper;
    }
}

bool ConfigManager::begin() {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return false;
    }
    
    // Set callback function for configuration changes
    conf.setRemotUpdateCallback(onConfigChanged);
    
    // IMPORTANT: Register custom routes BEFORE ConfigAssist setup

    if(server) {
        Serial.println("!!!!!!!!!!!!!SERVER STARTED!!!!!!!!!!!!");
    } else {
        Serial.println("!!!!!!!!!!!!!SERVER FAILED!!!!!!!!!!!!");
    }


    basicAPI();
    sensorAPI();
    csvImportExportAPI();
    pointsAPI();
    alarmsAPI();
    logsAPI();
    downloadAPI();
    

    

   




    

    

    
    

    





    


    
    

    


    

 // WiFi setup logging
 bool startAP = true;
 if (conf("st_ssid") != "" && conf("st_pass") != "") {
     LoggerManager::info("CONFIG", "Attempting WiFi connection to: " + String(conf("st_ssid")));
     if (connectWiFi(10000)) {
         startAP = false;
         IPAddress ip = WiFi.localIP();
         LoggerManager::info("CONFIG", "WiFi connected successfully - IP: " + (ip ? ip.toString() : "0.0.0.0"));
     } else {
         LoggerManager::warning("CONFIG", "WiFi connection failed, starting AP mode");
     }
 } else {
     LoggerManager::info("CONFIG", "No WiFi credentials configured, starting AP mode");
 }
    
    // Setup WiFi
    // bool startAP = true;
    // if (conf("st_ssid") != "" && conf("st_pass") != "") {
    //     // Try to connect to WiFi if credentials are available
    //     if (connectWiFi(10000)) {
    //         startAP = false;
    //     }
    // }
    
    // Setup ConfigAssist with web server AFTER registering custom routes
    conf.setup(*server, startAP);
    
    // Start the web server
    server->begin();
    
    // Load sensor configuration
    //loadSensorConfig();
    loadPointsConfig();
    // loadAlarmsConfig(); // Now handled within loadPointsConfig()
    Serial.println("CM.begin(): Sensor data loaded:");
    //Serial.println(controller.getSensorsJson());
    
    // Apply configuration to controller
    controller.setDeviceId(getDeviceId());
    LoggerManager::info("CONFIG", "Device ID set to: " + String(getDeviceId()));
    
    controller.setMeasurementPeriod(getMeasurementPeriod());
    LoggerManager::info("CONFIG", "Measurement period set to: " + String(getMeasurementPeriod()) + " seconds");

    // Load acknowledged delays
    controller.setAcknowledgedDelayCritical(getAcknowledgedDelayCritical() * 60 * 1000);
    controller.setAcknowledgedDelayHigh(getAcknowledgedDelayHigh() * 60 * 1000);
    controller.setAcknowledgedDelayMedium(getAcknowledgedDelayMedium() * 60 * 1000);
    controller.setAcknowledgedDelayLow(getAcknowledgedDelayLow() * 60 * 1000);
    LoggerManager::info("CONFIG", "Acknowledged delays configured");
    
    LoggerManager::info("CONFIG", "ConfigManager initialization completed successfully");
    
    return true;
}

void ConfigManager::update() {
    // Handle client requests
    server->handleClient();
}

bool ConfigManager::connectWiFi(int timeoutMs) {
    // Use ConfigAssistHelper to connect to WiFi
    bool connected = confHelper->connectToNetwork(timeoutMs, -1);
    
    if (connected) {
        Serial.print("Connected to WiFi. IP: ");
        Serial.println(WiFi.localIP().toString());
    } else {
        Serial.println("Failed to connect to WiFi");
    }
    
    return connected;
}

// Update the onConfigChanged method in ConfigManager.cpp
void ConfigManager::onConfigChanged(String key) {
    if (instance == nullptr) return;
    
    Serial.print("Config changed: ");
    Serial.print(key);
    Serial.print(" = ");
    Serial.println(instance->conf(key));
    
    if (key == "device_id") {
        instance->controller.setDeviceId(instance->conf(key).toInt());
    } else if (key == "measurement_period") {
        instance->controller.setMeasurementPeriod(instance->conf(key).toInt());
    } else if (key == "reset_min_max") {
        instance->resetMinMaxValues();
    } else if (key == "ack_delay_critical") {
        unsigned long delayMs = instance->conf(key).toInt() * 60 * 1000; // Convert minutes to milliseconds
        instance->controller.setAcknowledgedDelayCritical(delayMs);
        Serial.printf("Set critical acknowledged delay to %lu ms (%d minutes)\n", delayMs, instance->conf(key).toInt());
    } else if (key == "ack_delay_high") {
        unsigned long delayMs = instance->conf(key).toInt() * 60 * 1000;
        instance->controller.setAcknowledgedDelayHigh(delayMs);
        Serial.printf("Set high acknowledged delay to %lu ms (%d minutes)\n", delayMs, instance->conf(key).toInt());
    } else if (key == "ack_delay_medium") {
        unsigned long delayMs = instance->conf(key).toInt() * 60 * 1000;
        instance->controller.setAcknowledgedDelayMedium(delayMs);
        Serial.printf("Set medium acknowledged delay to %lu ms (%d minutes)\n", delayMs, instance->conf(key).toInt());
    } else if (key == "ack_delay_low") {
        unsigned long delayMs = instance->conf(key).toInt() * 60 * 1000;
        instance->controller.setAcknowledgedDelayLow(delayMs);
        Serial.printf("Set low acknowledged delay to %lu ms (%d minutes)\n", delayMs, instance->conf(key).toInt());
    }
}



void ConfigManager::resetMinMaxValues() {
    controller.resetMinMaxValues();
}



// Save all measurement points and their bindings
void ConfigManager::savePointsConfig() {
    LoggerManager::info("CONFIG_SAVE", "Saving points configuration to /points2.ini");
    Serial.println("Save points to config ....");
    ConfigAssist pointsConf("/points2.ini", false);

    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) {
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (!point) continue;
        String key = "ds_" + String(point->getAddress());
        pointsConf[key + "_name"] = point->getName();
        pointsConf[key + "_low_alarm"] = String(point->getLowAlarmThreshold());
        pointsConf[key + "_high_alarm"] = String(point->getHighAlarmThreshold());
        // Hysteresis is stored per alarm, not per point
        // We'll use the first alarm's hysteresis as the default for all alarms of this point
        auto pointAlarms = controller.getAlarmsForPoint(point);
        int16_t hysteresis = 5; // Default
        if (!pointAlarms.empty()) {
            hysteresis = pointAlarms[0]->getHysteresis();
        }
        pointsConf[key + "_hysteresis"] = String(hysteresis);

        // Get alarm settings from TemperatureController
        for (auto alarm : pointAlarms) {
            String alarmKey = key;
            switch (alarm->getType()) {
                case AlarmType::LOW_TEMPERATURE:
                    alarmKey += "_low";
                    break;
                case AlarmType::HIGH_TEMPERATURE:
                    alarmKey += "_high";
                    break;
                case AlarmType::SENSOR_ERROR:
                    alarmKey += "_error";
                    break;
            }
            pointsConf[alarmKey + "_enable"] = String(alarm->isEnabled() ? "true" : "false");
            pointsConf[alarmKey + "_priority"] = String(static_cast<int>(alarm->getPriority()));
        }

        Sensor* bound = point->getBoundSensor();
        if (bound && bound->getType() == SensorType::DS18B20) {
            pointsConf[key + "_sensor_rom"] = bound->getDS18B20RomString();
            pointsConf[key + "_sensor_bus"] = controller.getSensorBus(bound);
        } else {
            pointsConf[key + "_sensor_rom"] = "";
            pointsConf[key + "_sensor_bus"] = "";
        }
    }

    // PT1000 points
    for (uint8_t i = 0; i < 10; ++i) {
        MeasurementPoint* point = controller.getPT1000Point(i);
        if (!point) continue;
        String key = "pt_" + String(point->getAddress());
        pointsConf[key + "_name"] = point->getName();
        pointsConf[key + "_low_alarm"] = String(point->getLowAlarmThreshold());
        pointsConf[key + "_high_alarm"] = String(point->getHighAlarmThreshold());
        // Hysteresis is stored per alarm, not per point
        // We'll use the first alarm's hysteresis as the default for all alarms of this point
        auto pointAlarms = controller.getAlarmsForPoint(point);
        int16_t hysteresis = 5; // Default
        if (!pointAlarms.empty()) {
            hysteresis = pointAlarms[0]->getHysteresis();
        }
        pointsConf[key + "_hysteresis"] = String(hysteresis);

        // Get alarm settings from TemperatureController
        for (auto alarm : pointAlarms) {
            String alarmKey = key;
            switch (alarm->getType()) {
                case AlarmType::LOW_TEMPERATURE:
                    alarmKey += "_low";
                    break;
                case AlarmType::HIGH_TEMPERATURE:
                    alarmKey += "_high";
                    break;
                case AlarmType::SENSOR_ERROR:
                    alarmKey += "_error";
                    break;
            }
            pointsConf[alarmKey + "_enable"] = String(alarm->isEnabled() ? "true" : "false");
            pointsConf[alarmKey + "_priority"] = String(static_cast<int>(alarm->getPriority()));
        }

        Sensor* bound = point->getBoundSensor();
        if (bound && bound->getType() == SensorType::PT1000) {
            pointsConf[key + "_sensor_cs"] = String(bound->getPT1000ChipSelectPin());
        } else {
            pointsConf[key + "_sensor_cs"] = "";
        }
    }
    pointsConf.saveConfigFile();
}

// // Load all measurement points and their bindings
// void ConfigManager::loadPointsConfig() {
//     ConfigAssist pointsConf("/points2.ini", false);

//     // DS18B20 points
//     for (uint8_t i = 0; i < 50; ++i) {
//         String key = "ds_" + String(i);
//         MeasurementPoint* point = controller.getDS18B20Point(i);
//         if (!point) continue;
//         point->setName(pointsConf(key + "_name"));
//         point->setLowAlarmThreshold(pointsConf(key + "_low_alarm").toInt());
//         point->setHighAlarmThreshold(pointsConf(key + "_high_alarm").toInt());
//         String rom = pointsConf(key + "_sensor_rom");
//         if (rom.length() == 16) {
//             Serial.println("ROM from file: " + rom);
//             controller.bindSensorToPointByRom(rom, i);
//         } else {
//             controller.unbindSensorFromPoint(i);
//         }
//     }

//     // PT1000 points
//     for (uint8_t i = 0; i < 10; ++i) {
//         uint8_t address = 50 + i;
//         String key = "pt_" + String(address);
//         MeasurementPoint* point = controller.getPT1000Point(i);
//         if (!point) continue;
//         point->setName(pointsConf(key + "_name"));
//         point->setLowAlarmThreshold(pointsConf(key + "_low_alarm").toInt());
//         point->setHighAlarmThreshold(pointsConf(key + "_high_alarm").toInt());
//         int cs = pointsConf(key + "_sensor_cs").toInt();
//         if (cs > 0) {
//             controller.bindSensorToPointByChipSelect(cs, address);
//         } else {
//             controller.unbindSensorFromPoint(address);
//         }
//     }
// }

void ConfigManager::loadPointsConfig() {
    ConfigAssist pointsConf("/points2.ini", false);
    LoggerManager::info("CONFIG_LOAD", "Loading points configuration from /points2.ini");

    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) {
        String key = "ds_" + String(i);
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (!point) continue;
        point->setName(pointsConf(key + "_name"));
        point->setLowAlarmThreshold(pointsConf(key + "_low_alarm").toInt());
        point->setHighAlarmThreshold(pointsConf(key + "_high_alarm").toInt());
        
        // Load hysteresis - will be applied to alarms later
        String hysteresisStr = pointsConf(key + "_hysteresis");
        int16_t hysteresis = 5; // Default
        if (!hysteresisStr.isEmpty()) {
            hysteresis = hysteresisStr.toInt();
        }
        
        // Load alarm settings - defer to after sensor binding
        // so we can auto-enable sensor error alarm if bound
        uint8_t bus = pointsConf(key + "_sensor_bus").toInt();
        String rom = pointsConf(key + "_sensor_rom");
        if (rom.length() == 16) {
            // Ensure the sensor exists and is initialized before binding
            Sensor* sensor = controller.findSensorByRom(rom);
            if (!sensor) {
                uint8_t romArr[8];
                for (int j = 0; j < 8; ++j)
                    romArr[j] = strtol(rom.substring(j*2, j*2+2).c_str(), nullptr, 16);
                String sensorName = "DS18B20_" + rom;
                sensor = new Sensor(SensorType::DS18B20, 0, sensorName);
                sensor->setupDS18B20(controller.getOneWirePin(bus), romArr);
                if (!sensor->initialize()) {
                    //sensor->setErrorStatus(0x01); // Mark as error (not connected)
                }
                controller.addSensor(sensor);
            }
            
            controller.bindSensorToPointByRom(rom, i);
        } else {
            controller.unbindSensorFromPoint(i);
        }
    }

    // PT1000 points
    for (uint8_t i = 0; i < 10; ++i) {
        uint8_t address = 50 + i;
        String key = "pt_" + String(address);
        MeasurementPoint* point = controller.getPT1000Point(i);
        if (!point) continue;
        point->setName(pointsConf(key + "_name"));
        point->setLowAlarmThreshold(pointsConf(key + "_low_alarm").toInt());
        point->setHighAlarmThreshold(pointsConf(key + "_high_alarm").toInt());
        
        // Load hysteresis - will be applied to alarms later
        String hysteresisStr = pointsConf(key + "_hysteresis");
        int16_t hysteresis = 5; // Default
        if (!hysteresisStr.isEmpty()) {
            hysteresis = hysteresisStr.toInt();
        }
        
        int cs = pointsConf(key + "_sensor_cs").toInt();
        if (cs > 0) {
            // Ensure the sensor exists and is initialized before binding
            Sensor* sensor = controller.findSensorByChipSelect(cs);
            if (!sensor) {
                String sensorName = "PT1000_CS" + String(cs);
                sensor = new Sensor(SensorType::PT1000, 0, sensorName);
                sensor->setupPT1000(cs, i);
                if (!sensor->initialize()) {
                    //sensor->setErrorStatus(0x01); // Mark as error (not connected)
                }
                controller.addSensor(sensor);
            }
            controller.bindSensorToPointByChipSelect(cs, address);
        } else {
            controller.unbindSensorFromPoint(address);
        }
    }
    
    // Load alarm configurations after sensor binding
    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) {
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (!point) continue;
        
        String key = "ds_" + String(i);
        
        // Get hysteresis for this point
        String hysteresisStr = pointsConf(key + "_hysteresis");
        int16_t pointHysteresis = 5; // Default
        if (!hysteresisStr.isEmpty()) {
            pointHysteresis = hysteresisStr.toInt();
        }
        
        // Create all 3 alarms for this point if they don't exist
        controller.ensureAlarmsForPoint(point);
        
        // Get alarms for this point
        auto alarms = controller.getAlarmsForPoint(point);
        for (auto alarm : alarms) {
            // Set hysteresis for all alarms of this point
            alarm->setHysteresis(pointHysteresis);
            String alarmKey = key;
            switch (alarm->getType()) {
                case AlarmType::LOW_TEMPERATURE:
                    alarmKey += "_low";
                    break;
                case AlarmType::HIGH_TEMPERATURE:
                    alarmKey += "_high";
                    break;
                case AlarmType::SENSOR_ERROR:
                    alarmKey += "_error";
                    break;
            }
            
            // Load enable state
            String enableStr = pointsConf(alarmKey + "_enable");
            if (!enableStr.isEmpty()) {
                alarm->setEnabled(enableStr == "true");
            } else {
                // Default: disabled for temperature alarms
                if (alarm->getType() != AlarmType::SENSOR_ERROR) {
                    alarm->setEnabled(false);
                } else {
                    // Auto-enable sensor error if sensor bound
                    alarm->setEnabled(point->getBoundSensor() != nullptr);
                }
            }
            
            // Load priority
            String priorityStr = pointsConf(alarmKey + "_priority");
            if (!priorityStr.isEmpty()) {
                alarm->setPriority(static_cast<AlarmPriority>(priorityStr.toInt()));
            } else {
                // Default priorities
                if (alarm->getType() == AlarmType::SENSOR_ERROR) {
                    alarm->setPriority(AlarmPriority::PRIORITY_HIGH);
                } else {
                    alarm->setPriority(AlarmPriority::PRIORITY_MEDIUM);
                }
            }
        }
    }
    
    // PT1000 points
    for (uint8_t i = 0; i < 10; ++i) {
        uint8_t address = 50 + i;
        MeasurementPoint* point = controller.getPT1000Point(i);
        if (!point) continue;
        
        String key = "pt_" + String(address);
        
        // Get hysteresis for this point
        String hysteresisStr = pointsConf(key + "_hysteresis");
        int16_t pointHysteresis = 5; // Default
        if (!hysteresisStr.isEmpty()) {
            pointHysteresis = hysteresisStr.toInt();
        }
        
        // Create all 3 alarms for this point if they don't exist
        controller.ensureAlarmsForPoint(point);
        
        // Get alarms for this point
        auto alarms = controller.getAlarmsForPoint(point);
        for (auto alarm : alarms) {
            // Set hysteresis for all alarms of this point
            alarm->setHysteresis(pointHysteresis);
            String alarmKey = key;
            switch (alarm->getType()) {
                case AlarmType::LOW_TEMPERATURE:
                    alarmKey += "_low";
                    break;
                case AlarmType::HIGH_TEMPERATURE:
                    alarmKey += "_high";
                    break;
                case AlarmType::SENSOR_ERROR:
                    alarmKey += "_error";
                    break;
            }
            
            // Load enable state
            String enableStr = pointsConf(alarmKey + "_enable");
            if (!enableStr.isEmpty()) {
                alarm->setEnabled(enableStr == "true");
            } else {
                // Default: disabled for temperature alarms
                if (alarm->getType() != AlarmType::SENSOR_ERROR) {
                    alarm->setEnabled(false);
                } else {
                    // Auto-enable sensor error if sensor bound
                    alarm->setEnabled(point->getBoundSensor() != nullptr);
                }
            }
            
            // Load priority
            String priorityStr = pointsConf(alarmKey + "_priority");
            if (!priorityStr.isEmpty()) {
                alarm->setPriority(static_cast<AlarmPriority>(priorityStr.toInt()));
            } else {
                // Default priorities
                if (alarm->getType() == AlarmType::SENSOR_ERROR) {
                    alarm->setPriority(AlarmPriority::PRIORITY_HIGH);
                } else {
                    alarm->setPriority(AlarmPriority::PRIORITY_MEDIUM);
                }
            }
        }
    }
    
    controller.applyConfigToRegisterMap();
}


// Update a measurement point and its binding in config
bool ConfigManager::updatePointInConfig(uint8_t address, const String& name, int16_t lowAlarm, int16_t highAlarm,
                                        const String& ds18b20RomString, int pt1000ChipSelect) {
    MeasurementPoint* point = controller.getMeasurementPoint(address);
    if (!point) return false;
    point->setName(name);
    point->setLowAlarmThreshold(lowAlarm);
    point->setHighAlarmThreshold(highAlarm);
    if (!ds18b20RomString.isEmpty()) {
        controller.bindSensorToPointByRom(ds18b20RomString, address);
    } else if (pt1000ChipSelect >= 0) {
        controller.bindSensorToPointByChipSelect(pt1000ChipSelect, address);
    } else {
        controller.unbindSensorFromPoint(address);
    }
    savePointsConfig();
    return true;
}


void ConfigManager::saveAlarmsConfig() {
    Serial.println("Save alarms to config....");
    ConfigAssist alarmsConf("/alarms.ini", false);
    
    // Clear existing entries by setting them to empty strings
    // ConfigAssist will treat empty strings as non-existent
    for (int i = 0; i < 1000; i++) { // Check reasonable number of possible alarms
        String alarmKey = "alarm" + String(i);
        if (alarmsConf.exists(alarmKey + "_type")) {
            alarmsConf[alarmKey + "_type"] = "";
            alarmsConf[alarmKey + "_priority"] = "";
            alarmsConf[alarmKey + "_point"] = "";
            alarmsConf[alarmKey + "_enabled"] = "";
        } else {
            break; // No more entries to clear
        }
    }
    
    // Save current alarms
    int alarmIndex = 0;
    for (int i = 0; i < controller.getAlarmCount(); ++i) {
        Alarm* alarm = controller.getAlarmByIndex(i);
        if (!alarm) continue;
        
        String alarmKey = "alarm" + String(alarmIndex++);
        alarmsConf[alarmKey + "_type"] = String(static_cast<int>(alarm->getType()));
        alarmsConf[alarmKey + "_priority"] = String(static_cast<int>(alarm->getPriority()));
        alarmsConf[alarmKey + "_point"] = String(alarm->getPointAddress());
        alarmsConf[alarmKey + "_enabled"] = alarm->isEnabled() ? "1" : "0";
        alarmsConf[alarmKey + "_hysteresis"] = String(alarm->getHysteresis());
    }
    
    alarmsConf.saveConfigFile();
    Serial.printf("Saved %d alarms to config\n", alarmIndex);
}




void ConfigManager::loadAlarmsConfig() {
    Serial.println("Loading alarms configuration...");
    Serial.println("WARNING: loadAlarmsConfig() is deprecated - alarms are now managed through loadPointsConfig()");
    return; // Exit early to prevent deleting alarms
    
    /* Deprecated - alarms are now managed through points configuration
    ConfigAssist alarmsConf("/alarms.ini", false);
    
    // Clear existing configured alarms first
    for (auto alarm : controller.getConfiguredAlarms()) {
        delete alarm;
    }
    controller.clearConfiguredAlarms();
    
    int loadedCount = 0;
    int alarmIndex = 0;
    
    // Load alarms sequentially
    while (true) {
        String alarmKey = "alarm" + String(alarmIndex++);
        
        // Check if all required keys exist
        String typeKey = alarmKey + "_type";
        String priorityKey = alarmKey + "_priority";
        String pointKey = alarmKey + "_point";
        String enabledKey = alarmKey + "_enabled";
        int16_t hysteresis = alarmsConf(alarmKey + "_hysteresis").toInt();
        if (hysteresis == 0) hysteresis = 1; // Default to 1 if not set
        
        
        if (!alarmsConf.exists(typeKey)) {
            break; // No more alarms to load
        }
        
        // Get values and validate they're not empty
        String typeStr = alarmsConf(typeKey);
        String priorityStr = alarmsConf(priorityKey);
        String pointStr = alarmsConf(pointKey);
        String enabledStr = alarmsConf(enabledKey);
        
        // Skip empty entries (these were cleared)
        if (typeStr.isEmpty() || priorityStr.isEmpty() || 
            pointStr.isEmpty() || enabledStr.isEmpty()) {
            Serial.printf("Skipping empty alarm entry at index %d\n", alarmIndex - 1);
            continue;
        }
        
        // Convert and validate values
        int type = typeStr.toInt();
        int priority = priorityStr.toInt();
        int pointAddress = pointStr.toInt();
        bool enabled = (enabledStr == "1");
        
        // Validate enum ranges
        if (type < 0 || type > 3 || priority < 0 || priority > 3) {
            Serial.printf("Skipping alarm %d: invalid type (%d) or priority (%d)\n", 
                         alarmIndex - 1, type, priority);
            continue;
        }
        
        // Validate point address
        if (pointAddress < 0 || pointAddress >= 60) {
            Serial.printf("Skipping alarm %d: invalid point address (%d)\n", 
                         alarmIndex - 1, pointAddress);
            continue;
        }
        
        // Create alarm
        bool success = controller.addAlarm(
            static_cast<AlarmType>(type),
            pointAddress,
            static_cast<AlarmPriority>(priority)
        );
        
        if (success) {
            // Set enabled state
            Alarm* alarm = controller.findAlarm("alarm_" + String(pointAddress) + "_" + String(type));
            if (alarm) {
                alarm->setEnabled(enabled);
                alarm->setHysteresis(hysteresis);
                loadedCount++;
                Serial.printf("Loaded alarm: type=%d, priority=%d, point=%d, enabled=%s, hyst=%d\n",
                             type, priority, pointAddress, enabled ? "true" : "false", hysteresis);
            }
        }
    }
    
    Serial.printf("Loaded %d valid alarm configurations\n", loadedCount);
    */
}

void ConfigManager::basicAPI(){

    // Add route for the main page
    server->on("/dashboard.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/dashboard.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/dashboard.html", "r");
            server->streamFile(file, "text/html");
            file.close();
            Serial.println("SERVER: /dashboard.html");
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->send(200, "text/html", "<html><body><h1>Temperature Monitoring System</h1><p><a href='/cfg'>Configuration</a></p><p><a href='/sensors.html'>Sensors</a></p></body></html>");
        }
    });
    
    // Add route for the sensors page
    server->on("/sensors.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/sensors.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/sensors.html", "r");
            server->streamFile(file, "text/html");
            file.close();
            Serial.println("SERVER: /sensors.html");
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "Sensors page not found");
        }
    });
    server->on("/points.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/points.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/points.html", "r");
            server->streamFile(file, "text/html");
            Serial.println("SERVER: /points.html");
            file.close();
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "Points page not found");
        }
    });




    server->on("/alarms.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/alarms.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/alarms.html", "r");
            server->streamFile(file, "text/html");
            file.close();
            Serial.println("SERVER: /alarms.html");
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->send(200, "text/html", "<html><body><h1>Temperature Monitoring System</h1><p><a href='/cfg'>Configuration</a></p><p><a href='/sensors.html'>Sensors</a></p></body></html>");
        }
    });

    server->on("/alarm-history.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/alarm-history.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/alarm-history.html", "r");
            server->streamFile(file, "text/html");
            file.close();
            Serial.println("SERVER: /alarm-history.html");
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->send(200, "text/html", "<html><body><h1>Temperature Monitoring System</h1><p><a href='/cfg'>Configuration</a></p><p><a href='/sensors.html'>Sensors</a></p></body></html>");
        }
    });

    server->on("/event-logs.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/event-logs.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/event-logs.html", "r");
            server->streamFile(file, "text/html");
            Serial.println("SERVER: /event-logs.html");
            file.close();
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "event-logs page not found");
        }
    });

    server->on("/download-logs.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/download-logs.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/download-logs.html", "r");
            server->streamFile(file, "text/html");
            Serial.println("SERVER: /download-logs.html");
            file.close();
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "download-logs page not found");
        }
    });

    server->on("/alarm-config.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/alarm-config.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/alarm-config.html", "r");
            server->streamFile(file, "text/html");
            Serial.println("SERVER: /alarm-config.html");
            file.close();
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "alarm-config.html page not found");
        }
    });

    // Serve common CSS file
    server->on("/common.css", HTTP_GET, [this]() {
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "max-age=86400"); // Cache for 24 hours
        
        if (LittleFS.exists("/common.css")) {
            // Use LittleFS if file exists there
            server->sendHeader("Content-Type", "text/css");
            server->sendHeader("Connection", "close");
            File file = LittleFS.open("/common.css", "r");
            server->streamFile(file, "text/css");
            file.close();
        } else {
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "common.css not found");
        }
    });
    
    // Serve Chart.js library
    server->on("/chart.min.js", HTTP_GET, [this]() {
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "max-age=86400"); // Cache for 24 hours
        
        if (LittleFS.exists("/chart.min.js")) {
            server->sendHeader("Content-Type", "application/javascript");
            server->sendHeader("Connection", "close");
            File file = LittleFS.open("/chart.min.js", "r");
            server->streamFile(file, "application/javascript");
            file.close();
        } else {
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "chart.min.js not found");
        }
    });

    // Add CORS support for OPTIONS requests
    server->on("/api/sensors", HTTP_OPTIONS, [this]() {
        server->sendHeader("HTTP/1.1 204 No Content", "");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server->send(204);
    });

};
void ConfigManager::sensorAPI(){
    // API endpoints for sensor data
    server->on("/api/sensors", HTTP_GET, [this]() {
        server->sendHeader("HTTP/1.1 200 OK", "");
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Connection", "close");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", controller.getSensorsJson());
    });
    
    server->on("/api/status", HTTP_GET, [this]() {
        server->sendHeader("HTTP/1.1 200 OK", "");
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Connection", "close");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", controller.getSystemStatusJson());
    });
    
    server->on("/api/reset-minmax", HTTP_POST, [this]() {
        controller.resetMinMaxValues();
        server->sendHeader("HTTP/1.1 200 OK", "");
        server->sendHeader("Content-Type", "text/plain");
        server->sendHeader("Connection", "close");
        server->send(200, "text/plain", "Min/Max values reset");
    });
    
    // API endpoint for sensor discovery
    server->on("/api/discover", HTTP_POST, [this]() {
        bool discoveredDS = controller.discoverDS18B20Sensors();
        bool discoveredPT = controller.discoverPTSensors();
        bool discovered = discoveredDS || discoveredPT;
        
        if (discovered) {

            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(200, "text/plain", "Sensors discovered");
        } else {
            server->sendHeader("HTTP/1.1 404 Not Found", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(404, "text/plain", "No sensors found");
        }
    });

    // POST /api/sensor-bind
    server->on("/api/sensor-bind", HTTP_POST, [this]() {
        if (server->hasArg("plain")) {
            DynamicJsonDocument doc(256);
            
            DeserializationError err = deserializeJson(doc, server->arg("plain"));
            
            if (!err) {
                uint8_t pointAddress = doc["pointAddress"];
                if (doc.containsKey("romString")) {
                    Serial.println("ROM:\n" + doc.as<String>());
                    String rom = doc["romString"].as<String>();
                    if (controller.bindSensorToPointByRom(rom, pointAddress)) {
                        //Serial.println("Save points to config ROM\n");
                        savePointsConfig();
                        server->send(200, "text/plain", "Bound");
                        return;
                    }
                } else if (doc.containsKey("chipSelect")) {
                    int cs = doc["chipSelect"];
                    Serial.println("CS:\n" + doc.as<String>());
                    if (controller.bindSensorToPointByChipSelect(cs, pointAddress)) {
                        //Serial.println("Save points to config ROM\n");
                        savePointsConfig();
                        server->send(200, "text/plain", "Bound");
                        return;
                    }
                }
            }
        }
        server->send(400, "text/plain", "Bad Request");
    });

    // POST /api/sensor-unbind
    server->on("/api/sensor-unbind", HTTP_POST, [this]() {
        if (server->hasArg("plain")) {
            DynamicJsonDocument doc(128);
            DeserializationError err = deserializeJson(doc, server->arg("plain"));
            if (!err) {
                if (doc.containsKey("romString")) {
                    String rom = doc["romString"].as<String>();
                    // Find point bound to this ROM and unbind
                    for (uint8_t i = 0; i < 50; ++i) {
                        Sensor* bound = controller.getDS18B20Point(i)->getBoundSensor();
                        if (bound && bound->getDS18B20RomString() == rom) {
                            if(controller.unbindSensorFromPoint(i)){
                                savePointsConfig();
                            server->send(200, "text/plain", "Unbound");
                            return;

                            };
                            
                        }
                    }
                } else if (doc.containsKey("chipSelect")) {
                    int cs = doc["chipSelect"];
                    for (uint8_t i = 0; i < 10; ++i) {
                        Sensor* bound = controller.getPT1000Point(i)->getBoundSensor();
                        if (bound && bound->getPT1000ChipSelectPin() == cs) {
                            if(controller.unbindSensorFromPoint(50 + i)){
                                savePointsConfig();
                                server->send(200, "text/plain", "Unbound");
                                return;
                            };

                        }
                    }
                }
            }
        }
        server->send(400, "text/plain", "Bad Request");
    });



};
void ConfigManager::csvImportExportAPI(){

     // Combined points and alarms export
     server->on("/api/export/config", HTTP_GET, [this]() {
        String csv = csvManager.exportPointsWithAlarmsToCSV();
        server->sendHeader("Content-Type", "text/csv");
        server->sendHeader("Content-Disposition", "attachment; filename=config.csv");
        server->send(200, "text/csv", csv);
    });

    // Combined points and alarms import
    server->on("/api/import/config", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No CSV data provided\"}");
            return;
        }
        
        bool success = csvManager.importPointsWithAlarmsFromCSV(server->arg("plain"));
        if (success) {
            savePointsConfig();
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration imported successfully\"}");
        } else {
            String error = csvManager.getLastError();
            server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"" + error + "\"}");
        }
    });


    // Add these to your ConfigManager::begin() method after existing API endpoints

    // CSV Export endpoint
    server->on("/api/csv/export", HTTP_GET, [this]() {
        CSVConfigManager csvManager(controller);
        String csvData = csvManager.exportPointsWithAlarmsToCSV();
        
        if (csvData.length() > 0) {
            String filename = "temperature_config_" + String(millis()) + ".csv";
            server->sendHeader("Content-Type", "text/csv");
            server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            server->send(200, "text/csv", csvData);
        } else {
            server->send(500, "application/json", "{\"error\":\"Failed to generate CSV\"}");
        }
    });

    // CSV Import endpoint
   // In ConfigManager.cpp, update the CSV import endpoint:
    server->on("/api/csv/import", HTTP_POST, [this]() {
        // This will be called after file upload is complete
    }, [this]() {
        // Handle file upload
        HTTPUpload& upload = server->upload();
        static String csvContent;
        
        if (upload.status == UPLOAD_FILE_START) {
            csvContent = "";
            LoggerManager::info("CONFIG_IMPORT", 
                "CSV upload started - filename: " + String(upload.filename.c_str()));
            Serial.printf("Upload Start: %s\n", upload.filename.c_str());
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            csvContent += String((char*)upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
            Serial.printf("Upload End: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
            LoggerManager::info("CONFIG_IMPORT", 
                "CSV upload completed - size: " + String(upload.totalSize) + " bytes");
            
            // Process the uploaded CSV
            if (csvManager.importPointsWithAlarmsFromCSV(csvContent)) {
                saveAlarmsConfig();
                savePointsConfig();
                LoggerManager::info("CONFIG_IMPORT", "CSV import successful");
                server->send(200, "application/json", "{\"success\":true}");
            } else {
                String error = csvManager.getLastError();
                LoggerManager::error("CONFIG_IMPORT", "CSV import failed: " + error);
                server->send(400, "application/json", "{\"success\":false,\"error\":\"" + error + "\"}");
            }
            csvContent = "";
        }
    });
    // Settings CSV Export endpoint
    server->on("/api/settings/export", HTTP_GET, [this]() {
        String csvData = settingsCSVManager.exportSettingsToCSV();
        
        if (csvData.length() > 0) {
            String filename = "device_settings_" + String(millis()) + ".csv";
            server->sendHeader("Content-Type", "text/csv");
            server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            server->send(200, "text/csv", csvData);
        } else {
            server->send(500, "application/json", "{\"error\":\"Failed to generate settings CSV\"}");
        }
    });

    // Settings CSV Import endpoint
    server->on("/api/settings/import", HTTP_POST, [this]() {
        // This will be called after file upload is complete
    }, [this]() {
        // Handle file upload
        HTTPUpload& upload = server->upload();
        static String csvContent;
        
        if (upload.status == UPLOAD_FILE_START) {
            csvContent = "";
            Serial.printf("Settings Upload Start: %s\n", upload.filename.c_str());
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            csvContent += String((char*)upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
            Serial.printf("Settings Upload End: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
            
            // Process the uploaded CSV
            if (settingsCSVManager.importSettingsFromCSV(csvContent)) {
                // Save configuration after successful import
                conf.saveConfigFile();
                server->send(200, "application/json", "{\"success\":true,\"message\":\"Settings imported successfully. Device will restart.\"}");
                
                // Restart device to apply new settings
                delay(1000);
                ESP.restart();
            } else {
                String error = settingsCSVManager.getLastError();
                server->send(400, "application/json", "{\"success\":false,\"error\":\"" + error + "\"}");
            }
            csvContent = "";
        }
    });

};
void ConfigManager::pointsAPI(){
    // GET points
    server->on("/api/points", HTTP_GET, [this]() {
        server->sendHeader("Content-Type", "application/json");
        server->send(200, "application/json", controller.getPointsJson());
    });

    // PUT point update
    server->on("/api/points", HTTP_PUT, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        Serial.println("/api/points HTTP_PUT:" + doc.as<String>());
        uint8_t address = doc["address"];
        String name = doc["name"].as<String>();
        int16_t low = doc["lowAlarmThreshold"];
        int16_t high = doc["highAlarmThreshold"];


        MeasurementPoint* point = controller.getMeasurementPoint(address);
        if (!point) {
            server->send(404, "application/json", "{\"error\":\"Point not found\"}");
            return;
        }
        point->setName(name);
        point->setLowAlarmThreshold(low);
        point->setHighAlarmThreshold(high);
        Serial.printf("Point: %s. LAS: %d, HAS: %d\n Delay....\n", point->getName(), point->getLowAlarmThreshold(), point->getHighAlarmThreshold());
        delay(5000);
        controller.applyConfigToRegisterMap();
        // Save to config if needed
        savePointsConfig(); // implement this to persist changes

        server->send(200, "application/json", "{\"success\":true}");
    });

};
void ConfigManager::alarmsAPI(){

    // Get alarms configuration
    server->on("/api/alarms", HTTP_GET, [this]() {
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", controller.getAlarmsJson());
    });

    // Add/Update alarm configuration
    server->on("/api/alarms", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        AlarmType type = static_cast<AlarmType>(doc["type"].as<int>());
        uint8_t pointAddress = doc["pointAddress"].as<int>();
        AlarmPriority priority = static_cast<AlarmPriority>(doc["priority"].as<int>());
        
        bool success = controller.addAlarm(type, pointAddress, priority);
        if (success) {
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server->send(400, "application/json", "{\"error\":\"Failed to add alarm\"}");
        }
    });

    // Delete alarm configuration
    server->on("/api/alarms", HTTP_DELETE, [this]() {
        if (!server->hasArg("configKey")) {
            server->send(400, "application/json", "{\"error\":\"No configKey provided\"}");
            return;
        }
        
        String configKey = server->arg("configKey");
        bool success = controller.removeAlarm(configKey);
        if (success) {
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"deleted\"}");
        } else {
            server->send(404, "application/json", "{\"error\":\"Alarm not found\"}");
        }
    });

    // Add these endpoints to your setupWebServer() method in ConfigManager.cpp

    // Get alarms configuration
    server->on("/api/alarms", HTTP_GET, [this]() {
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", controller.getAlarmsJson());
    });

    // Add/Update alarm configuration
    server->on("/api/alarms", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        AlarmType type = static_cast<AlarmType>(doc["type"].as<int>());
        uint8_t pointAddress = doc["pointAddress"].as<int>();
        AlarmPriority priority = static_cast<AlarmPriority>(doc["priority"].as<int>());
        
        bool success = controller.addAlarm(type, pointAddress, priority);
        if (success) {
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server->send(400, "application/json", "{\"error\":\"Failed to add alarm\"}");
        }
    });

    // Update alarm configuration
    server->on("/api/alarms", HTTP_PUT, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        String configKey = doc["configKey"].as<String>();
        AlarmPriority priority = static_cast<AlarmPriority>(doc["priority"].as<int>());
        bool enabled = doc["enabled"].as<bool>();
        
        bool success = controller.updateAlarm(configKey, priority, enabled);
        if (success) {
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"updated\"}");
        } else {
            server->send(404, "application/json", "{\"error\":\"Alarm not found\"}");
        }
    });

    // Delete alarm configuration
    server->on("/api/alarms", HTTP_DELETE, [this]() {
        if (!server->hasArg("configKey")) {
            server->send(400, "application/json", "{\"error\":\"No configKey provided\"}");
            return;
        }
        
        String configKey = server->arg("configKey");
        bool success = controller.removeAlarm(configKey);
        if (success) {
            saveAlarmsConfig();
            server->send(200, "application/json", "{\"status\":\"deleted\"}");
        } else {
            server->send(404, "application/json", "{\"error\":\"Alarm not found\"}");
        }
    });

    // Acknowledge specific alarm
    server->on("/api/alarms/acknowledge", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }

        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        String configKey = doc["configKey"].as<String>();
        
        // Find alarm in configured alarms
        Alarm* alarm = controller.findAlarm(configKey);
        if (!alarm) {
            server->send(404, "application/json", "{\"error\":\"Alarm not found\"}");
            return;
        }

        // Find corresponding active alarm and acknowledge it
        bool acknowledged = false;
        for (auto activeAlarm : controller.getActiveAlarms()) {
            if (activeAlarm->getSource() == alarm->getSource() && 
                activeAlarm->getType() == alarm->getType()) {
                activeAlarm->acknowledge();
                acknowledged = true;
                Serial.printf("Acknowledged alarm: %s for point %d\n", 
                            activeAlarm->getTypeString().c_str(),
                            activeAlarm->getSource()->getAddress());
                break;
            }
        }

        if (acknowledged) {
            server->send(200, "application/json", "{\"status\":\"acknowledged\"}");
        } else {
            server->send(404, "application/json", "{\"error\":\"No active alarm found to acknowledge\"}");
        }
    });

    // Acknowledge all active alarms
    server->on("/api/alarms/acknowledge-all", HTTP_POST, [this]() {
        std::vector<Alarm*> activeAlarms = controller.getActiveAlarms();
        int acknowledgedCount = 0;
        
        for (auto alarm : activeAlarms) {
            if (!alarm->isAcknowledged()) {
                alarm->acknowledge();
                acknowledgedCount++;
                Serial.printf("Acknowledged alarm: %s for point %d\n", 
                            alarm->getTypeString().c_str(),
                            alarm->getSource() ? alarm->getSource()->getAddress() : -1);
            }
        }
        
        DynamicJsonDocument response(256);
        response["status"] = "success";
        response["acknowledgedCount"] = acknowledgedCount;
        response["message"] = String(acknowledgedCount) + " alarms acknowledged";
        
        String output;
        serializeJson(response, output);
        server->send(200, "application/json", output);
    });

    // Clear resolved alarms
    server->on("/api/alarms/clear-resolved", HTTP_POST, [this]() {
        std::vector<Alarm*> configuredAlarms = controller.getConfiguredAlarms();
        int clearedCount = 0;
        
        // Remove resolved alarms from configured alarms
        for (auto it = configuredAlarms.begin(); it != configuredAlarms.end();) {
            if ((*it)->isResolved()) {
                String configKey = (*it)->getConfigKey();
                bool removed = controller.removeAlarm(configKey);
                if (removed) {
                    clearedCount++;
                    Serial.printf("Cleared resolved alarm: %s\n", configKey.c_str());
                }
                // Note: iterator is handled by removeAlarm method
                it = configuredAlarms.begin(); // Restart iteration after removal
            } else {
                ++it;
            }
        }
        
        // Save configuration after clearing
        if (clearedCount > 0) {
            saveAlarmsConfig();
        }
        
        DynamicJsonDocument response(256);
        response["status"] = "success";
        response["clearedCount"] = clearedCount;
        response["message"] = String(clearedCount) + " resolved alarms cleared";
        
        String output;
        serializeJson(response, output);
        server->send(200, "application/json", output);
    });

    // Get active alarms only (for dashboard/monitoring)
    server->on("/api/alarms/active", HTTP_GET, [this]() {
        DynamicJsonDocument doc(4096);
        JsonArray alarmArray = doc.createNestedArray("alarms");
        
        for (auto alarm : controller.getActiveAlarms()) {
            JsonObject obj = alarmArray.createNestedObject();
            obj["type"] = static_cast<int>(alarm->getType());
            obj["stage"] = static_cast<int>(alarm->getStage());
            obj["priority"] = static_cast<int>(alarm->getPriority());
            obj["timestamp"] = alarm->getTimestamp();
            obj["acknowledgedTime"] = alarm->getAcknowledgedTime();
            obj["message"] = alarm->getMessage();
            obj["isActive"] = alarm->isActive();
            obj["isAcknowledged"] = alarm->isAcknowledged();
            
            if (alarm->getSource()) {
                obj["pointAddress"] = alarm->getSource()->getAddress();
                obj["pointName"] = alarm->getSource()->getName();
                obj["currentTemp"] = alarm->getSource()->getCurrentTemp();
            }
        }
        
        String output;
        serializeJson(doc, output);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });

    // Get alarm statistics
    server->on("/api/alarms/stats", HTTP_GET, [this]() {
        std::vector<Alarm*> activeAlarms = controller.getActiveAlarms();
        
        int criticalCount = 0, highCount = 0, mediumCount = 0, lowCount = 0;
        int newCount = 0, activeCount = 0, acknowledgedCount = 0;
        
        for (auto alarm : activeAlarms) {
            // Count by priority
            switch (alarm->getPriority()) {
                case AlarmPriority::PRIORITY_CRITICAL: criticalCount++; break;
                case AlarmPriority::PRIORITY_HIGH: highCount++; break;
                case AlarmPriority::PRIORITY_MEDIUM: mediumCount++; break;
                case AlarmPriority::PRIORITY_LOW: lowCount++; break;
            }
            
            // Count by stage
            switch (alarm->getStage()) {
                case AlarmStage::NEW: newCount++; break;
                case AlarmStage::ACTIVE: activeCount++; break;
                case AlarmStage::ACKNOWLEDGED: acknowledgedCount++; break;
                default: break;
            }
        }
        
        DynamicJsonDocument doc(512);
        doc["totalActive"] = activeAlarms.size();
        doc["totalConfigured"] = controller.getAlarmCount();
        
        JsonObject byPriority = doc.createNestedObject("byPriority");
        byPriority["critical"] = criticalCount;
        byPriority["high"] = highCount;
        byPriority["medium"] = mediumCount;
        byPriority["low"] = lowCount;
        
        JsonObject byStage = doc.createNestedObject("byStage");
        byStage["new"] = newCount;
        byStage["active"] = activeCount;
        byStage["acknowledged"] = acknowledgedCount;
        
        String output;
        serializeJson(doc, output);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });

    // Get acknowledged delays
    server->on("/api/alarms/delays", HTTP_GET, [this]() {
        DynamicJsonDocument doc(512);
        doc["critical"] = getAcknowledgedDelayCritical();
        doc["high"] = getAcknowledgedDelayHigh();
        doc["medium"] = getAcknowledgedDelayMedium();
        doc["low"] = getAcknowledgedDelayLow();
        
        String output;
        serializeJson(doc, output);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });

    // Update acknowledged delays
    server->on("/api/alarms/delays", HTTP_PUT, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data\"}");
            return;
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        bool updated = false;
        
        if (doc.containsKey("critical")) {
            int minutes = doc["critical"].as<int>();
            if (minutes >= 1 && minutes <= 1440) {
                conf["ack_delay_critical"] = String(minutes);
                controller.setAcknowledgedDelayCritical(minutes * 60 * 1000);
                updated = true;
            }
        }
        
        if (doc.containsKey("high")) {
            int minutes = doc["high"].as<int>();
            if (minutes >= 1 && minutes <= 1440) {
                conf["ack_delay_high"] = String(minutes);
                controller.setAcknowledgedDelayHigh(minutes * 60 * 1000);
                updated = true;
            }
        }
        
        if (doc.containsKey("medium")) {
            int minutes = doc["medium"].as<int>();
            if (minutes >= 1 && minutes <= 1440) {
                conf["ack_delay_medium"] = String(minutes);
                controller.setAcknowledgedDelayMedium(minutes * 60 * 1000);
                updated = true;
            }
        }
        
        if (doc.containsKey("low")) {
            int minutes = doc["low"].as<int>();
            if (minutes >= 1 && minutes <= 1440) {
                conf["ack_delay_low"] = String(minutes);
                controller.setAcknowledgedDelayLow(minutes * 60 * 1000);
                updated = true;
            }
        }
        
        if (updated) {
            conf.saveConfigFile();
            server->send(200, "application/json", "{\"status\":\"updated\"}");
        } else {
            server->send(400, "application/json", "{\"error\":\"No valid delays provided\"}");
        }
    });

    // GET /api/alarm-config - Get alarm configuration for all measurement points
    server->on("/api/alarm-config", HTTP_GET, [this]() {
        DynamicJsonDocument doc(8192); // Increased size for all points
        JsonArray pointsArray = doc.createNestedArray("points");
        
        // Get DS18B20 points (0-49)
        for (uint8_t i = 0; i < 50; ++i) {
            MeasurementPoint* point = controller.getMeasurementPoint(i);
            if (!point) continue;
            
            JsonObject pointObj = pointsArray.createNestedObject();
            pointObj["address"] = point->getAddress();
            pointObj["name"] = point->getName();
            pointObj["currentTemp"] = point->getCurrentTemp();
            pointObj["sensorBound"] = (point->getBoundSensor() != nullptr);
            pointObj["lowThreshold"] = point->getLowAlarmThreshold();
            pointObj["highThreshold"] = point->getHighAlarmThreshold();
            // Get hysteresis from the first alarm (all alarms for a point should have the same hysteresis)
            int16_t pointHysteresis = 5; // Default
            auto alarmsForHyst = controller.getAlarmsForPoint(point);
            if (!alarmsForHyst.empty()) {
                pointHysteresis = alarmsForHyst[0]->getHysteresis();
            }
            pointObj["hysteresis"] = pointHysteresis;
            
            // Get actual alarm settings from configured alarms
            auto alarms = controller.getAlarmsForPoint(point);
            
            // Set defaults first
            pointObj["lowPriority"] = 2; // Medium
            pointObj["highPriority"] = 2; // Medium
            pointObj["errorPriority"] = 3; // High
            pointObj["lowEnabled"] = false;
            pointObj["highEnabled"] = false;
            pointObj["errorEnabled"] = false;
            
            // Update with actual values from alarms
            for (auto alarm : alarms) {
                switch (alarm->getType()) {
                    case AlarmType::LOW_TEMPERATURE:
                        pointObj["lowPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["lowEnabled"] = alarm->isEnabled();
                        break;
                    case AlarmType::HIGH_TEMPERATURE:
                        pointObj["highPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["highEnabled"] = alarm->isEnabled();
                        break;
                    case AlarmType::SENSOR_ERROR:
                        pointObj["errorPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["errorEnabled"] = alarm->isEnabled();
                        break;
                }
            }
        }
        
        // Get PT1000 points (50-59)
        for (uint8_t i = 50; i < 60; ++i) {
            MeasurementPoint* point = controller.getMeasurementPoint(i);
            if (!point) continue;
            
            JsonObject pointObj = pointsArray.createNestedObject();
            pointObj["address"] = point->getAddress();
            pointObj["name"] = point->getName();
            pointObj["currentTemp"] = point->getCurrentTemp();
            pointObj["sensorBound"] = (point->getBoundSensor() != nullptr);
            pointObj["lowThreshold"] = point->getLowAlarmThreshold();
            pointObj["highThreshold"] = point->getHighAlarmThreshold();
            // Get hysteresis from the first alarm (all alarms for a point should have the same hysteresis)
            int16_t pointHysteresis = 5; // Default
            auto alarmsForHyst = controller.getAlarmsForPoint(point);
            if (!alarmsForHyst.empty()) {
                pointHysteresis = alarmsForHyst[0]->getHysteresis();
            }
            pointObj["hysteresis"] = pointHysteresis;
            
            // Get actual alarm settings from configured alarms
            auto alarms = controller.getAlarmsForPoint(point);
            
            // Set defaults first
            pointObj["lowPriority"] = 2; // Medium
            pointObj["highPriority"] = 2; // Medium
            pointObj["errorPriority"] = 3; // High
            pointObj["lowEnabled"] = false;
            pointObj["highEnabled"] = false;
            pointObj["errorEnabled"] = false;
            
            // Update with actual values from alarms
            for (auto alarm : alarms) {
                switch (alarm->getType()) {
                    case AlarmType::LOW_TEMPERATURE:
                        pointObj["lowPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["lowEnabled"] = alarm->isEnabled();
                        break;
                    case AlarmType::HIGH_TEMPERATURE:
                        pointObj["highPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["highEnabled"] = alarm->isEnabled();
                        break;
                    case AlarmType::SENSOR_ERROR:
                        pointObj["errorPriority"] = static_cast<int>(alarm->getPriority());
                        pointObj["errorEnabled"] = alarm->isEnabled();
                        break;
                }
            }
        }
        
        String output;
        serializeJson(doc, output);
        
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });

    // POST /api/alarm-config - Update alarm configuration for multiple points
    server->on("/api/alarm-config", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"error\":\"No data provided\"}");
            return;
        }
        
        DynamicJsonDocument doc(8192); // Large enough for multiple points
        DeserializationError err = deserializeJson(doc, server->arg("plain"));
        if (err) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
            return;
        }
        
        if (!doc.containsKey("changes") || !doc["changes"].is<JsonArray>()) {
            server->send(400, "application/json", "{\"error\":\"Missing or invalid 'changes' array\"}");
            return;
        }
        
        JsonArray changes = doc["changes"];
        int updatedCount = 0;
        int errorCount = 0;
        String errors = "";
        
        // Apply changes to each point
        for (JsonVariant changeVar : changes) {
            JsonObject change = changeVar.as<JsonObject>();
            
            if (!change.containsKey("address")) {
                errorCount++;
                errors += "Missing address in change entry; ";
                continue;
            }
            
            uint8_t address = change["address"];
            MeasurementPoint* point = controller.getMeasurementPoint(address);
            
            if (!point) {
                errorCount++;
                errors += "Point " + String(address) + " not found; ";
                continue;
            }
            
            try {
                // Update point name if provided
                if (change.containsKey("name")) {
                    point->setName(change["name"].as<String>());
                }
                
                // Update thresholds if provided
                if (change.containsKey("lowThreshold")) {
                    point->setLowAlarmThreshold(change["lowThreshold"].as<int16_t>());
                }
                if (change.containsKey("highThreshold")) {
                    point->setHighAlarmThreshold(change["highThreshold"].as<int16_t>());
                }
                
                // Update hysteresis if provided - will be applied to all alarms
                int16_t hysteresis = -1;
                if (change.containsKey("hysteresis")) {
                    hysteresis = change["hysteresis"].as<int16_t>();
                }
                
                // Get alarms for this point
                auto alarms = controller.getAlarmsForPoint(point);
                
                // Update alarm priorities and enable flags
                for (auto alarm : alarms) {
                    // Apply hysteresis if provided
                    if (hysteresis >= 0) {
                        alarm->setHysteresis(hysteresis);
                    }
                    
                    switch (alarm->getType()) {
                        case AlarmType::LOW_TEMPERATURE:
                            if (change.containsKey("lowPriority")) {
                                int priority = change["lowPriority"].as<int>();
                                Serial.printf("Setting LOW alarm priority to %d for point %d\n", priority, address);
                                alarm->setPriority(static_cast<AlarmPriority>(priority));
                            }
                            if (change.containsKey("lowEnabled")) {
                                alarm->setEnabled(change["lowEnabled"].as<bool>());
                            }
                            break;
                        case AlarmType::HIGH_TEMPERATURE:
                            if (change.containsKey("highPriority")) {
                                int priority = change["highPriority"].as<int>();
                                Serial.printf("Setting HIGH alarm priority to %d for point %d\n", priority, address);
                                alarm->setPriority(static_cast<AlarmPriority>(priority));
                            }
                            if (change.containsKey("highEnabled")) {
                                alarm->setEnabled(change["highEnabled"].as<bool>());
                            }
                            break;
                        case AlarmType::SENSOR_ERROR:
                            if (change.containsKey("errorPriority")) {
                                int priority = change["errorPriority"].as<int>();
                                Serial.printf("Setting ERROR alarm priority to %d for point %d\n", priority, address);
                                alarm->setPriority(static_cast<AlarmPriority>(priority));
                            }
                            if (change.containsKey("errorEnabled")) {
                                alarm->setEnabled(change["errorEnabled"].as<bool>());
                            }
                            break;
                    }
                }
                
                updatedCount++;
                
            } catch (...) {
                errorCount++;
                errors += "Error updating point " + String(address) + "; ";
            }
        }
        
        // Apply configuration to register map and save
        if (updatedCount > 0) {
            controller.applyConfigToRegisterMap();
            savePointsConfig(); // Save the updated configuration
        }
        
        // Send response
        DynamicJsonDocument response(1024);
        response["success"] = (errorCount == 0);
        response["updatedCount"] = updatedCount;
        response["errorCount"] = errorCount;
        response["message"] = String(updatedCount) + " points updated successfully";
        
        if (errorCount > 0) {
            response["errors"] = errors;
        }
        
        String output;
        serializeJson(response, output);
        server->send((errorCount == 0) ? 200 : 207, "application/json", output); // 207 = Multi-Status
    });

};

// Add these methods to your ConfigManager.cpp file in the logsAPI() function

void ConfigManager::logsAPI() {
    // Get alarm history
    server->on("/api/alarm-history", HTTP_GET, [this]() {
        String startDate = server->arg("start");
        String endDate = server->arg("end");
        
        if (startDate.isEmpty() || endDate.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date parameters\"}");
            return;
        }
        
        // Use static method
        String historyJson = LoggerManager::getAlarmHistoryJson(startDate, endDate);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", historyJson);
    });
    
    // Export alarm history as CSV
    server->on("/api/alarm-history/export", HTTP_GET, [this]() {
        String startDate = server->arg("start");
        String endDate = server->arg("end");
        
        if (startDate.isEmpty() || endDate.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date parameters\"}");
            return;
        }
        
        // Use static method
        String csvData = LoggerManager::getAlarmHistoryCsv(startDate, endDate);
        
        if (csvData.length() > 0) {
            String filename = "alarm_history_" + startDate + "_to_" + endDate + ".csv";
            server->sendHeader("Content-Type", "text/csv");
            server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            server->send(200, "text/csv", csvData);
        } else {
            server->send(404, "application/json", "{\"success\":false,\"error\":\"No alarm history found\"}");
        }
    });
    
    // Get available alarm log files
    server->on("/api/alarm-history/files", HTTP_GET, [this]() {
        DynamicJsonDocument doc(2048);
        JsonArray filesArray = doc.createNestedArray("files");
        
        std::vector<String> files = LoggerManager::getAlarmStateLogFiles();
        for (const String& file : files) {
            filesArray.add(file);
        }
        
        String output;
        serializeJson(doc, output);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });

    // NEW EVENT LOG ENDPOINTS
    
    // Get event logs
    server->on("/api/event-logs", HTTP_GET, [this]() {
        String startDate = server->arg("start");
        String endDate = server->arg("end");
        
        if (startDate.isEmpty() || endDate.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date parameters\"}");
            return;
        }
        
        String eventLogsJson = LoggerManager::getEventLogsJson(startDate, endDate);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", eventLogsJson);
    });
    
    // Export event logs as CSV
    server->on("/api/event-logs/export", HTTP_GET, [this]() {
        String startDate = server->arg("start");
        String endDate = server->arg("end");
        
        if (startDate.isEmpty() || endDate.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date parameters\"}");
            return;
        }
        
        String csvData = LoggerManager::getEventLogsCsv(startDate, endDate);
        
        if (csvData.length() > 0) {
            String filename = "event_logs_" + startDate + "_to_" + endDate + ".csv";
            server->sendHeader("Content-Type", "text/csv");
            server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            server->send(200, "text/csv", csvData);
        } else {
            server->send(404, "application/json", "{\"success\":false,\"error\":\"No event logs found\"}");
        }
    });
    
    // Get available event log files
    server->on("/api/event-logs/files", HTTP_GET, [this]() {
        DynamicJsonDocument doc(2048);
        JsonArray filesArray = doc.createNestedArray("files");
        
        std::vector<String> files = LoggerManager::getEventLogFilesStatic();
        for (const String& file : files) {
            filesArray.add(file);
        }
        
        String output;
        serializeJson(doc, output);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", output);
    });
    
    // Get event log statistics
    server->on("/api/event-logs/stats", HTTP_GET, [this]() {
        String startDate = server->arg("start");
        String endDate = server->arg("end");
        
        if (startDate.isEmpty() || endDate.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date parameters\"}");
            return;
        }
        
        String statsJson = LoggerManager::getEventLogStatsJson(startDate, endDate);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->send(200, "application/json", statsJson);
    });
}

// Implementation of alarm event retrieval for charts
std::vector<ConfigManager::AlarmEvent> ConfigManager::getAlarmEventsForPoint(
    uint8_t pointAddress, const String& startDate, const String& endDate) {
    
    std::vector<AlarmEvent> events;
    
    // Get alarm log files in date range
    std::vector<String> files = LoggerManager::getAlarmStateLogFiles();
    
    Serial.printf("getAlarmEventsForPoint: Point=%d, StartDate=%s, EndDate=%s\n", 
                 pointAddress, startDate.c_str(), endDate.c_str());
    Serial.printf("Found %d alarm log files\n", files.size());
    
    for (const String& filename : files) {
        Serial.printf("Checking alarm file: %s\n", filename.c_str());
        // Check if file is in date range
        // Format is "alarm_states_YYYY-MM-DD.csv"
        if (filename.startsWith("alarm_states_") && filename.endsWith(".csv")) {
            // Extract date: remove "alarm_states_" (13 chars) and ".csv" (4 chars)
            String fileDate = filename.substring(13, filename.length() - 4);
            
            Serial.printf("File date: %s, comparing with range [%s, %s]\n", 
                         fileDate.c_str(), startDate.c_str(), endDate.c_str());
            
            // Simple date comparison (works for YYYY-MM-DD format)
            if (fileDate >= startDate && fileDate <= endDate) {
                Serial.printf("Processing alarm file: %s (date: %s)\n", filename.c_str(), fileDate.c_str());
                
                // Debug: Show what directory we're using
                String alarmDir = LoggerManager::getLogDirectoryPath("alarm");
                Serial.printf("DEBUG: Alarm directory path: %s\n", alarmDir.c_str());
                Serial.printf("DEBUG: Attempting to open: %s/%s\n", alarmDir.c_str(), filename.c_str());
                
                // Open and parse the file - use "alarm" not "alarms"!
                File file = LoggerManager::openLogFile(filename, "alarm");
                if (!file) {
                    Serial.printf("ERROR: Failed to open alarm file: %s\n", filename.c_str());
                    Serial.printf("ERROR: File path would be: %s/%s\n", alarmDir.c_str(), filename.c_str());
                    continue;
                }
                
                Serial.printf("SUCCESS: Opened alarm file: %s (size: %d bytes)\n", filename.c_str(), file.size());
                
                // Skip header line
                if (file.available()) {
                    file.readStringUntil('\n');
                }
                
                // Read each line
                int lineNumber = 1; // Header was line 0
                while (file.available()) {
                    String line = file.readStringUntil('\n');
                    lineNumber++;
                    
                    if (line.isEmpty()) {
                        Serial.printf("DEBUG: Line %d is empty, skipping\n", lineNumber);
                        continue;
                    }
                    
                    // Debug first few lines
                    if (lineNumber <= 5) {
                        Serial.printf("DEBUG: Line %d: %s\n", lineNumber, line.c_str());
                    }
                    
                    // Parse CSV line: Timestamp,PointNumber,PointName,AlarmType,AlarmPriority,PreviousState,NewState,CurrentTemperature,Threshold
                    int commaCount = 0;
                    int lastComma = -1;
                    String fields[9];
                    
                    for (int i = 0; i <= line.length() && commaCount < 9; i++) {
                        if (i == line.length() || line.charAt(i) == ',') {
                            fields[commaCount] = line.substring(lastComma + 1, i);
                            lastComma = i;
                            commaCount++;
                        }
                    }
                    
                    if (commaCount < 9) {
                        Serial.printf("DEBUG: Line %d has only %d fields, expected 9, skipping\n", lineNumber, commaCount);
                        continue;
                    }
                    
                    // Check if this is for our point (field 1 is PointNumber)
                    int parsedPoint = fields[1].toInt();
                    Serial.printf("DEBUG: Line %d - Point=%d (looking for %d), Type=%s, State=%s, Temp=%s, Threshold=%s\n", 
                                 lineNumber, parsedPoint, pointAddress, fields[3].c_str(), fields[6].c_str(),
                                 fields[7].c_str(), fields[8].c_str());
                    
                    if (fields[1].toInt() == pointAddress) {
                        AlarmEvent event;
                        
                        // Store full timestamp for Chart.js compatibility
                        // Format: "2025-07-25 23:51:37" needs to become "2025-07-25T23:51:37"
                        String timestamp = fields[0];
                        // Replace space with 'T' for ISO format
                        timestamp.replace(' ', 'T');
                        event.timestamp = timestamp;
                        
                        event.type = fields[3];      // AlarmType
                        event.newState = fields[6];  // NewState
                        
                        // Parse temperature and threshold (not in x10 format in this log)
                        event.temperature = fields[7].toInt();
                        event.threshold = fields[8].toInt();
                        
                        Serial.printf("Found alarm event: Point %d, Type: %s, State: %s, Timestamp: %s\n", 
                                     pointAddress, event.type.c_str(), event.newState.c_str(), event.timestamp.c_str());
                        
                        events.push_back(event);
                    }
                }
                
                file.close();
                Serial.printf("DEBUG: Finished processing file %s, found %d events for point %d\n", 
                             filename.c_str(), events.size(), pointAddress);
            }
        } else {
            Serial.printf("DEBUG: Skipping file %s - doesn't match alarm file pattern\n", filename.c_str());
        }
    }
    
    Serial.printf("DEBUG: Total alarm events found for point %d: %d events\n", pointAddress, events.size());
    return events;
}

void ConfigManager::downloadAPI() {
    // API endpoint to list all data log files
    server->on("/api/data-log-files", HTTP_GET, [this]() {
        DynamicJsonDocument doc(4096);
        doc["success"] = true;
        JsonArray filesArray = doc.createNestedArray("files");
        
        // Get list of temperature data log files
        std::vector<String> files = LoggerManager::getLogFiles();
        for (const String& filename : files) {
            JsonObject fileObj = filesArray.createNestedObject();
            fileObj["filename"] = filename;
            
            // Get file info using static method
            size_t fileSize;
            String date;
            if (LoggerManager::getFileInfo(filename, "data", fileSize, date)) {
                fileObj["size"] = fileSize;
                fileObj["date"] = date;
            } else {
                fileObj["size"] = 0;
                fileObj["date"] = "";
            }
        }
        
        String output;
        serializeJson(doc, output);
        
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", output);
    });

    // API endpoint to list event log files
    server->on("/api/event-log-files", HTTP_GET, [this]() {
        DynamicJsonDocument doc(4096);
        doc["success"] = true;
        JsonArray filesArray = doc.createNestedArray("files");
        
        // Get list of event log files
        std::vector<String> files = LoggerManager::getEventLogFilesStatic();
        for (const String& filename : files) {
            JsonObject fileObj = filesArray.createNestedObject();
            fileObj["filename"] = filename;
            
            // Get file info using static method
            size_t fileSize;
            String date;
            if (LoggerManager::getFileInfo(filename, "event", fileSize, date)) {
                fileObj["size"] = fileSize;
                fileObj["date"] = date;
            } else {
                fileObj["size"] = 0;
                fileObj["date"] = "";
            }
        }
        
        String output;
        serializeJson(doc, output);
        
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", output);
    });

    // API endpoint to list alarm log files
    server->on("/api/alarm-log-files", HTTP_GET, [this]() {
        DynamicJsonDocument doc(4096);
        doc["success"] = true;
        JsonArray filesArray = doc.createNestedArray("files");
        
        // Get list of alarm state log files
        std::vector<String> files = LoggerManager::getAlarmStateLogFiles();
        for (const String& filename : files) {
            JsonObject fileObj = filesArray.createNestedObject();
            fileObj["filename"] = filename;
            
            // Get file info using static method
            size_t fileSize;
            String date;
            if (LoggerManager::getFileInfo(filename, "alarm", fileSize, date)) {
                fileObj["size"] = fileSize;
                fileObj["date"] = date;
            } else {
                fileObj["size"] = 0;
                fileObj["date"] = "";
            }
        }
        
        String output;
        serializeJson(doc, output);
        
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", output);
    });

    // API endpoint to download temperature data log files
    server->on("/api/data-log-download", HTTP_GET, [this]() {
        String filename = server->arg("file");
        
        if (filename.isEmpty()) {
            server->send(400, "text/plain", "Missing file parameter");
            return;
        }
        
        // Security check - only allow files that start with "temp_log_" and end with ".csv"
        if (!filename.startsWith("temp_log_") || !filename.endsWith(".csv")) {
            server->send(403, "text/plain", "Invalid file type");
            return;
        }
        
        // Open file using static method
        File file = LoggerManager::openLogFile(filename, "data");
        if (!file) {
            server->send(404, "text/plain", "File not found");
            return;
        }
        
        // Set headers for file download
        server->sendHeader("Content-Type", "text/csv");
        server->sendHeader("Content-Disposition", "attachment; filename=" + filename);
        server->sendHeader("Access-Control-Allow-Origin", "*");
        
        // Stream the file directly
        server->streamFile(file, "text/csv");
        file.close();
        
        Serial.printf("Downloaded data log file: %s\n", filename.c_str());
    });

    // API endpoint to download event log files
    server->on("/api/event-log-download", HTTP_GET, [this]() {
        String filename = server->arg("file");
        
        if (filename.isEmpty()) {
            server->send(400, "text/plain", "Missing file parameter");
            return;
        }
        
        // Security check - only allow files that start with "events_" and end with ".csv"
        if (!filename.startsWith("events_") || !filename.endsWith(".csv")) {
            server->send(403, "text/plain", "Invalid file type");
            return;
        }
        
        // Open file using static method
        File file = LoggerManager::openLogFile(filename, "event");
        if (!file) {
            server->send(404, "text/plain", "File not found");
            return;
        }
        
        // Set headers for file download
        server->sendHeader("Content-Type", "text/csv");
        server->sendHeader("Content-Disposition", "attachment; filename=" + filename);
        server->sendHeader("Access-Control-Allow-Origin", "*");
        
        // Stream the file directly
        server->streamFile(file, "text/csv");
        file.close();
        
        Serial.printf("Downloaded event log file: %s\n", filename.c_str());
    });

    // API endpoint to download alarm state log files
    server->on("/api/alarm-log-download", HTTP_GET, [this]() {
        String filename = server->arg("file");
        
        if (filename.isEmpty()) {
            server->send(400, "text/plain", "Missing file parameter");
            return;
        }
        
        // Security check - only allow files that start with "alarm_states_" and end with ".csv"
        if (!filename.startsWith("alarm_states_") || !filename.endsWith(".csv")) {
            server->send(403, "text/plain", "Invalid file type");
            return;
        }
        
        // Open file using static method
        File file = LoggerManager::openLogFile(filename, "alarm");
        if (!file) {
            server->send(404, "text/plain", "File not found");
            return;
        }
        
        // Set headers for file download
        server->sendHeader("Content-Type", "text/csv");
        server->sendHeader("Content-Disposition", "attachment; filename=" + filename);
        server->sendHeader("Access-Control-Allow-Origin", "*");
        
        // Stream the file directly
        server->streamFile(file, "text/csv");
        file.close();
        
        Serial.printf("Downloaded alarm state log file: %s\n", filename.c_str());
    });
    
    // API endpoint for temperature history (for charts) - Simplified version
    server->on("/api/temperature-history", HTTP_GET, [this]() {
        Serial.println("SERVER: /api/temperature-history called");
        uint8_t pointAddress = server->arg("point").toInt();
        String hours = server->arg("hours");
        Serial.printf("Point: %d, Hours: %s\n", pointAddress, hours.c_str());
        
        if (pointAddress >= 60) {
            server->send(400, "text/plain", "Invalid point address");
            return;
        }
        
        // Default to 24 hours if not specified
        int hoursToFetch = hours.isEmpty() ? 24 : hours.toInt();
        if (hoursToFetch < 1) hoursToFetch = 1;
        if (hoursToFetch > 168) hoursToFetch = 168; // Max 7 days
        
        // Build JSON response with limited data
        String jsonResponse = "{\"success\":true,\"pointAddress\":" + String(pointAddress) + 
                             ",\"hours\":" + String(hoursToFetch);
        
        // Get point info for thresholds
        MeasurementPoint* point = controller.getMeasurementPoint(pointAddress);
        if (point) {
            jsonResponse += ",\"pointName\":\"" + point->getName() + "\"";
            jsonResponse += ",\"lowThreshold\":" + String(point->getLowAlarmThreshold());
            jsonResponse += ",\"highThreshold\":" + String(point->getHighAlarmThreshold());
        }
        
        jsonResponse += ",\"data\":[";
        
        // Calculate decimation factor based on time range
        // Aim for ~300-500 points for smooth visualization
        int decimationFactor = 1;
        if (hoursToFetch == 1) decimationFactor = 1;      // ~60 points
        else if (hoursToFetch <= 6) decimationFactor = 2;  // ~180 points  
        else if (hoursToFetch <= 12) decimationFactor = 3; // ~240 points
        else if (hoursToFetch <= 24) decimationFactor = 5; // ~288 points
        else if (hoursToFetch <= 48) decimationFactor = 10; // ~288 points
        else if (hoursToFetch <= 96) decimationFactor = 20; // ~288 points
        else decimationFactor = 30; // 7 days: ~336 points
        
        Serial.printf("Decimation factor: %d\n", decimationFactor);
        
        // Get list of log files
        // Note: Files are sorted oldest first, so files[0] is the oldest
        std::vector<String> files = LoggerManager::getLogFiles();
        Serial.printf("Found %d log files\n", files.size());
        
        bool firstDataPoint = true;
        int pointCounter = 0;
        int totalPointsProcessed = 0;
        const int MAX_POINTS = 500; // Increased for better data density and smooth charts
        
        // Calculate how many files to process based on time range
        int daysToProcess = (hoursToFetch + 23) / 24; // Round up
        int filesToProcess = min(daysToProcess + 1, (int)files.size()); // +1 for partial days
        int filesProcessed = 0;
        
        Serial.printf("Time range: %d hours = %d days, will process %d files\n", 
                     hoursToFetch, daysToProcess, filesToProcess);
        
        // Process files in reverse order (most recent first)
        int validDataPoints = 0; // Count of non-null data points
        unsigned long processingStartTime = millis(); // Add timeout tracking
        const unsigned long MAX_PROCESSING_TIME = 5000; // 5 second timeout
        
        // Calculate target file index range based on date
        time_t now;
        time(&now);
        struct tm* currentTime = localtime(&now);
        
        for (int fileIdx = files.size() - 1; fileIdx >= 0 && filesProcessed < filesToProcess && totalPointsProcessed < MAX_POINTS; fileIdx--) {
            const String& filename = files[fileIdx];
            
            // Extract date from filename (temp_log_YYYY-MM-DD.csv)
            if (filename.startsWith("temp_log_") && filename.endsWith(".csv")) {
                String fileDateStr = filename.substring(9, 19); // Extract YYYY-MM-DD
                
                // TODO: Add proper date comparison to skip files outside the requested range
                // For now, process files based on count
            }
            
            Serial.printf("Processing file %d/%d: %s\n", filesProcessed + 1, filesToProcess, filename.c_str());
            
            // Check for timeout
            if (millis() - processingStartTime > MAX_PROCESSING_TIME) {
                Serial.println("Temperature history processing timeout - sending partial data");
                break;
            }
            
            filesProcessed++;
            
            // Open file for reading
            File file = LoggerManager::openLogFile(filename, "data");
            if (!file) {
                Serial.println("Failed to open file");
                continue;
            }
            
            // Skip header line
            if (file.available()) {
                file.readStringUntil('\n');
            }
            
            // Read data lines
            int emptyLinesInRow = 0;
            int linesRead = 0;
            int linesInThisFile = 0;
            
            while (file.available() && totalPointsProcessed < MAX_POINTS) {
                // Yield periodically to prevent watchdog
                if (linesInThisFile++ % 100 == 0) {
                    yield();
                    
                    // Also check timeout here
                    if (millis() - processingStartTime > MAX_PROCESSING_TIME) {
                        Serial.println("Timeout during file processing");
                        file.close();
                        break;
                    }
                }
                
                String line = file.readStringUntil('\n');
                if (line.isEmpty()) continue;
                linesRead++;
                
                // Quick check for empty data BEFORE decimation
                if (linesRead < 200) { // Check first 200 lines
                    // Parse to check if point has data
                    int quickCommaCount = 0;
                    int quickLastComma = -1;
                    String quickTempStr = "";
                    int quickTargetColumn = pointAddress + 2;
                    
                    for (int i = 0; i <= line.length() && quickCommaCount <= quickTargetColumn; i++) {
                        if (i == line.length() || line.charAt(i) == ',') {
                            if (quickCommaCount == quickTargetColumn) {
                                quickTempStr = line.substring(quickLastComma + 1, i);
                                break;
                            }
                            quickLastComma = i;
                            quickCommaCount++;
                        }
                    }
                    
                    if (quickTempStr.isEmpty() || quickTempStr == " ") {
                        emptyLinesInRow++;
                        
                        // Only attempt jump once per file to avoid infinite loops
                        static bool hasJumpedInCurrentFile = false;
                        static String lastJumpedFile = "";
                        
                        // Reset jump flag for new file
                        if (lastJumpedFile != filename) {
                            hasJumpedInCurrentFile = false;
                            lastJumpedFile = filename;
                        }
                        
                        if (!hasJumpedInCurrentFile && emptyLinesInRow > 50 && validDataPoints == 0) {
                            // Skip to end of file
                            long fileSize = file.size();
                            Serial.printf("No data in first %d lines, jumping to find data (file size: %ld)\n", 
                                        emptyLinesInRow, fileSize);
                            
                            hasJumpedInCurrentFile = true; // Mark that we've jumped
                            
                            // Try multiple positions to find data
                            bool foundDataAfterJump = false;
                            size_t positions[] = {
                                fileSize / 4,      // 25% into file
                                fileSize / 2,      // 50% into file  
                                fileSize * 3 / 4,  // 75% into file
                                fileSize > 10000 ? fileSize - 10000 : fileSize - 1000  // Near end
                            };
                            
                            for (size_t jumpPos : positions) {
                                if (jumpPos >= fileSize) continue;
                                
                                file.seek(jumpPos);
                                file.readStringUntil('\n'); // Skip partial line
                                
                                // Try a few lines to see if there's data
                                for (int testLines = 0; testLines < 10; testLines++) {
                                    String testLine = file.readStringUntil('\n');
                                    if (testLine.length() < 10) continue;
                                    
                                    // Quick check for data in the target column
                                    int testCommaCount = 0;
                                    bool hasData = false;
                                    for (int i = 0; i < testLine.length() && testCommaCount <= quickTargetColumn; i++) {
                                        if (testLine[i] == ',') {
                                            testCommaCount++;
                                            if (testCommaCount == quickTargetColumn && i < testLine.length() - 1) {
                                                // Check if there's data after this comma
                                                int nextComma = testLine.indexOf(',', i + 1);
                                                String testTemp = testLine.substring(i + 1, nextComma > 0 ? nextComma : testLine.length());
                                                testTemp.trim();
                                                if (!testTemp.isEmpty()) {
                                                    hasData = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    
                                    if (hasData) {
                                        Serial.printf("Found data at position %lu\n", jumpPos);
                                        // Go back to start of found data section
                                        file.seek(jumpPos);
                                        file.readStringUntil('\n'); // Skip partial line again
                                        foundDataAfterJump = true;
                                        break;
                                    }
                                    
                                    yield();
                                }
                                
                                if (foundDataAfterJump) break;
                            }
                            
                            if (!foundDataAfterJump) {
                                Serial.println("No data found in file after searching multiple positions");
                                break; // Skip this file
                            }
                            
                            emptyLinesInRow = 0;
                            linesRead = 0;
                            pointCounter = 0; // Reset decimation counter
                            continue;
                        }
                    } else {
                        emptyLinesInRow = 0;
                    }
                }
                
                // Only process every Nth line based on decimation
                if (pointCounter++ % decimationFactor != 0) continue;
                
                // Debug first line parsing
                if (totalPointsProcessed == 0 && filesProcessed == 1) {
                    Serial.printf("First data line: %s\n", line.substring(0, 50).c_str());
                }
                
                // Parse only the required fields
                int commaCount = 0;
                int lastComma = -1;
                String dateStr = "";
                String timeStr = "";
                String tempStr = "";
                
                // Target column for this point
                int targetColumn = pointAddress + 2;
                
                if (totalPointsProcessed == 0) {
                    Serial.printf("Looking for point %d in column %d\n", pointAddress, targetColumn);
                }
                
                for (int i = 0; i <= line.length(); i++) {
                    if (i == line.length() || line.charAt(i) == ',') {
                        if (commaCount == 0) {
                            dateStr = line.substring(lastComma + 1, i);
                        } else if (commaCount == 1) {
                            timeStr = line.substring(lastComma + 1, i);
                        } else if (commaCount == targetColumn) {
                            tempStr = line.substring(lastComma + 1, i);
                            break;
                        }
                        lastComma = i;
                        commaCount++;
                    }
                }
                
                // Handle missing temperature data
                bool hasData = !tempStr.isEmpty() && tempStr != " ";
                
                if (hasData) {
                    // Log when we find actual data
                    if (validDataPoints < 5) {
                        Serial.printf("Found data at point %d: temp='%s'\n", 
                                    totalPointsProcessed, tempStr.c_str());
                    }
                }
                
                // Add data point to JSON (with null for missing data)
                if (!firstDataPoint) {
                    jsonResponse += ",";
                }
                firstDataPoint = false;
                
                // Combine date and time into ISO format for consistency with alarm events
                // dateStr format: "2025-07-26", timeStr format: "HH:MM:SS"
                String fullTimestamp = dateStr + "T" + timeStr;
                jsonResponse += "{\"timestamp\":\"" + fullTimestamp + "\",";
                
                if (hasData) {
                    // Parse temperature value
                    float temp = tempStr.toFloat();
                    
                    // Check if it's a valid temperature
                    if (temp == 0.0 && tempStr != "0") {
                        // Invalid temperature, treat as null
                        jsonResponse += "\"temperature\":null}";
                    } else {
                        // Debug logging
                        if (totalPointsProcessed < 5) {
                            Serial.printf("Debug: timeStr='%s', tempStr='%s', parsed temp=%.1f\n", 
                                          timeStr.c_str(), tempStr.c_str(), temp);
                        }
                        jsonResponse += "\"temperature\":" + String(temp, 1) + "}";
                        validDataPoints++; // Count valid data points
                    }
                } else {
                    // No data, use null
                    jsonResponse += "\"temperature\":null}";
                }
                
                totalPointsProcessed++;
                
                // Check memory usage periodically
                if (jsonResponse.length() > 20000) {
                    Serial.println("Response too large, stopping");
                    break;
                }
                
                // Yield to prevent watchdog
                if (totalPointsProcessed % 10 == 0) {
                    yield();
                }
            }
            file.close();
        }
        
        jsonResponse += "]";
        
        // Get alarm events for this point and time range
        // Use the date range from the actual log files processed
        String endDate = "";
        String startDate = "";
        
        // Extract dates from the first and last processed log files
        if (!files.empty()) {
            // Get the most recent date from the LAST file in array (newest)
            // Files are processed in reverse order, so the newest is at the end
            String lastFile = files[files.size() - 1];  // Get the last element (newest file)
            Serial.printf("DEBUG: Newest file in array: %s\n", lastFile.c_str());
            if (lastFile.startsWith("temp_log_") && lastFile.length() > 18) {
                // Extract date from filename: temp_log_YYYY-MM-DD_n.csv
                endDate = lastFile.substring(9, 19);  // Extract YYYY-MM-DD
                Serial.printf("DEBUG: Extracted endDate from '%s': '%s'\n", lastFile.c_str(), endDate.c_str());
            }
            
            // Get the oldest date based on hours requested
            // For now, use a simple calculation
            if (!endDate.isEmpty()) {
                int daysBack = (hoursToFetch + 23) / 24;  // Round up
                
                // Parse the end date
                int year = endDate.substring(0, 4).toInt();
                int month = endDate.substring(5, 7).toInt();
                int day = endDate.substring(8, 10).toInt();
                
                // Calculate start date - simple approach for now
                // Just subtract days from the current date
                char dateBuffer[11];
                
                if (daysBack <= 0) {
                    startDate = endDate;
                } else if (daysBack == 1) {
                    // For 24 hours, just go back one day
                    if (day > 1) {
                        snprintf(dateBuffer, sizeof(dateBuffer), "%04d-%02d-%02d", year, month, day - 1);
                    } else {
                        // Handle month boundary
                        int prevMonth = month - 1;
                        int prevYear = year;
                        if (prevMonth < 1) {
                            prevMonth = 12;
                            prevYear--;
                        }
                        // Get last day of previous month
                        int lastDay = 31;
                        if (prevMonth == 4 || prevMonth == 6 || prevMonth == 9 || prevMonth == 11) {
                            lastDay = 30;
                        } else if (prevMonth == 2) {
                            lastDay = 28; // Simplified - doesn't handle leap years
                        }
                        snprintf(dateBuffer, sizeof(dateBuffer), "%04d-%02d-%02d", prevYear, prevMonth, lastDay);
                    }
                    startDate = String(dateBuffer);
                } else {
                    // For multiple days, use a simpler approach
                    // This is a temporary fix - in production you'd use proper date libraries
                    snprintf(dateBuffer, sizeof(dateBuffer), "%04d-%02d-%02d", year, month, day > daysBack ? day - daysBack : 1);
                    startDate = String(dateBuffer);
                }
            }
        }
        
        // Fallback to current date if we couldn't determine from files
        if (endDate.isEmpty()) {
            time_t now;
            time(&now);
            struct tm* timeinfo = localtime(&now);
            if (timeinfo != nullptr) {
                char dateBuffer[11];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", timeinfo);
                endDate = String(dateBuffer);
                startDate = endDate;
            } else {
                // If time is not set, use a recent date as fallback
                endDate = "2025-07-26";  // Today's date based on log files
                startDate = "2025-07-25"; // Yesterday
            }
        }
        
        // Debug: Show calculated date range
        Serial.printf("DEBUG: Calculated date range for alarms: startDate=%s, endDate=%s (hoursToFetch=%d)\n", 
                     startDate.c_str(), endDate.c_str(), hoursToFetch);
        
        // Get alarm events
        std::vector<AlarmEvent> events = getAlarmEventsForPoint(pointAddress, startDate, endDate);
        
        // Filter alarm events to match the exact time range requested
        time_t currentTimestamp;
        time(&currentTimestamp);
        time_t startTime = currentTimestamp - (hoursToFetch * 3600); // Convert hours to seconds
        
        jsonResponse += ",\"alarmEvents\":[";
        bool firstEvent = true;
        int includedEvents = 0;
        
        for (const auto& event : events) {
            // Parse event timestamp to check if it's within the time range
            // Format: "YYYY-MM-DDTHH:MM:SS"
            struct tm eventTm = {0};
            int year, month, day, hour, min, sec;
            if (sscanf(event.timestamp.c_str(), "%d-%d-%dT%d:%d:%d", 
                      &year, &month, &day, &hour, &min, &sec) == 6) {
                eventTm.tm_year = year - 1900;
                eventTm.tm_mon = month - 1;
                eventTm.tm_mday = day;
                eventTm.tm_hour = hour;
                eventTm.tm_min = min;
                eventTm.tm_sec = sec;
                time_t eventTime = mktime(&eventTm);
                
                // Only include events within the requested time range
                if (eventTime >= startTime && eventTime <= currentTimestamp) {
                    if (!firstEvent) jsonResponse += ",";
                    firstEvent = false;
                    includedEvents++;
            
                    jsonResponse += "{\"timestamp\":\"" + event.timestamp + "\"";
                    jsonResponse += ",\"type\":\"" + event.type + "\"";  
                    jsonResponse += ",\"state\":\"" + event.newState + "\"";
                    // Alarm state logs store actual temperature values, not x10 format
                    jsonResponse += ",\"temperature\":" + String(event.temperature);
                    jsonResponse += ",\"threshold\":" + String(event.threshold) + "}";
                } else {
                    Serial.printf("Skipping alarm event outside time range: %s\n", event.timestamp.c_str());
                }
            }
        }
        
        jsonResponse += "]}";
        
        Serial.printf("Total points: %d, Valid data points: %d, Alarm events: %d included, %d total, Response size: %d bytes\n", 
                     totalPointsProcessed, validDataPoints, includedEvents, events.size(), jsonResponse.length());
        
        // Send complete response
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Cache-Control", "no-store");
        server->send(200, "application/json", jsonResponse);
    });
}
