#include "Sensor.h"

Sensor::Sensor(SensorType type, uint8_t address, const String& name)
    : address(address), name(name), type(type), 
      currentTemp(0), minTemp(32767), maxTemp(-32768),
      alarmStatus(0), errorStatus(0),
      lowAlarmThreshold(-40), highAlarmThreshold(85),
      oneWire(nullptr), dallasTemperature(nullptr), max31865(nullptr) {
    
    // Initialize the connection union based on sensor type
    if (type == SensorType::DS18B20) {
        connection.ds18b20.oneWirePin = 0;
        memset(connection.ds18b20.oneWireAddress, 0, 8);
    } else {
        connection.pt1000.csPin = 0;
        connection.pt1000.maxAddress = 0;
    }
    
    calculateModbusRegisters();
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

void Sensor::calculateModbusRegisters() {
    // Based on README file, map the sensor address to the appropriate Modbus registers
    if (type == SensorType::DS18B20) {
        // DS18B20 sensors use addresses 0-49
        if (address < 50) {
            currentTempRegister = 100 + address;
            minTempRegister = 200 + address;
            maxTempRegister = 300 + address;
            alarmStatusRegister = 400 + address;
            errorStatusRegister = 500 + address;
            lowAlarmThresholdRegister = 600 + address;
            highAlarmThresholdRegister = 700 + address;
        }
    } else if (type == SensorType::PT1000) {
        // PT1000 sensors use addresses 50-59
        if (address >= 50 && address < 60) {
            uint8_t offset = address - 50;
            currentTempRegister = 150 + offset;
            minTempRegister = 250 + offset;
            maxTempRegister = 350 + offset;
            alarmStatusRegister = 450 + offset;
            errorStatusRegister = 550 + offset;
            lowAlarmThresholdRegister = 650 + offset;
            highAlarmThresholdRegister = 750 + offset;
        }
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
        
        // Set resolution to 12 bits (0.0625°C)
        DeviceAddress deviceAddress;
        memcpy(deviceAddress, connection.ds18b20.oneWireAddress, 8);
        dallasTemperature->setResolution(deviceAddress, 12);
        bool success = dallasTemperature->isConnected(deviceAddress);
        Serial.printf("Device ROM: %d, device status: %d\n", deviceAddress[7], success);
        
        return success;//dallasTemperature->isConnected(deviceAddress);
    } 
    else if (type == SensorType::PT1000) {
        max31865 = new Adafruit_MAX31865(connection.pt1000.csPin);
        // Initialize the MAX31865 for PT1000 sensors (RTD_PT1000)
        return max31865->begin(MAX31865_3WIRE);  // Adjust based on your wiring (2, 3, or 4 wire)
    }
    
    return false;
}

bool Sensor::readTemperature() {
    Serial.println("Reading sensors");
    float tempC = 0.0;
    bool success = false;
    
    // Clear previous error flags
    errorStatus &= ~(ERROR_COMMUNICATION | ERROR_OUT_OF_RANGE | ERROR_DISCONNECTED);
    
    if (type == SensorType::DS18B20) {
        Serial.println("Reading DS18B20 sensors");
        if (dallasTemperature != nullptr) {
            DeviceAddress deviceAddress;
            memcpy(deviceAddress, connection.ds18b20.oneWireAddress, 8);
            
            if (dallasTemperature->isConnected(deviceAddress)) {
                dallasTemperature->requestTemperaturesByAddress(deviceAddress);
                //delay(750);
                tempC = dallasTemperature->getTempC(deviceAddress);
                Serial.printf("Temperature: %f", tempC);
                Serial.println(errorStatus);
                
                if (tempC != DEVICE_DISCONNECTED_C) {
                    success = true;
                    Serial.println(success);
                } else {
                    errorStatus |= ERROR_DISCONNECTED;
                }
            } else {
                errorStatus |= ERROR_COMMUNICATION;
            }
        }
    } 
    else if (type == SensorType::PT1000) {
        if (max31865 != nullptr) {
            uint8_t fault = max31865->readFault();
            
            if (fault) {
                // Handle MAX31865 faults
                if (fault & MAX31865_FAULT_HIGHTHRESH) {
                    errorStatus |= ERROR_OUT_OF_RANGE;
                }
                if (fault & MAX31865_FAULT_LOWTHRESH) {
                    errorStatus |= ERROR_OUT_OF_RANGE;
                }
                if (fault & MAX31865_FAULT_REFINLOW || 
                    fault & MAX31865_FAULT_REFINHIGH || 
                    fault & MAX31865_FAULT_RTDINLOW || 
                    fault & MAX31865_FAULT_OVUV) {
                    errorStatus |= ERROR_COMMUNICATION;
                }
                
                max31865->clearFault();
            } else {
                tempC = max31865->temperature(100.0, 430.0); // PT1000 has 1000 ohm at 0°C, adjust reference resistor value as needed
                success = true;
            }
        }
    }
    
    if (success) {
        // Check if temperature is within valid range (-40°C to 200°C)
        if (tempC < -40.0 || tempC > 200.0) {
            errorStatus |= ERROR_OUT_OF_RANGE;
        } else {
            // Convert to int16_t (fixed-point with no decimal places)
            currentTemp = static_cast<int16_t>(round(tempC));
            
            // Update min/max temperatures
            if (currentTemp < minTemp) {
                minTemp = currentTemp;
            }
            if (currentTemp > maxTemp) {
                maxTemp = currentTemp;
            }
        }
    }
    
    return success;
}

void Sensor::updateAlarmStatus() {
    // Clear previous alarm flags
    alarmStatus &= ~(ALARM_LOW_TEMP | ALARM_HIGH_TEMP);
    
    // Only check alarms if there are no errors
    if (errorStatus == 0) {
        // Check low temperature alarm
        if (currentTemp < lowAlarmThreshold) {
            alarmStatus |= ALARM_LOW_TEMP;
        }
        
        // Check high temperature alarm
        if (currentTemp > highAlarmThreshold) {
            alarmStatus |= ALARM_HIGH_TEMP;
        }
    }
}

void Sensor::resetMinMaxTemp() {
    minTemp = currentTemp;
    maxTemp = currentTemp;
}

void Sensor::setAddress(uint8_t newAddress) {
    address = newAddress;
    calculateModbusRegisters();
}

void Sensor::setLowAlarmThreshold(int16_t threshold) {
    lowAlarmThreshold = threshold;
    // Update alarm status after changing threshold
    updateAlarmStatus();
}

void Sensor::setHighAlarmThreshold(int16_t threshold) {
    highAlarmThreshold = threshold;
    // Update alarm status after changing threshold
    updateAlarmStatus();
}

    // Get DS18B20 ROM address
    const uint8_t* Sensor::getDS18B20Address() const {
        if (type == SensorType::DS18B20) {
            return connection.ds18b20.oneWireAddress;
        }
        return nullptr;
    }
    
