#include "ConfigManager.h"
#include <ArduinoJson.h>


ConfigManager* ConfigManager::instance = nullptr;


// YAML configuration definition
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid:
      label: WiFi SSID
      default: Beeline_2G_F13F37
  - st_pass:
      label: WiFi Password
      default: 1122334455667788
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
        } else {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->send(200, "text/html", "<html><body><h1>Temperature Monitoring System</h1><p><a href='/cfg'>Configuration</a></p><p><a href='/sensors.html'>Sensors</a></p></body></html>");
        }
    });


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


    
    // Add CORS support for OPTIONS requests
    server->on("/api/sensors", HTTP_OPTIONS, [this]() {
        server->sendHeader("HTTP/1.1 204 No Content", "");
        server->sendHeader("Access-Control-Allow-Origin", "*");
        server->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server->send(204);
    });
    
    // Setup WiFi
    bool startAP = true;
    if (conf("st_ssid") != "" && conf("st_pass") != "") {
        // Try to connect to WiFi if credentials are available
        if (connectWiFi(10000)) {
            startAP = false;
        }
    }
    
    // Setup ConfigAssist with web server AFTER registering custom routes
    conf.setup(*server, startAP);
    
    // Start the web server
    server->begin();
    
    // Load sensor configuration
    //loadSensorConfig();
    loadPointsConfig();
    loadAlarmsConfig();
    Serial.println("CM.begin(): Sensor data loaded:");
    //Serial.println(controller.getSensorsJson());
    
    // Apply configuration to controller
    controller.setDeviceId(getDeviceId());
    Serial.println("Device ID set");
    controller.setMeasurementPeriod(getMeasurementPeriod());
    Serial.println("Measurement period set");
    
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
    }
}


void ConfigManager::resetMinMaxValues() {
    controller.resetMinMaxValues();
}



// Save all measurement points and their bindings
void ConfigManager::savePointsConfig() {
    Serial.println('Save points to config ....');
    ConfigAssist pointsConf("/points2.ini", false);

    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) {
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (!point) continue;
        String key = "ds_" + String(point->getAddress());
        pointsConf[key + "_name"] = point->getName();
        pointsConf[key + "_low_alarm"] = String(point->getLowAlarmThreshold());
        pointsConf[key + "_high_alarm"] = String(point->getHighAlarmThreshold());

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

    // DS18B20 points
    for (uint8_t i = 0; i < 50; ++i) {
        String key = "ds_" + String(i);
        MeasurementPoint* point = controller.getDS18B20Point(i);
        if (!point) continue;
        point->setName(pointsConf(key + "_name"));
        point->setLowAlarmThreshold(pointsConf(key + "_low_alarm").toInt());
        point->setHighAlarmThreshold(pointsConf(key + "_high_alarm").toInt());
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
}

