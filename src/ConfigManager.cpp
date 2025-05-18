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
  - rs485_rx_pin:
      label: RS485 RX Pin
      type: number
      min: 0
      max: 39
      default: 22
  - rs485_tx_pin:
      label: RS485 TX Pin
      type: number
      min: 0
      max: 39
      default: 23
  - rs485_de_pin:
      label: RS485 DE/RE Pin
      type: number
      min: 0
      max: 39
      default: 18

Sensor settings:
  - onewire_pin:
      label: OneWire Bus Pin
      type: number
      min: 0
      max: 39
      default: 4
  - auto_discover:
      label: Auto-discover sensors on startup
      checked: true
  - reset_min_max:
      label: Reset Min/Max Values
      type: button
      attribs: onClick="resetMinMax()"
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
    server->on("/", HTTP_GET, [this]() {
        if (LittleFS.exists("/index.html")) {
            server->sendHeader("HTTP/1.1 200 OK", "");
            server->sendHeader("Content-Type", "text/html");
            server->sendHeader("Connection", "close");
            server->sendHeader("Cache-Control", "max-age=3600");
            File file = LittleFS.open("/index.html", "r");
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
        bool discovered = controller.discoverDS18B20Sensors();
        if (discovered) {
            // Add discovered sensors to configuration
            for (int i = 0; i < controller.getSensorCount(); i++) {
                Sensor* sensor = controller.getSensorByIndex(i);
                if (sensor && sensor->getType() == SensorType::DS18B20) {
                    addSensorToConfig(
                        SensorType::DS18B20,
                        sensor->getAddress(),
                        sensor->getName(),
                        sensor->getDS18B20Address()
                    );
                }
            }
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
    
    // API endpoint to update a sensor
    server->on("/api/sensors", HTTP_PUT, [this]() {
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, body);
            
            if (!error) {
                uint8_t address = doc["address"];
                String name = doc["name"].as<String>();
                int16_t lowAlarm = doc["lowAlarm"];
                int16_t highAlarm = doc["highAlarm"];
                
                Sensor* sensor = controller.findSensor(address);
                if (sensor) {
                    sensor->setName(name);
                    sensor->setLowAlarmThreshold(lowAlarm);
                    sensor->setHighAlarmThreshold(highAlarm);
                    
                    // Update configuration
                    updateSensorInConfig(address, name, lowAlarm, highAlarm);
                    server->sendHeader("HTTP/1.1 200 OK", "");
                    server->sendHeader("Content-Type", "text/plain");
                    server->sendHeader("Connection", "close");
                    server->send(200, "text/plain", "Sensor updated");
                } else {
                    server->sendHeader("HTTP/1.1 404 Not Found", "");
                    server->sendHeader("Content-Type", "text/plain");
                    server->sendHeader("Connection", "close");
                    server->send(404, "text/plain", "Sensor not found");
                }
            } else {
                server->sendHeader("HTTP/1.1 400 Bad Request", "");
                server->sendHeader("Content-Type", "text/plain");
                server->sendHeader("Connection", "close");
                server->send(400, "text/plain", "Invalid JSON");
            }
        } else {
            server->sendHeader("HTTP/1.1 400 Bad Request", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(400, "text/plain", "No data provided");
        }
    });
    
    // API endpoint to delete a sensor
    server->on("/api/sensors", HTTP_DELETE, [this]() {
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, body);
            
            if (!error) {
                uint8_t address = doc["address"];
                
                if (controller.removeSensor(address)) {
                    // Remove from configuration
                    removeSensorFromConfig(address);
                    server->sendHeader("HTTP/1.1 200 OK", "");
                    server->sendHeader("Content-Type", "text/plain");
                    server->sendHeader("Connection", "close");
                    server->send(200, "text/plain", "Sensor removed");
                } else {
                    server->sendHeader("HTTP/1.1 404 Not Found", "");
                    server->sendHeader("Content-Type", "text/plain");
                    server->sendHeader("Connection", "close");
                    server->send(404, "text/plain", "Sensor not found");
                }
            } else {
                server->sendHeader("HTTP/1.1 400 Bad Request", "");
                server->sendHeader("Content-Type", "text/plain");
                server->sendHeader("Connection", "close");
                server->send(400, "text/plain", "Invalid JSON");
            }
        } else {
            server->sendHeader("HTTP/1.1 400 Bad Request", "");
            server->sendHeader("Content-Type", "text/plain");
            server->sendHeader("Connection", "close");
            server->send(400, "text/plain", "No data provided");
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
    loadSensorConfig();
    Serial.println("Sensor data loaded");
    
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

bool ConfigManager::addSensorToConfig(SensorType type, uint8_t address, const String& name, const uint8_t* romAddress) {
    // Create a ConfigAssist for sensor configuration
    ConfigAssist sensorConf("/sensors2.ini", false);
    
    // Create a unique key for this sensor
    String sensorKey = String(type == SensorType::DS18B20 ? "ds_" : "pt_") + String(address);
    
    // Check if sensor already exists
    if (sensorConf(sensorKey + "_name") != "") {
        return false; // Sensor already exists
    }
    
    // Add sensor configuration
    sensorConf[sensorKey + "_name"] = name;
    sensorConf[sensorKey + "_type"] = type == SensorType::DS18B20 ? "DS18B20" : "PT1000";
    sensorConf[sensorKey + "_address"] = String(address);
    sensorConf[sensorKey + "_low_alarm"] = "-10"; // Default low alarm
    sensorConf[sensorKey + "_high_alarm"] = "50"; // Default high alarm
    
    // Add ROM address for DS18B20 sensors
    if (type == SensorType::DS18B20 && romAddress != nullptr) {
        String romHex = "";
        
        Serial.println(F("--- Processing ROM address ---"));
        Serial.print(F("Raw ROM bytes: "));
        for (int i = 0; i < 8; i++) {
            Serial.print(romAddress[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        for (int i = 0; i < 8; i++) {
            if (romAddress[i] < 16) romHex += "0";
            romHex += String(romAddress[i], HEX);
            
            // Debug each byte as it's added to the string
            Serial.print(F("Added byte "));
            Serial.print(i);
            Serial.print(F(": "));
            Serial.print(romAddress[i], HEX);
            Serial.print(F(" -> Current romHex: "));
            Serial.println(romHex);
        }
        
        sensorConf[sensorKey + "_rom"] = romHex;
        
        // Show the final string being written to the configuration
        Serial.print(F("Final ROM string written to config: "));
        Serial.println(romHex);
        Serial.print(F("Config key: "));
        Serial.println(sensorKey + "_rom");
    }
    
    // Save configuration
    sensorConf.saveConfigFile();
    
    return true;
}

bool ConfigManager::removeSensorFromConfig(uint8_t address) {
    // Create a ConfigAssist for sensor configuration
    ConfigAssist sensorConf("/sensors2.ini", false);
    
    // Try both sensor types
    String dsKey = "ds_" + String(address);
    String ptKey = "pt_" + String(address);
    
    bool found = false;
    
    // Check if DS18B20 sensor exists
    if (sensorConf(dsKey + "_name") != "") {
        // Remove all keys for this sensor by setting them to empty
        sensorConf[dsKey + "_name"] = "";
        sensorConf[dsKey + "_type"] = "";
        sensorConf[dsKey + "_address"] = "";
        sensorConf[dsKey + "_low_alarm"] = "";
        sensorConf[dsKey + "_high_alarm"] = "";
        sensorConf[dsKey + "_rom"] = "";
        found = true;
    }
    
    // Check if PT1000 sensor exists
    if (sensorConf(ptKey + "_name") != "") {
        // Remove all keys for this sensor by setting them to empty
        sensorConf[ptKey + "_name"] = "";
        sensorConf[ptKey + "_type"] = "";
        sensorConf[ptKey + "_address"] = "";
        sensorConf[ptKey + "_low_alarm"] = "";
        sensorConf[ptKey + "_high_alarm"] = "";
        found = true;
    }
    
    if (found) {
        // Save configuration
        sensorConf.saveConfigFile();
    }
    
    return found;
}

bool ConfigManager::updateSensorInConfig(uint8_t address, const String& name, int16_t lowAlarm, int16_t highAlarm) {
    // Create a ConfigAssist for sensor configuration
    ConfigAssist sensorConf("/sensors2.ini", false);
    
    // Try both sensor types
    String dsKey = "ds_" + String(address);
    String ptKey = "pt_" + String(address);
    
    bool found = false;
    
    // Check if DS18B20 sensor exists
    if (sensorConf(dsKey + "_name") != "") {
        sensorConf[dsKey + "_name"] = name;
        sensorConf[dsKey + "_low_alarm"] = String(lowAlarm);
        sensorConf[dsKey + "_high_alarm"] = String(highAlarm);
        found = true;
    }
    
    // Check if PT1000 sensor exists
    if (sensorConf(ptKey + "_name") != "") {
        sensorConf[ptKey + "_name"] = name;
        sensorConf[ptKey + "_low_alarm"] = String(lowAlarm);
        sensorConf[ptKey + "_high_alarm"] = String(highAlarm);
        found = true;
    }
    
    if (found) {
        // Save configuration
        sensorConf.saveConfigFile();
    }
    
    return found;
}

void ConfigManager::loadSensorConfig() {
    // Create a ConfigAssist for sensor configuration
    ConfigAssist sensorConf("/sensors2.ini", false);
    
    // Check if the file exists
    if (!LittleFS.exists("/sensors2.ini")) {
        return; // No sensor configuration yet
    }
    
    // Load the configuration
    sensorConf.loadConfigFile();
    
    // Get all keys (ConfigAssist doesn't have getKeys method, so we'll check known patterns)
    for (int i = 0; i < 60; i++) {
        // Check DS18B20 sensors
        String dsPrefix = "ds_" + String(i);
        if (sensorConf(dsPrefix + "_name") != "") {
            // Get sensor type
            String typeStr = sensorConf(dsPrefix + "_type");
            SensorType type = SensorType::DS18B20;
            
            // Get sensor address
            uint8_t address = sensorConf(dsPrefix + "_address").toInt();
            
            // Get sensor name
            String name = sensorConf(dsPrefix + "_name");
            
            // Create sensor
            Sensor* newSensor = new Sensor(type, address, name);
            
            // Set thresholds
            newSensor->setLowAlarmThreshold(sensorConf(dsPrefix + "_low_alarm").toInt());
            newSensor->setHighAlarmThreshold(sensorConf(dsPrefix + "_high_alarm").toInt());
            
            // Set up physical connection
            String romHex = sensorConf(dsPrefix + "_rom");
            if (romHex.length() == 16) {
                uint8_t romAddress[8];
                for (int j = 0; j < 8; j++) {
                    String byteHex = romHex.substring(j*2, j*2+2);
                    romAddress[j] = strtol(byteHex.c_str(), NULL, 16);
                }
                newSensor->setupDS18B20(getOneWirePin(), romAddress);
            }
            
            // Initialize sensor
            if (newSensor->initialize()) {
                controller.addSensor(type, address, name);
            } else {
                delete newSensor;
            }
        }
        
        // Check PT1000 sensors
        String ptPrefix = "pt_" + String(i);
        if (sensorConf(ptPrefix + "_name") != "") {
            // Get sensor type
            String typeStr = sensorConf(ptPrefix + "_type");
            SensorType type = SensorType::PT1000;
            
            // Get sensor address
            uint8_t address = sensorConf(ptPrefix + "_address").toInt();
            
            // Get sensor name
            String name = sensorConf(ptPrefix + "_name");
            
            // Create sensor
            Sensor* newSensor = new Sensor(type, address, name);
            
            // Set thresholds
            newSensor->setLowAlarmThreshold(sensorConf(ptPrefix + "_low_alarm").toInt());
            newSensor->setHighAlarmThreshold(sensorConf(ptPrefix + "_high_alarm").toInt());
            
            // Initialize sensor
            if (newSensor->initialize()) {
                controller.addSensor(type, address, name);
            } else {
                delete newSensor;
            }
        }
    }
}

void ConfigManager::resetMinMaxValues() {
    controller.resetMinMaxValues();
}
