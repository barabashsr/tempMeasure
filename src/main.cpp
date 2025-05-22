#include <Arduino.h>
#include "TemperatureController.h"
#include "TempModbusServer.h"
#include "ConfigManager.h"
#include <SPI.h>


// Create temperature controller

//DS18B20 PINs
#define BUS1_PIN  4
#define BUS2_PIN  18
#define BUS3_PIN  19
#define BUS4_PIN  5

//PT1000 PINs

#define CS1_PIN  32
#define CS2_PIN  33
#define CS3_PIN  26
#define CS4_PIN  27
#define CS5_PIN  15

//RS485 PINs
#define RX_PIN  22
#define TX_PIN  23
#define DE_PIN  18

//SPI PINs
#define SCK_PIN  14
#define MISO_PIN  12
#define MOSI_PIN  13
//#define SS_PIN  15





uint8_t onwWirePins[4] = {BUS1_PIN,  BUS2_PIN, BUS3_PIN, BUS4_PIN};
uint8_t csPins[5] = {CS1_PIN,  CS2_PIN, CS3_PIN, CS4_PIN, CS5_PIN};
TemperatureController controller(onwWirePins, csPins);

// Create configuration manager
ConfigManager* configManager;

// Create Modbus server
TempModbusServer* modbusServer;

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial) {}

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);  // SCK, MISO, MOSI, SS

    // Configure all CS pins as OUTPUT
    pinMode(CS1_PIN, OUTPUT);
    pinMode(CS2_PIN, OUTPUT);
    pinMode(CS3_PIN, OUTPUT);
    pinMode(CS4_PIN, OUTPUT);
    pinMode(CS5_PIN, OUTPUT);
    
    // Set all CS pins HIGH initially (inactive)
    digitalWrite(CS1_PIN, HIGH);
    digitalWrite(CS2_PIN, HIGH);
    digitalWrite(CS3_PIN, HIGH);
    digitalWrite(CS4_PIN, HIGH);
    digitalWrite(CS5_PIN, HIGH);



    Serial.println("\nIndustrial Temperature Monitoring System");
    Serial.println("--------------------------------------");
    //controller = new TemperatureController(onwWirePins);
    Serial.println("\nController created");
    
    // Initialize controller
    controller.begin();
    Serial.println("\nController begin");
    
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
    //controller.setOneWireBusPin(configManager->getOneWirePin());
    Serial.println("controller.setOneWireBusPin(configManager->getOneWirePin());");
    
    //Discover DS18B20 sensors
    controller.discoverDS18B20Sensors();
    controller.discoverPTSensors();
    // Initialize Modbus server if enabled in config
    if (configManager->isModbusEnabled()) {
        Serial.println("Init Modbus RTU server...");
        modbusServer = new TempModbusServer(
            controller.getRegisterMap(),
            configManager->getModbusAddress(),
            Serial2,
            RX_PIN,
            TX_PIN,
            DE_PIN,
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
        Serial.println("\nSensors Status:");
        //Serial.println(controller.getSystemStatusJson());
        Serial.println(controller.getSensorsJson());
        //Serial.println("\nPoints Status:");
        //Serial.println(controller.getSystemStatusJson());
        //Serial.println(controller.getPointsJson());
        
        lastPrintTime = millis();
    }
    
    // Small delay to prevent CPU hogging
    delay(100);
}
