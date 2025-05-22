#include "Sensor.h"

//SPI PINs
// #define SCK_PIN  14
// #define MISO_PIN  12
// #define MOSI_PIN  13

Sensor::Sensor(SensorType type, uint8_t address, const String& name)
    : address(address), name(name), type(type),
      currentTemp(0), minTemp(32767), maxTemp(-32768),
      lowAlarmThreshold(-40), highAlarmThreshold(85),
      alarmStatus(0), errorStatus(0),
      oneWire(nullptr), dallasTemperature(nullptr), max31865(nullptr)
{
    if (type == SensorType::DS18B20) {
        connection.ds18b20.oneWirePin = 0;
        memset(connection.ds18b20.oneWireAddress, 0, 8);
    } else {
        connection.pt1000.csPin = 0;
        connection.pt1000.maxAddress = 0;
    }
}

Sensor::~Sensor() {
    if (oneWire != nullptr) {
        delete oneWire;
        oneWire = nullptr;
    }
    if (dallasTemperature != nullptr) {
        delete dallasTemperature;
        dallasTemperature = nullptr;
    }
    if (max31865 != nullptr) {
        delete max31865;
        max31865 = nullptr;
    }
}

void Sensor::setupDS18B20(uint8_t pin, const uint8_t* deviceAddress) {
    connection.ds18b20.oneWirePin = pin;
    memcpy(connection.ds18b20.oneWireAddress, deviceAddress, 8);
}

void Sensor::setupPT1000(uint8_t csPin, uint8_t maxAddress) {
    connection.pt1000.csPin = csPin;
    connection.pt1000.maxAddress = maxAddress;
}

bool Sensor::initialize() {
    if (type == SensorType::DS18B20) {
        oneWire = new OneWire(connection.ds18b20.oneWirePin);
        dallasTemperature = new DallasTemperature(oneWire);
        dallasTemperature->begin();
        DeviceAddress deviceAddress;
        memcpy(deviceAddress, connection.ds18b20.oneWireAddress, 8);
        dallasTemperature->setResolution(deviceAddress, 12);
        return dallasTemperature->isConnected(deviceAddress);
    } else if (type == SensorType::PT1000) {
        max31865 = new Adafruit_MAX31865(connection.pt1000.csPin);
        
        // Try to begin the MAX31865
        bool beginSuccess = max31865->begin(MAX31865_3WIRE); // Adjust for your wiring
        
        // if (!beginSuccess) {
        //     errorStatus |= ERROR_COMMUNICATION;
        //     return false;
        // }
        
        // // Check for faults immediately after initialization
        // uint8_t fault = max31865->readFault();
        // if (fault) {
        //     max31865->clearFault();
        //     errorStatus |= ERROR_COMMUNICATION;
        //     return false;
        // }
        
        // // Read RTD value to verify communication
        // uint16_t rtd = max31865->readRTD();
        // if (rtd == 0 || rtd == 0xFFFF) {  // Common values when module is not connected
        //     errorStatus |= ERROR_COMMUNICATION;
        //     // return false;
        // }
        
        // // Calculate resistance to check if it's within reasonable range for PT1000
        // float ratio = rtd / 32768.0;
        // float resistance = 4300.0 * ratio;  // Using RREF of 4300 for PT1000
        
        // // PT1000 should be roughly 1000 ohms at 0°C, with reasonable range between 800-1400 ohms
        // // for normal temperature measurements (-50°C to +100°C)
        // if (resistance < 800.0 || resistance > 2200.0) {
        //     errorStatus |= ERROR_DISCONNECTED;
        //     // return false;
        // }
        
        return true;
    }
    return false;
}

