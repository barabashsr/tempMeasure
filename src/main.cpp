#include <Arduino.h>
#include "Sensor.h"

// Define the OneWire bus pin
const int ONE_WIRE_BUS = 4;

// Create an array to hold our sensors
Sensor* sensors[2];

// Function to print sensor details
void printSensorDetails(Sensor* sensor) {
  Serial.print("Sensor: ");
  Serial.print(sensor->getName());
  Serial.print(" (Address: ");
  Serial.print(sensor->getAddress());
  Serial.println(")");
  
  Serial.print("  Current Temperature: ");
  Serial.print(sensor->getCurrentTemp());
  Serial.println(" °C");
  
  Serial.print("  Min Temperature: ");
  Serial.print(sensor->getMinTemp());
  Serial.println(" °C");
  
  Serial.print("  Max Temperature: ");
  Serial.print(sensor->getMaxTemp());
  Serial.println(" °C");
  
  // Print alarm status
  uint16_t alarmStatus = sensor->getAlarmStatus();
  Serial.print("  Alarm Status: 0x");
  Serial.print(alarmStatus, HEX);
  Serial.print(" (");
  if (alarmStatus & Sensor::ALARM_LOW_TEMP) Serial.print("LOW_TEMP ");
  if (alarmStatus & Sensor::ALARM_HIGH_TEMP) Serial.print("HIGH_TEMP ");
  if (alarmStatus == 0) Serial.print("NONE");
  Serial.println(")");
  
  // Print error status
  uint16_t errorStatus = sensor->getErrorStatus();
  Serial.print("  Error Status: 0x");
  Serial.print(errorStatus, HEX);
  Serial.print(" (");
  if (errorStatus & Sensor::ERROR_COMMUNICATION) Serial.print("COMM_ERROR ");
  if (errorStatus & Sensor::ERROR_OUT_OF_RANGE) Serial.print("RANGE_ERROR ");
  if (errorStatus & Sensor::ERROR_DISCONNECTED) Serial.print("DISCONNECTED ");
  if (errorStatus == 0) Serial.print("NONE");
  Serial.println(")");
  
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nDS18B20 Sensor Test");
  Serial.println("------------------");
  
  // Create a temporary OneWire and DallasTemperature instance to discover sensors
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature dallasSensors(&oneWire);
  dallasSensors.begin();
  
  // Count devices on the bus
  int deviceCount = dallasSensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 sensors.");
  
  // Check if we found at least two sensors
  if (deviceCount < 2) {
    Serial.println("Warning: Less than 2 sensors detected!");
  }
  
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
        
        // Set some test alarm thresholds
        sensors[i]->setLowAlarmThreshold(15);  // Alarm if below 15°C
        sensors[i]->setHighAlarmThreshold(30); // Alarm if above 30°C
      } else {
        Serial.print("Failed to initialize sensor ");
        Serial.println(i);
      }
    }
  }
  
  Serial.println("\nStarting temperature readings...");
  Serial.println("--------------------------------");
}

void loop() {
  // Read temperatures from all sensors
  for (int i = 0; i < 2; i++) {
    if (sensors[i] != nullptr) {
      // Read temperature
      if (sensors[i]->readTemperature()) {
        // Update alarm status based on thresholds
        sensors[i]->updateAlarmStatus();
      }
      
      // Print sensor details
      printSensorDetails(sensors[i]);
    }
  }
  
  Serial.println("--------------------------------");
  delay(2000); // Wait 2 seconds before next reading
}
