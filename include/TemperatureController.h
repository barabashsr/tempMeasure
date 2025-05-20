#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.h"
#include "MeasurementPoint.h"
#include "RegisterMap.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

class TemperatureController {
public:
    TemperatureController(uint8_t oneWirePin = 4);
    ~TemperatureController();

    bool begin();

    MeasurementPoint* getMeasurementPoint(uint8_t address);
    MeasurementPoint* getDS18B20Point(uint8_t idx);
    MeasurementPoint* getPT1000Point(uint8_t idx);

    bool addSensor(Sensor* sensor);
    bool removeSensorByRom(const String& romString);
    Sensor* findSensorByRom(const String& romString);
    Sensor* findSensorByChipSelect(uint8_t csPin);
    int getSensorCount() const { return sensors.size(); }
    Sensor* getSensorByIndex(int idx);

    bool bindSensorToPointByRom(const String& romString, uint8_t pointAddress);
    bool bindSensorToPointByChipSelect(uint8_t csPin, uint8_t pointAddress);
    bool unbindSensorFromPoint(uint8_t pointAddress);
    Sensor* getBoundSensor(uint8_t pointAddress);

    void update();
    void readAllPoints();
    void updateRegisterMap();
    void applyConfigFromRegisterMap();
    void applyConfigToRegisterMap();

    bool discoverDS18B20Sensors();

    String getSensorsJson();
    String getPointsJson();
    String getSystemStatusJson();

    void resetMinMaxValues();

    RegisterMap& getRegisterMap() { return registerMap; }

    void setDeviceId(uint16_t id);
    uint16_t getDeviceId() const;
    void setFirmwareVersion(uint16_t version);
    uint16_t getFirmwareVersion() const;
    void setMeasurementPeriod(uint16_t seconds);
    uint16_t getMeasurementPeriod() const;
    void setOneWireBusPin(uint8_t pin);
    uint8_t getOneWirePin() {return oneWireBusPin;}

    int getDS18B20Count() const;
    int getPT1000Count() const;
    void updateAllSensors();

private:
    MeasurementPoint dsPoints[50];
    MeasurementPoint ptPoints[10];
    std::vector<Sensor*> sensors;
    RegisterMap registerMap;

    uint16_t measurementPeriodSeconds;
    uint16_t deviceId;
    uint16_t firmwareVersion;
    unsigned long lastMeasurementTime;
    bool systemInitialized;
    uint8_t oneWireBusPin;

    bool isDS18B20Address(uint8_t address) const { return address < 50; }
    bool isPT1000Address(uint8_t address) const { return address >= 50 && address < 60; }
};
