#ifndef INDICATOR_INTERFACE_H
#define INDICATOR_INTERFACE_H

#include <Arduino.h>
#include <Wire.h>
#include <map>
#include <string>
#include "PCF8575.h"

class IndicatorInterface {
public:
    // Constructor
    IndicatorInterface(TwoWire& i2cBus, uint8_t i2cAddress, int intPin = -1);
    
    // Destructor
    ~IndicatorInterface();
    
    // Initialization
    bool begin();
    
    // Configuration setters
    void setDirection(uint16_t directionMask);          // 0 = input, 1 = output
    void setMode(uint16_t modeMask);                    // 0 = normal, 1 = inverted
    void setPortNames(const std::map<std::string, uint8_t>& portNames);
    void setPortName(const std::string& name, uint8_t portNumber);
    
    // Port control methods
    bool writePort(const std::string& portName, bool state);
    bool writePort(uint8_t portNumber, bool state);
    void writePorts(uint16_t portMask);                 // Write only to outputs
    void setAllOutputs(bool state);
    void setAllOutputsHigh();
    void setAllOutputsLow();
    
    void setPortInverted(const std::string& portName, bool inverted);
    void setPortInverted(uint8_t portNumber, bool inverted);

    
    // Port reading methods
    uint16_t getCurrentState();
    bool readPort(const std::string& portName);
    bool readPort(uint8_t portNumber);
    
    // Utility methods
    bool isOutput(uint8_t portNumber);
    bool isInput(uint8_t portNumber);
    bool isInverted(uint8_t portNumber);
    uint8_t getPortNumber(const std::string& portName);
    std::string getPortName(uint8_t portNumber);
    
    // Interrupt handling
    void handleInterrupt();
    void setInterruptCallback(void (*callback)(uint16_t currentState, uint16_t changedPins));
    
    // Debug methods
    void printPortStates();
    void printConfiguration();

private:
    // Hardware configuration
    TwoWire* _i2cBus;
    uint8_t _i2cAddress;
    int _intPin;
    PCF8575 _pcf8575;
    
    // Port configuration
    uint16_t _directionMask;    // 0 = input, 1 = output
    uint16_t _modeMask;         // 0 = normal, 1 = inverted
    std::map<std::string, uint8_t> _portNames;
    std::map<uint8_t, std::string> _portNumbers;
    
    // State tracking
    uint16_t _currentState;
    uint16_t _lastState;
    unsigned long _lastReadTime;
    unsigned long _pollInterval;
    
    // Interrupt handling
    volatile bool _interruptFlag;
    bool _useInterrupts;
    void (*_interruptCallback)(uint16_t currentState, uint16_t changedPins);
    
    // Internal methods
    void _updateState();
    void _clearInterrupt();
    uint16_t _readPCF();
    void _writePCF(uint16_t state);
    bool _applyModeLogic(uint8_t portNumber, bool state);
    bool _reverseModeLogic(uint8_t portNumber, bool state);
    void _configureInterruptPin();
    
    // Static interrupt handler
    static IndicatorInterface* _instance;
    static void IRAM_ATTR _staticInterruptHandler();
};

#endif