bool Sensor::readTemperature() {
    float tempC = 0.0;
    bool success = false;
    errorStatus &= ~(ERROR_COMMUNICATION | ERROR_OUT_OF_RANGE | ERROR_DISCONNECTED);

    if (type == SensorType::DS18B20) {
        if (dallasTemperature != nullptr) {
            DeviceAddress deviceAddress;
            memcpy(deviceAddress, connection.ds18b20.oneWireAddress, 8);
            if (dallasTemperature->isConnected(deviceAddress)) {
                dallasTemperature->requestTemperaturesByAddress(deviceAddress);
                tempC = dallasTemperature->getTempC(deviceAddress);
                if (tempC != DEVICE_DISCONNECTED_C) {
                    success = true;
                } else {
                    errorStatus |= ERROR_DISCONNECTED;
                }
            } else {
                errorStatus |= ERROR_COMMUNICATION;
            }
        }
    } else if (type == SensorType::PT1000) {
        if (max31865 != nullptr) {
            uint8_t fault = max31865->readFault();
            // if (fault) {
            //     // if (fault & MAX31865_FAULT_HIGHTHRESH || fault & MAX31865_FAULT_LOWTHRESH) {
            //     //     errorStatus |= ERROR_OUT_OF_RANGE;
            //     // }
                if (fault & (MAX31865_FAULT_REFINLOW | MAX31865_FAULT_REFINHIGH |
                             MAX31865_FAULT_RTDINLOW | MAX31865_FAULT_OVUV)) {
                    errorStatus |= ERROR_COMMUNICATION;
                    max31865->clearFault();
                
                } else {
                    tempC = max31865->temperature(1000.0, 4300.0); // PT1000: 1000 ohm at 0°C, adjust reference as needed
                    success = true;
                }
        }
    }

    if (success) {
        if (tempC < -40.0 || tempC > 200.0) {
            errorStatus |= ERROR_OUT_OF_RANGE;
        } else {
            currentTemp = static_cast<int16_t>(round(tempC));
            if (currentTemp < minTemp) minTemp = currentTemp;
            if (currentTemp > maxTemp) maxTemp = currentTemp;
        }
    }
    updateAlarmStatus();
    return success;
}

SensorType Sensor::getType() const { return type; }
uint8_t Sensor::getAddress() const { return address; }
String Sensor::getName() const { return name; }
void Sensor::setName(const String& newName) { name = newName; }

int16_t Sensor::getCurrentTemp() const {return currentTemp; }
int16_t Sensor::getMinTemp() const { return minTemp; }
int16_t Sensor::getMaxTemp() const { return maxTemp; }
int16_t Sensor::getLowAlarmThreshold() const { return lowAlarmThreshold; }
int16_t Sensor::getHighAlarmThreshold() const { return highAlarmThreshold; }
uint8_t Sensor::getAlarmStatus() const { return alarmStatus; }
uint8_t Sensor::getErrorStatus() const { return errorStatus; }

void Sensor::setAddress(uint8_t newAddress) { address = newAddress; }
void Sensor::setLowAlarmThreshold(int16_t threshold) { lowAlarmThreshold = threshold; updateAlarmStatus(); }
void Sensor::setHighAlarmThreshold(int16_t threshold) { highAlarmThreshold = threshold; updateAlarmStatus(); }

const uint8_t* Sensor::getDS18B20Address() const {
    if (type == SensorType::DS18B20) {
        return connection.ds18b20.oneWireAddress;
    }
    return nullptr;
}

void Sensor::resetMinMaxTemp() {
    minTemp = currentTemp;
    maxTemp = currentTemp;
}

void Sensor::updateAlarmStatus() {
    alarmStatus = 0;
    if (errorStatus != 0) return;
    if (currentTemp < lowAlarmThreshold) alarmStatus |= ALARM_LOW_TEMP;
    if (currentTemp > highAlarmThreshold) alarmStatus |= ALARM_HIGH_TEMP;
}


uint8_t Sensor::getPT1000ChipSelectPin() const {
    if (type == SensorType::PT1000) return connection.pt1000.csPin;
    return 0;
}

String Sensor::getDS18B20RomString() const {
    if (type != SensorType::DS18B20) return "";
    char buf[17]; // 16 hex chars + null
    const uint8_t* rom = connection.ds18b20.oneWireAddress;
    for (int i = 0; i < 8; ++i) {
        sprintf(buf + i*2, "%02X", rom[i]);
    }
    return String(buf);
}

void Sensor::getDS18B20RomArray(uint8_t out[8]) const {
    if (type == SensorType::DS18B20) {
        memcpy(out, connection.ds18b20.oneWireAddress, 8);
    }
}