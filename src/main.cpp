#include <Arduino.h>
#include "Sensor.h"
#include "RegisterMap.h"
#include "TempModbusServer.h"

// Define the OneWire bus pin
const int ONE_WIRE_BUS = 4;

// Create register map
RegisterMap registerMap;

// Create Modbus server (using Serial2 for RTU communication)
TempModbusServer* modbusServer;

// Create an array to hold our sensors
Sensor* sensors[2];

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("\nTemperature Monitoring System");
    Serial.println("----------------------------");
    
    // Create a temporary OneWire and DallasTemperature instance to discover sensors
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature dallasSensors(&oneWire);
    dallasSensors.begin();
    
    // Count devices on the bus
    int deviceCount = dallasSensors.getDeviceCount();
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" DS18B20 sensors.");
    
    // Get addresses for up to 2 sensors
    DeviceAddress sensorAddress;
    
    // Create and initialize sensor objects
    for (int i = 0; i < min(2, deviceCount); i++) {
        if (dallasSensors.getAddress(sensorAddress, i)) {
            // Print the address
            Serial.print("Sensor ");
            Serial.print(i);
            Serial.print(" Address: ");
            for (uint8_t j = 0; j < 8; j++) {
                if (sensorAddress[j] < 16) Serial.print("0");
                Serial.print(sensorAddress[j], HEX);
            }
            Serial.println();
            
            // Create a new sensor with address i and name "Sensor i"
            String sensorName = "Sensor " + String(i);
            sensors[i] = new Sensor(SensorType::DS18B20, i, sensorName);
            
            // Set up the DS18B20 with the pin and address
            sensors[i]->setupDS18B20(ONE_WIRE_BUS, sensorAddress);
            
            // Initialize the sensor
            if (sensors[i]->initialize()) {
                Serial.print("Sensor ");
                Serial.print(i);
                Serial.println(" initialized successfully.");
                
                // Increment the active DS18B20 count in register map
                registerMap.incrementActiveDS18B20();
                
                // Set custom alarm thresholds
                sensors[i]->setLowAlarmThreshold(15);  // Alarm if below 15°C
                sensors[i]->setHighAlarmThreshold(30); // Alarm if above 30°C
                
                // Update register map with sensor configuration
                registerMap.updateFromSensor(*sensors[i]);
            } else {
                Serial.print("Failed to initialize sensor ");
                Serial.println(i);
            }
        }
    }
    
    // Initialize Modbus server with Serial2 (RX=22, TX=23)
    modbusServer = new TempModbusServer(registerMap, 0x01, Serial2, 22, 23, 9600);
    
    if (modbusServer->begin()) {
        Serial.println("Modbus RTU server started successfully");
    } else {
        Serial.println("Failed to start Modbus RTU server");
    }
    
    Serial.println("\nSystem is now running...");
    Serial.println("Reading temperatures and serving Modbus requests");
}

void loop() {
    // Read temperatures from all sensors
    for (int i = 0; i < 2; i++) {
        if (sensors[i] != nullptr) {
            // Read temperature
            if (sensors[i]->readTemperature()) {
                // Update alarm status based on thresholds
                sensors[i]->updateAlarmStatus();
                
                
                // Update register map with sensor data
                registerMap.updateFromSensor(*sensors[i]);
            }
        }
    }
    
    // Print a status message every 10 seconds
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 10000) {
        Serial.println("System running - serving Modbus RTU requests");
        
        // Print current temperature readings
        for (int i = 0; i < 2; i++) {
            if (sensors[i] != nullptr) {
                Serial.print("Sensor ");
                Serial.print(i);
                Serial.print(": ");
                Serial.print(sensors[i]->getCurrentTemp());
                Serial.println(" °C");
            }
        }
        
        lastPrintTime = millis();
    }
    
    // Small delay to prevent CPU hogging
    //delay(100);
}
