/**
 * @file TempModbusServer.h
 * @brief Modbus RTU server for temperature monitoring system
 * @author Claude Code Session 20250720_221011
 * @date 2025-01-20
 * @details Implements a Modbus RTU server that provides access to temperature
 *          data and configuration through standard Modbus function codes.
 * 
 * @section dependencies Dependencies
 * - ModbusServerRTU for Modbus protocol implementation
 * - RegisterMap for register addressing and data storage
 * - LoggerManager for logging Modbus transactions
 * 
 * @section hardware Hardware Requirements
 * - RS485 transceiver for Modbus RTU communication
 * - UART/Serial port for RTU communication
 * - DE (Driver Enable) pin for RS485 direction control
 */

#ifndef TEMP_MODBUS_SERVER_H
#define TEMP_MODBUS_SERVER_H

#include <Arduino.h>
#include "ModbusServerRTU.h"
#include "RegisterMap.h"
#include "LoggerManager.h"

/**
 * @class TempModbusServer
 * @brief Modbus RTU server implementation for temperature monitoring
 * @details Provides Modbus RTU communication interface for reading temperature
 *          values, status information, and configuring alarm thresholds.
 *          Supports function codes 03 (Read Holding Registers), 06 (Write Single
 *          Register), and 16 (Write Multiple Registers).
 */
class TempModbusServer {
private:
    ModbusServerRTU* mbServer;          ///< Modbus RTU server instance
    RegisterMap& registerMap;           ///< Reference to register map
    uint8_t serverID;                   ///< Modbus server ID (1-247)
    HardwareSerial& serial;             ///< Hardware serial port reference
    int rxPin;                          ///< UART RX pin
    int txPin;                          ///< UART TX pin
    int dePin;                          ///< RS485 Driver Enable pin
    int baudRate;                       ///< Serial baud rate
    
    // Worker functions for different Modbus function codes
    /**
     * @brief Worker function for Read Holding Registers (FC03)
     * @param[in] request Modbus request message
     * @return ModbusMessage Response message with register data
     * @note Static function called by Modbus library
     */
    static ModbusMessage readHoldingRegistersWorker(ModbusMessage request);
    
    /**
     * @brief Worker function for Write Single Register (FC06)
     * @param[in] request Modbus request message
     * @return ModbusMessage Response message with confirmation
     * @note Static function called by Modbus library
     */
    static ModbusMessage writeHoldingRegisterWorker(ModbusMessage request);
    
    /**
     * @brief Worker function for Write Multiple Registers (FC16)
     * @param[in] request Modbus request message
     * @return ModbusMessage Response message with confirmation
     * @note Static function called by Modbus library
     */
    static ModbusMessage writeMultipleRegistersWorker(ModbusMessage request);
    
    // Pointer to the RegisterMap instance for static worker functions
    static RegisterMap* registerMapPtr; ///< Static pointer for worker functions

public:
    /**
     * @brief Construct a new Temp Modbus Server object
     * @param[in] regMap Reference to register map
     * @param[in] id Modbus server ID (1-247)
     * @param[in] serialPort Hardware serial port to use
     * @param[in] rx RX pin number
     * @param[in] tx TX pin number
     * @param[in] de Driver Enable pin for RS485
     * @param[in] baud Baud rate (default: 9600)
     */
    TempModbusServer(RegisterMap& regMap, uint8_t id, HardwareSerial& serialPort, 
                 int rx, int tx, int de, int baud = 9600);
    
    /**
     * @brief Destroy the Temp Modbus Server object
     * @details Cleans up Modbus server resources
     */
    ~TempModbusServer();
    
    /**
     * @brief Initialize and start Modbus server
     * @return true if server started successfully
     * @return false if initialization failed
     * @details Configures serial port, registers worker functions,
     *          and starts listening for Modbus requests
     */
    bool begin();
    
    /**
     * @brief Stop Modbus server
     * @details Stops server and releases resources
     */
    void stop();
    
    /**
     * @brief Process pending commands from Modbus writes
     * @details Checks for pending commands in register map and executes them
     * @note Should be called periodically from main loop
     */
    void processCommands();
};



#endif // TEMP_MODBUS_SERVER_H
