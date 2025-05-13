#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MAX31865.h>

enum class SensorType {
    DS18B20,
    PT1000
};

class Sensor {
private:
    uint8_t address;       // Modbus register address
    String name;           // Equipment name
    SensorType type;
    int16_t currentTemp;   // Current temperature in Celsius
    int16_t minTemp;       // Minimum recorded temperature
    int16_t maxTemp;       // Maximum recorded temperature
    uint16_t alarmStatus;  // Bit flags for alarms
    uint16_t errorStatus;  // Bit flags for errors
    int16_t lowAlarmThreshold;
    int16_t highAlarmThreshold;
    
    // Physical connection details
    union {
        struct {
            uint8_t oneWirePin;
            uint8_t oneWireAddress[8];
        } ds18b20;
        
        struct {
            uint8_t csPin;
            uint8_t maxAddress;
        } pt1000;
    } connection;

    // References to sensor libraries
    OneWire* oneWire;
    DallasTemperature* dallasTemperature;
    Adafruit_MAX31865* max31865;
    
    // Helper methods
    void calculateModbusRegisters();
    uint16_t currentTempRegister;
    uint16_t minTempRegister;
    uint16_t maxTempRegister;
    uint16_t alarmStatusRegister;
    uint16_t errorStatusRegister;
    uint16_t lowAlarmThresholdRegister;
    uint16_t highAlarmThresholdRegister;

public:
    Sensor(SensorType type, uint8_t address, const String& name);
    ~Sensor();
    
    bool initialize();
    bool readTemperature();
    void updateAlarmStatus();
    void resetMinMaxTemp();
    
    // DS18B20 specific setup
    void setupDS18B20(uint8_t pin, const uint8_t* deviceAddress);
    
    // PT1000 specific setup
    void setupPT1000(uint8_t csPin, uint8_t maxAddress);
    
    // Getters and setters
    uint8_t getAddress() const { return address; }
    void setAddress(uint8_t newAddress);
    String getName() const { return name; }
    void setName(const String& newName) { name = newName; }
    int16_t getCurrentTemp() const { return currentTemp; }
    int16_t getMinTemp() const { return minTemp; }
    int16_t getMaxTemp() const { return maxTemp; }
    uint16_t getAlarmStatus() const { return alarmStatus; }
    uint16_t getErrorStatus() const { return errorStatus; }
    int16_t getLowAlarmThreshold() const { return lowAlarmThreshold; }
    void setLowAlarmThreshold(int16_t threshold);
    int16_t getHighAlarmThreshold() const { return highAlarmThreshold; }
    void setHighAlarmThreshold(int16_t threshold);
    SensorType getType() const { return type; }
    
    // Modbus register getters
    uint16_t getCurrentTempRegister() const { return currentTempRegister; }
    uint16_t getMinTempRegister() const { return minTempRegister; }
    uint16_t getMaxTempRegister() const { return maxTempRegister; }
    uint16_t getAlarmStatusRegister() const { return alarmStatusRegister; }
    uint16_t getErrorStatusRegister() const { return errorStatusRegister; }
    uint16_t getLowAlarmThresholdRegister() const { return lowAlarmThresholdRegister; }
    uint16_t getHighAlarmThresholdRegister() const { return highAlarmThresholdRegister; }
    
    // Error status bit flags
    static const uint16_t ERROR_COMMUNICATION = 0x0001;
    static const uint16_t ERROR_OUT_OF_RANGE = 0x0002;
    static const uint16_t ERROR_DISCONNECTED = 0x0004;
    
    // Alarm status bit flags
    static const uint16_t ALARM_LOW_TEMP = 0x0001;
    static const uint16_t ALARM_HIGH_TEMP = 0x0002;
    
    // Get DS18B20 ROM address
    const uint8_t* getDS18B20Address() const {
        if (type == SensorType::DS18B20) {
            return connection.ds18b20.oneWireAddress;
        }
        return nullptr;
    }
    };

#endif // SENSOR_H
