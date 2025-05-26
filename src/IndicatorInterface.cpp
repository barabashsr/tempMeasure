#include "IndicatorInterface.h"

// Static instance for interrupt handling
IndicatorInterface* IndicatorInterface::_instance = nullptr;

IndicatorInterface::IndicatorInterface(TwoWire& i2cBus, uint8_t i2cAddress, int intPin)
    : _i2cBus(&i2cBus), _i2cAddress(i2cAddress), _intPin(intPin), _pcf8575(i2cAddress),
      _directionMask(0x0000), _modeMask(0x0000), _currentState(0xFFFF), _lastState(0xFFFF),
      _lastReadTime(0), _pollInterval(50), _interruptFlag(false), _useInterrupts(false),
      _interruptCallback(nullptr) {
    
    // Set static instance for interrupt handling
    _instance = this;
    
    // Configure interrupt usage
    _useInterrupts = (intPin >= 0);
}

IndicatorInterface::~IndicatorInterface() {
    if (_useInterrupts && _intPin >= 0) {
        detachInterrupt(digitalPinToInterrupt(_intPin));
    }
    _instance = nullptr;
}

bool IndicatorInterface::begin() {
    // Initialize I2C if not already done
    if (!_i2cBus) {
        return false;
    }
    
    // Initialize PCF8575
    if (!_pcf8575.begin()) {
        return false;
    }
    
    // Configure interrupt pin if specified
    if (_useInterrupts) {
        _configureInterruptPin();
    }
    
    // Initialize all pins as inputs (HIGH state)
    _pcf8575.write16(0xFFFF);
    delay(100);
    _clearInterrupt();
    
    // Read initial state
    _currentState = _readPCF();
    _lastState = _currentState;
    _lastReadTime = millis();
    
    return true;
}

void IndicatorInterface::_configureInterruptPin() {
    if (_intPin < 0) return;
    
    // Configure pin based on ESP32 capabilities
    if (_intPin == 34 || _intPin == 35 || _intPin == 36 || _intPin == 39) {
        // Input-only pins without internal pull-up
        pinMode(_intPin, INPUT);
        // Note: External pull-up resistor (4.7kΩ) required!
    } else {
        // Regular GPIO pins with internal pull-up
        pinMode(_intPin, INPUT_PULLUP);
    }
    
    // Attach interrupt
    attachInterrupt(digitalPinToInterrupt(_intPin), _staticInterruptHandler, FALLING);
}

void IndicatorInterface::setDirection(uint16_t directionMask) {
    _directionMask = directionMask;
    
    // Update PCF8575 state to reflect direction changes
    uint16_t newState = _currentState;
    
    // Set input pins HIGH (input mode for PCF8575)
    for (int i = 0; i < 16; i++) {
        if (!isOutput(i)) {
            newState |= (1 << i);
        }
    }
    
    _writePCF(newState);
}

void IndicatorInterface::setMode(uint16_t modeMask) {
    _modeMask = modeMask;
}

void IndicatorInterface::setPortNames(const std::map<std::string, uint8_t>& portNames) {
    _portNames = portNames;
    
    // Build reverse mapping
    _portNumbers.clear();
    for (const auto& pair : _portNames) {
        _portNumbers[pair.second] = pair.first;
    }
}

void IndicatorInterface::setPortName(const std::string& name, uint8_t portNumber) {
    if (portNumber > 15) return;
    
    _portNames[name] = portNumber;
    _portNumbers[portNumber] = name;
}

bool IndicatorInterface::writePort(const std::string& portName, bool state) {
    auto it = _portNames.find(portName);
    if (it == _portNames.end()) {
        return false;
    }
    return writePort(it->second, state);
}

bool IndicatorInterface::writePort(uint8_t portNumber, bool state) {
    if (portNumber > 15 || !isOutput(portNumber)) {
        return false;
    }
    
    // Apply mode logic (inversion if needed)
    bool actualState = _applyModeLogic(portNumber, state);
    
    // Update current state
    uint16_t newState = _currentState;
    
    // Always keep input pins HIGH
    for (int i = 0; i < 16; i++) {
        if (!isOutput(i)) {
            newState |= (1 << i);
        }
    }
    
    // Set the specific output pin
    if (actualState) {
        newState |= (1 << portNumber);
    } else {
        newState &= ~(1 << portNumber);
    }
    
    _writePCF(newState);
    return true;
}

void IndicatorInterface::writePorts(uint16_t portMask) {
    uint16_t newState = _currentState;
    
    // Apply direction mask - only write to outputs
    for (int i = 0; i < 16; i++) {
        if (isOutput(i)) {
            bool state = (portMask >> i) & 0x01;
            bool actualState = _applyModeLogic(i, state);
            
            if (actualState) {
                newState |= (1 << i);
            } else {
                newState &= ~(1 << i);
            }
        } else {
            // Keep input pins HIGH
            newState |= (1 << i);
        }
    }
    
    _writePCF(newState);
}

void IndicatorInterface::setAllOutputs(bool state) {
    uint16_t newState = _currentState;
    
    for (int i = 0; i < 16; i++) {
        if (isOutput(i)) {
            bool actualState = _applyModeLogic(i, state);
            
            if (actualState) {
                newState |= (1 << i);
            } else {
                newState &= ~(1 << i);
            }
        } else {
            // Keep input pins HIGH
            newState |= (1 << i);
        }
    }
    
    _writePCF(newState);
}

void IndicatorInterface::setAllOutputsHigh() {
    setAllOutputs(true);
}

void IndicatorInterface::setAllOutputsLow() {
    setAllOutputs(false);
}

