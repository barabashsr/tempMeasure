#ifndef MEASUREMENT_POINT_H
#define MEASUREMENT_POINT_H

#include <Arduino.h>
#include "Sensor.h"
#include "LoggerManager.h"



class MeasurementPoint {
public:
    // Constructor
    MeasurementPoint() : address(0), name(""), currentTemp(0), minTemp(32767), maxTemp(-32768),
    lowAlarmThreshold(-10), highAlarmThreshold(50), alarmStatus(0), errorStatus(0), boundSensor(nullptr) {}
    MeasurementPoint(uint8_t address, const String& name);

    // Destructor
    ~MeasurementPoint();

    // Getters
    uint8_t getAddress() const;
    String getName() const;
    int16_t getCurrentTemp() const;
    int16_t getMinTemp() const;
    int16_t getMaxTemp() const;
    int16_t getLowAlarmThreshold() const;
    int16_t getHighAlarmThreshold() const;
    uint8_t getAlarmStatus() const;
    uint8_t getErrorStatus() const;

    // Setters
    void setName(const String& newName);
    void setLowAlarmThreshold(int16_t threshold);
    void setHighAlarmThreshold(int16_t threshold);

    // Sensor binding (optional)
    void bindSensor(Sensor* sensor);
    void unbindSensor();
    Sensor* getBoundSensor() const;

    // Operations
    void update();              // Should be called to refresh temperature and status
    void resetMinMaxTemp();     // Resets min/max to current
    // void setOneWireBus(uint8_t bus);
    // uint8_t getOneWireBus();

private:
    uint8_t address;
    String name;
    // uint8_t oneWireBus;

    int16_t currentTemp;        // Latest temperature (°C x1)
    int16_t minTemp;            // Minimum recorded temperature
    int16_t maxTemp;            // Maximum recorded temperature
    int16_t lowAlarmThreshold;  // Alarm threshold low
    int16_t highAlarmThreshold; // Alarm threshold high
    uint8_t alarmStatus;        // Alarm status bits
    uint8_t errorStatus;        // Error status bits

    Sensor* boundSensor;        // Pointer to bound sensor, or nullptr

    void updateAlarmStatus();
};


#endif // TEMPERATURE_CONTROLLER_H
