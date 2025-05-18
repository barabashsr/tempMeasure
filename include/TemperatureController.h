#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include "Sensor.h"
#include "RegisterMap.h"

class TemperatureController {
private:
    std::vector<Sensor*> sensors;
    RegisterMap registerMap;
    
    // Configuration settings
    uint16_t measurementPeriodSeconds;
    uint16_t deviceId;
    uint16_t firmwareVersion;
    unsigned long lastMeasurementTime;
    bool systemInitialized;
    
    // OneWire bus pin for DS18B20 sensors
    uint8_t oneWireBusPin;

public:
    TemperatureController(uint8_t oneWirePin = 4);
    ~TemperatureController();
    
    // Initialization
    bool begin();
    
    // Sensor management
    bool addSensor(SensorType type, uint8_t address, const String& name);
    bool removeSensor(uint8_t address);
    bool updateSensorAddress(uint8_t oldAddress, uint8_t newAddress);
    bool updateSensorName(uint8_t address, const String& newName);
    Sensor* findSensor(uint8_t address);
    int getSensorCount() const { return sensors.size(); }
    int getDS18B20Count() const;
    int getPT1000Count() const;
    
    // Data collection
    void update(); // Main update function to be called in loop
    void readAllSensors();
    void updateRegisterMap();
    void applyConfigFromRegisterMap();
    
    // Configuration
    void setMeasurementPeriod(uint16_t seconds) { measurementPeriodSeconds = seconds; }
    uint16_t getMeasurementPeriod() const { return measurementPeriodSeconds; }
    void setDeviceId(uint16_t id);
    uint16_t getDeviceId() const { return deviceId; }
    void setFirmwareVersion(uint16_t version);
    uint16_t getFirmwareVersion() const { return firmwareVersion; }
    
    // Sensor discovery
    bool discoverDS18B20Sensors();
    
    // Access to register map for Modbus server
    RegisterMap& getRegisterMap() { return registerMap; }
    
    // JSON representation for web interface
    String getSensorsJson();
    String getSystemStatusJson();
    
    // Reset min/max values for all sensors
    void resetMinMaxValues();

    // Add a pre-configured sensor
    bool addSensor(Sensor* sensor);

    // Get sensor by index
    Sensor* getSensorByIndex(int index);

    void setOneWireBusPin(uint8_t pin) { oneWireBusPin = pin; }
    bool addSensorFromConfig(Sensor* sensor); 
};

#endif // TEMPERATURE_CONTROLLER_H