uint16_t IndicatorInterface::getCurrentState() {
    if (_useInterrupts) {
        // In interrupt mode, state is updated automatically
        return _currentState;
    } else {
        // In polling mode, read current state
        if (millis() - _lastReadTime >= _pollInterval) {
            _updateState();
        }
        return _currentState;
    }
}

bool IndicatorInterface::readPort(const std::string& portName) {
    auto it = _portNames.find(portName);
    if (it == _portNames.end()) {
        return false;
    }
    return readPort(it->second);
}

bool IndicatorInterface::readPort(uint8_t portNumber) {
    if (portNumber > 15) {
        return false;
    }
    
    uint16_t currentState = getCurrentState();
    bool rawState = (currentState >> portNumber) & 0x01;
    
    // Apply mode logic (reverse inversion for reading)
    return _reverseModeLogic(portNumber, rawState);
}

bool IndicatorInterface::isOutput(uint8_t portNumber) {
    return (_directionMask >> portNumber) & 0x01;
}

bool IndicatorInterface::isInput(uint8_t portNumber) {
    return !isOutput(portNumber);
}

bool IndicatorInterface::isInverted(uint8_t portNumber) {
    return (_modeMask >> portNumber) & 0x01;
}

uint8_t IndicatorInterface::getPortNumber(const std::string& portName) {
    auto it = _portNames.find(portName);
    return (it != _portNames.end()) ? it->second : 255;
}

std::string IndicatorInterface::getPortName(uint8_t portNumber) {
    auto it = _portNumbers.find(portNumber);
    return (it != _portNumbers.end()) ? it->second : "";
}

void IndicatorInterface::handleInterrupt() {
    if (_interruptFlag) {
        _interruptFlag = false;
        _updateState();
    }
}

void IndicatorInterface::setInterruptCallback(void (*callback)(uint16_t currentState, uint16_t changedPins)) {
    _interruptCallback = callback;
}

void IndicatorInterface::printPortStates() {
    uint16_t state = getCurrentState();
    
    Serial.println("=== Port States ===");
    Serial.print("Raw state: 0x");
    Serial.println(state, HEX);
    
    for (int i = 15; i >= 0; i--) {
        bool rawState = (state >> i) & 0x01;
        bool logicalState = _reverseModeLogic(i, rawState);
        
        Serial.print("P");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(rawState ? "HIGH" : "LOW");
        Serial.print(" (");
        Serial.print(isOutput(i) ? "OUT" : "IN");
        if (isInverted(i)) Serial.print(",INV");
        Serial.print(") = ");
        Serial.print(logicalState ? "TRUE" : "FALSE");
        
        std::string name = getPortName(i);
        if (!name.empty()) {
            Serial.print(" [");
            Serial.print(name.c_str());
            Serial.print("]");
        }
        Serial.println();
    }
}

void IndicatorInterface::printConfiguration() {
    Serial.println("=== Configuration ===");
    Serial.print("I2C Address: 0x");
    Serial.println(_i2cAddress, HEX);
    Serial.print("INT Pin: ");
    Serial.println(_intPin);
    Serial.print("Use Interrupts: ");
    Serial.println(_useInterrupts ? "YES" : "NO");
    Serial.print("Direction Mask: 0x");
    Serial.println(_directionMask, HEX);
    Serial.print("Mode Mask: 0x");
    Serial.println(_modeMask, HEX);
    
    Serial.println("Port Names:");
    for (const auto& pair : _portNames) {
        Serial.print("  ");
        Serial.print(pair.first.c_str());
        Serial.print(" = P");
        Serial.println(pair.second);
    }
}

// Private methods
void IndicatorInterface::_updateState() {
    uint16_t newState = _readPCF();
    uint16_t changedPins = _currentState ^ newState;
    
    _lastState = _currentState;
    _currentState = newState;
    _lastReadTime = millis();
    
    // Call interrupt callback if pins changed
    if (changedPins != 0 && _interruptCallback) {
        _interruptCallback(_currentState, changedPins);
    }
}

void IndicatorInterface::_clearInterrupt() {
    _pcf8575.read16();
    delay(1);
    _pcf8575.read16();
}

uint16_t IndicatorInterface::_readPCF() {
    return _pcf8575.read16();
}

void IndicatorInterface::_writePCF(uint16_t state) {
    _pcf8575.write16(state);
    delay(5);
    _clearInterrupt();
    _currentState = state;
}

bool IndicatorInterface::_applyModeLogic(uint8_t portNumber, bool state) {
    // Fixed: Normal mode = no inversion, Inverted mode = invert
    return isInverted(portNumber) ? !state : state;
}

bool IndicatorInterface::_reverseModeLogic(uint8_t portNumber, bool state) {
    // Fixed: For reading, apply same logic as writing
    return isInverted(portNumber) ? !state : state;
}

void IRAM_ATTR IndicatorInterface::_staticInterruptHandler() {
    if (_instance) {
        _instance->_interruptFlag = true;
    }
}


void IndicatorInterface::setPortInverted(const std::string& portName, bool inverted) {
    auto it = _portNames.find(portName);
    if (it != _portNames.end()) {
        setPortInverted(it->second, inverted);
    }
}

void IndicatorInterface::setPortInverted(uint8_t portNumber, bool inverted) {
    if (portNumber > 15) return;
    
    if (inverted) {
        _modeMask |= (1 << portNumber);   // Set bit to 1 for inverted
    } else {
        _modeMask &= ~(1 << portNumber);  // Set bit to 0 for normal
    }
}