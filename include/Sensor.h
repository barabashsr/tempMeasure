#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MAX31865.h>

// Define your sensor types
enum class SensorType {
    DS18B20,
    PT1000
};

// Error and alarm bitmasks
constexpr uint8_t ERROR_COMMUNICATION = 0x01;
constexpr uint8_t ERROR_OUT_OF_RANGE  = 0x02;
constexpr uint8_t ERROR_DISCONNECTED  = 0x04;

constexpr uint8_t ALARM_LOW_TEMP  = 0x01;
constexpr uint8_t ALARM_HIGH_TEMP = 0x02;

class Sensor {
public:
    Sensor(SensorType type, uint8_t address, const String& name);
    ~Sensor();

    // Setup methods for each sensor type
    void setupDS18B20(uint8_t pin, const uint8_t* deviceAddress);
    void setupPT1000(uint8_t csPin, uint8_t maxAddress);

    // Initialize hardware
    bool initialize();

    // Read temperature from the sensor, update internal state
    bool readTemperature();

    // Accessors
    SensorType getType() const;
    uint8_t getAddress() const;
    String getName() const;
    void setName(const String& newName);

    int16_t getCurrentTemp() const;
    int16_t getMinTemp() const;
    int16_t getMaxTemp() const;
    int16_t getLowAlarmThreshold() const;
    int16_t getHighAlarmThreshold() const;
    uint8_t getAlarmStatus() const;
    uint8_t getErrorStatus() const;
    uint8_t getPT1000ChipSelectPin() const;
    

    void setAddress(uint8_t newAddress);
    void setLowAlarmThreshold(int16_t threshold);
    void setHighAlarmThreshold(int16_t threshold);

    // DS18B20 ROM address accessor
    const uint8_t* getDS18B20Address() const;

    // Reset min/max values
    void resetMinMaxTemp();

    // Update alarm status (call after reading temperature or changing thresholds)
    void updateAlarmStatus();
    String getDS18B20RomString() const;        // ROM as hex string
    void getDS18B20RomArray(uint8_t out[8]) const; // ROM as array
    uint8_t getOneWirePin() {return connection.ds18b20.oneWirePin;}

private:
    uint8_t address;
    String name;
    SensorType type;

    int16_t currentTemp;
    int16_t minTemp;
    int16_t maxTemp;
    int16_t lowAlarmThreshold;
    int16_t highAlarmThreshold;
    uint8_t alarmStatus;
    uint8_t errorStatus;

    // Hardware-specific members
    OneWire* oneWire;
    DallasTemperature* dallasTemperature;
    Adafruit_MAX31865* max31865;

    // Connection details
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
};

#endif // SENSOR_H
