#include <Arduino.h>
#include "TemperatureController.h"
#include "TempModbusServer.h"

// Define pins
const int ONE_WIRE_BUS = 4;
const int RS485_RX_PIN = 22;
const int RS485_TX_PIN = 23;
const int RS485_DE_PIN = 18;

// Create temperature controller
TemperatureController controller(ONE_WIRE_BUS);

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
    
    // Set device information
    controller.setDeviceId(1000);       // Model number
    controller.setFirmwareVersion(0x0100); // v1.0
    controller.setMeasurementPeriod(5); // Read sensors every 5 seconds
    
    // Discover DS18B20 sensors
    Serial.println("Discovering DS18B20 sensors...");
    if (controller.discoverDS18B20Sensors()) {
        Serial.print("Found ");
        Serial.print(controller.getDS18B20Count());
        Serial.println(" DS18B20 sensors");
    } else {
        Serial.println("No DS18B20 sensors found");
    }
    
    // Manually add a PT1000 sensor (if needed)
    // controller.addSensor(SensorType::PT1000, 50, "Boiler Temperature");
    
    // Initialize Modbus server with Serial2 (RX=22, TX=23)
    modbusServer = new TempModbusServer(controller.getRegisterMap(), 0x01, Serial2, RS485_RX_PIN, RS485_TX_PIN, 9600);
    
    if (modbusServer->begin()) {
        Serial.println("Modbus RTU server started successfully");
    } else {
        Serial.println("Failed to start Modbus RTU server");
    }
    
    Serial.println("\nSystem is now running...");
}

void loop() {
    // Update controller (reads sensors and updates register map)
    controller.update();
    
    // Print status every 30 seconds
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 30000) {
        Serial.println("\nSystem Status:");
        Serial.println(controller.getSystemStatusJson());
        Serial.println("\nSensor Data:");
        Serial.println(controller.getSensorsJson());
        
        lastPrintTime = millis();
    }
    
    // Small delay to prevent CPU hogging
    delay(100);
}
