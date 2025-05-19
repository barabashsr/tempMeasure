#include <Arduino.h>
#include "TemperatureController.h"
#include "TempModbusServer.h"
#include "ConfigManager.h"

// Create temperature controller
TemperatureController controller;

// Create configuration manager
ConfigManager* configManager;

// Create Modbus server
TempModbusServer* modbusServer;

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("\nIndustrial Temperature Monitoring System");
    Serial.println("--------------------------------------");
    
    // Initialize controller
    controller.begin();
    
    // Initialize configuration manager
    configManager = new ConfigManager(controller);
    if (!configManager->begin()) {
        Serial.println("Failed to initialize configuration manager");
    }
    
    // Apply configuration to controller
    controller.setDeviceId(configManager->getDeviceId());
    Serial.println("controller.setDeviceId(configManager->getDeviceId());");
    controller.setMeasurementPeriod(configManager->getMeasurementPeriod()*1000);
    Serial.println("controller.setMeasurementPeriod(configManager->getMeasurementPeriod());");
    controller.setOneWireBusPin(configManager->getOneWirePin());
    Serial.println("controller.setOneWireBusPin(configManager->getOneWirePin());");
    
    //Discover DS18B20 sensors if auto-discover is enabled
    if (configManager->getAutoDiscover() && controller.getDS18B20Count() == 0) {
        Serial.println("Auto-discovering DS18B20 sensors...");
        if (controller.discoverDS18B20Sensors()) {
            Serial.print("Found ");
            Serial.print(controller.getDS18B20Count());
            Serial.println(" DS18B20 sensors");
            
            // Add discovered sensors to configuration
            // for (int i = 0; i < controller.getSensorCount(); i++) {
            //     Sensor* sensor = controller.getSensorByIndex(i);
            //     if (sensor && sensor->getType() == SensorType::DS18B20) {
            //         configManager->addSensorToConfig(
            //             SensorType::DS18B20,
            //             sensor->getAddress(),
            //             sensor->getName(),
            //             sensor->getDS18B20Address()
            //         );
            //     }
            // }
        } else {
            Serial.println("No DS18B20 sensors found");
        }
    }
    
    // Initialize Modbus server if enabled in config
    if (configManager->isModbusEnabled()) {
        Serial.println("Init Modbus RTU server...");
        modbusServer = new TempModbusServer(
            controller.getRegisterMap(),
            configManager->getModbusAddress(),
            Serial2,
            configManager->getRxPin(),
            configManager->getTxPin(),
            configManager->getModbusBaudRate()
        );
        Serial.println("Init Modbus RTU server!");

        
        if (modbusServer->begin()) {
            Serial.println("Modbus RTU server started successfully");
        } else {
            Serial.println("Failed to start Modbus RTU server");
        }
    }


    
    Serial.println("\nSystem is now running...");
}

void loop() {
    // Update configuration manager
    configManager->update();
    
    // Update controller (reads sensors and updates register map)
    controller.update();
    
    // Print status every 30 seconds if not in portal mode
    static unsigned long lastPrintTime = 0;
    if (!configManager->isPortalActive() && millis() - lastPrintTime > controller.getMeasurementPeriod()) {
        Serial.println("\nSystem Status:");
        //Serial.println(controller.getSystemStatusJson());
        Serial.println(controller.getSensorsJson());
        
        lastPrintTime = millis();
    }
    
    // Small delay to prevent CPU hogging
    delay(100);
}
