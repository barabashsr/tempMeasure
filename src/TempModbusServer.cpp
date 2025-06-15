#include "TempModbusServer.h"

TempModbusServer::TempModbusServer(RegisterMap& regMap, 
    uint8_t id, 
    HardwareSerial& serialPort, 
    int rx, 
    int tx, 
    int de,
    int baud) 
: registerMap(regMap), serverID(id), serial(serialPort), 
rxPin(rx), txPin(tx), baudRate(baud), dePin(de) {

// Create ModbusRTU server with 2000ms timeout
mbServer = new ModbusServerRTU(1000, dePin);

// Set static pointer to register map for worker functions
registerMapPtr = &regMap;

// LOG: Modbus server creation
LoggerManager::info("MODBUS", 
"Modbus server created - ID: " + String(id) + 
", Baud: " + String(baud) + 
", RX: " + String(rx) + 
", TX: " + String(tx) + 
", DE: " + String(de));
}


RegisterMap* TempModbusServer::registerMapPtr = nullptr;

TempModbusServer::~TempModbusServer() {
    if (mbServer) {
        // No need to call stop() as it doesn't exist
        delete mbServer;
        mbServer = nullptr;
    }
}


// bool TempModbusServer::begin() {
//     // Prepare serial port for Modbus RTU
//     RTUutils::prepareHardwareSerial(serial);
//     Serial.println("Modbus begin1");
//     if (baudRate == 0) {
//         baudRate = 9600;
//     }
//     serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
//     Serial.println("Modbus begin2");
    
//     // Register worker functions for different Modbus function codes
//     mbServer->registerWorker(serverID, READ_HOLD_REGISTER, &TempModbusServer::readHoldingRegistersWorker);
//     Serial.println("Modbus begin3");
//     mbServer->registerWorker(serverID, WRITE_HOLD_REGISTER, &TempModbusServer::writeHoldingRegisterWorker);
//     Serial.println("Modbus begin4");
//     mbServer->registerWorker(serverID, WRITE_MULT_REGISTERS, &TempModbusServer::writeMultipleRegistersWorker);
//     Serial.println("Modbus begin5");
    
//     // Start ModbusRTU server - note that begin() returns void
//     mbServer->begin(serial);
//     return true; // Assume success since we can't check
// }

bool TempModbusServer::begin() {
    LoggerManager::info("MODBUS", "Starting Modbus RTU server initialization");
    
    // Prepare serial port for Modbus RTU
    RTUutils::prepareHardwareSerial(serial);
    Serial.println("Modbus begin1");
    
    if (baudRate == 0) {
        baudRate = 9600;
        LoggerManager::warning("MODBUS", "Baud rate was 0, defaulting to 9600");
    }
    
    serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
    LoggerManager::info("MODBUS", 
        "Serial port initialized - Baud: " + String(baudRate) + 
        ", RX: " + String(rxPin) + 
        ", TX: " + String(txPin));
    Serial.println("Modbus begin2");
    
    // Register worker functions for different Modbus function codes
    mbServer->registerWorker(serverID, READ_HOLD_REGISTER, &TempModbusServer::readHoldingRegistersWorker);
    LoggerManager::info("MODBUS", "Registered READ_HOLD_REGISTER worker (FC=03)");
    Serial.println("Modbus begin3");
    
    mbServer->registerWorker(serverID, WRITE_HOLD_REGISTER, &TempModbusServer::writeHoldingRegisterWorker);
    LoggerManager::info("MODBUS", "Registered WRITE_HOLD_REGISTER worker (FC=06)");
    Serial.println("Modbus begin4");
    
    mbServer->registerWorker(serverID, WRITE_MULT_REGISTERS, &TempModbusServer::writeMultipleRegistersWorker);
    LoggerManager::info("MODBUS", "Registered WRITE_MULT_REGISTERS worker (FC=16)");
    Serial.println("Modbus begin5");
    
    // Start ModbusRTU server - note that begin() returns void
    mbServer->begin(serial);
    
    LoggerManager::info("MODBUS", 
        "Modbus RTU server started successfully - Server ID: " + String(serverID));
    
    return true; // Assume success since we can't check
}



void TempModbusServer::stop() {
    // There is no stop() method in ModbusServerRTU
    // You might want to implement any cleanup needed here
}


// Worker function for READ_HOLD_REGISTER (FC=03)
// ModbusMessage TempModbusServer::readHoldingRegistersWorker(ModbusMessage request) {
//     uint16_t address;           // Requested register address
//     uint16_t words;             // Requested number of registers
//     ModbusMessage response;     // Response message to be sent back
    
//     // Get request values
//     request.get(2, address);
//     request.get(4, words);
    
//     // Check if address and word count are valid
//     if (words > 0 && words <= 125) {  // Max 125 registers per request as per Modbus spec
//         // Set up response header
//         response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
        
//         // Add requested register values to response
//         bool allValid = true;
//         for (uint16_t i = 0; i < words; i++) {
//             uint16_t regValue = registerMapPtr->readHoldingRegister(address + i);
            
//             // Check if register read was successful - 0xFFFF is often used as an error indicator
//             if (regValue != 0xFFFF) {
//                 response.add(regValue);
//             } else {
//                 allValid = false;
//                 break;
//             }
//         }
        
//         // If any register was invalid, return error response
//         if (!allValid) {
//             response.clear();
//             response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
//         }
//     } else {
//         // Invalid word count, return error response
//         response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
//     }
    
//     return response;
// }

ModbusMessage TempModbusServer::readHoldingRegistersWorker(ModbusMessage request) {
    uint16_t address;           // Requested register address
    uint16_t words;             // Requested number of registers
    ModbusMessage response;     // Response message to be sent back
    
    // Get request values
    request.get(2, address);
    request.get(4, words);
    
    // LOG: Read request received
    LoggerManager::info("MODBUS_READ", 
        "Read request - Address: " + String(address) + 
        ", Count: " + String(words) + 
        ", Client ID: " + String(request.getServerID()));
    
    // Check if address and word count are valid
    if (words > 0 && words <= 125) {  // Max 125 registers per request as per Modbus spec
        // Set up response header
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
        
        // Add requested register values to response
        bool allValid = true;
        String readValues = "";
        
        for (uint16_t i = 0; i < words; i++) {
            uint16_t regValue = registerMapPtr->readHoldingRegister(address + i);
            
            // Check if register read was successful - 0xFFFF is often used as an error indicator
            if (regValue != 0xFFFF) {
                response.add(regValue);
                if (i < 5) { // Log first 5 values to avoid spam
                    readValues += String(address + i) + ":" + String(regValue) + " ";
                } else if (i == 5) {
                    readValues += "...";
                }
            } else {
                allValid = false;
                LoggerManager::error("MODBUS_READ", 
                    "Failed to read register " + String(address + i));
                break;
            }
        }
        
        // If any register was invalid, return error response
        if (!allValid) {
            response.clear();
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            LoggerManager::error("MODBUS_READ", 
                "Read failed - Invalid register address range: " + 
                String(address) + "-" + String(address + words - 1));
        } else {
            LoggerManager::info("MODBUS_READ", 
                "Read successful - " + String(words) + " registers: " + readValues);
        }
    } else {
        // Invalid word count, return error response
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        LoggerManager::error("MODBUS_READ", 
            "Read failed - Invalid word count: " + String(words) + " (max 125)");
    }
    
    return response;
}



// Worker function for WRITE_HOLD_REGISTER (FC=06)
// ModbusMessage TempModbusServer::writeHoldingRegisterWorker(ModbusMessage request) {
//     uint16_t address;           // Register address to write
//     uint16_t value;             // Value to write
//     ModbusMessage response;     // Response message to be sent back
    
//     // Get request values
//     request.get(2, address);
//     request.get(4, value);
    
//     // Try to write the value to the register
//     if (registerMapPtr->writeHoldingRegister(address, value)) {
//         // Success - echo the request as response
//         return request;
//     } else {
//         // Failed - return error response
//         response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
//         return response;
//     }
// }

ModbusMessage TempModbusServer::writeHoldingRegisterWorker(ModbusMessage request) {
    uint16_t address;           // Register address to write
    uint16_t value;             // Value to write
    ModbusMessage response;     // Response message to be sent back
    
    // Get request values
    request.get(2, address);
    request.get(4, value);
    
    // LOG: Write request received
    LoggerManager::info("MODBUS_WRITE", 
        "Write single register - Address: " + String(address) + 
        ", Value: " + String(value) + 
        ", Client ID: " + String(request.getServerID()));
    
    // Try to write the value to the register
    if (registerMapPtr->writeHoldingRegister(address, value)) {
        // Success - echo the request as response
        LoggerManager::info("MODBUS_WRITE", 
            "Write successful - Register " + String(address) + " = " + String(value));
        return request;
    } else {
        // Failed - return error response
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        LoggerManager::error("MODBUS_WRITE", 
            "Write failed - Invalid register address: " + String(address));
        return response;
    }
}


// Worker function for WRITE_MULT_REGISTERS (FC=16)
// ModbusMessage TempModbusServer::writeMultipleRegistersWorker(ModbusMessage request) {
//     uint16_t address;           // Starting register address
//     uint16_t words;             // Number of registers to write
//     uint8_t bytesCount;         // Number of data bytes in request
//     ModbusMessage response;     // Response message to be sent back
    
//     // Get request values
//     request.get(2, address);
//     request.get(4, words);
//     request.get(6, bytesCount);
    
//     // Check if word count is valid
//     if (words > 0 && words <= 123 && bytesCount == words * 2) {  // Max 123 registers per request
//         bool allWritten = true;
        
//         // Write each register value
//         for (uint16_t i = 0; i < words; i++) {
//             uint16_t value;
//             request.get(7 + i * 2, value);
            
//             if (!registerMapPtr->writeHoldingRegister(address + i, value)) {
//                 allWritten = false;
//                 break;
//             }
//         }
        
//         if (allWritten) {
//             // Success - create response with address and word count
//             response.add(request.getServerID(), request.getFunctionCode());
//             response.add(address);
//             response.add(words);
//         } else {
//             // Failed to write at least one register
//             response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
//         }
//     } else {
//         // Invalid word count or byte count
//         response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
//     }
    
//     return response;
// }
ModbusMessage TempModbusServer::writeMultipleRegistersWorker(ModbusMessage request) {
    uint16_t address;           // Starting register address
    uint16_t words;             // Number of registers to write
    uint8_t bytesCount;         // Number of data bytes in request
    ModbusMessage response;     // Response message to be sent back
    
    // Get request values
    request.get(2, address);
    request.get(4, words);
    request.get(6, bytesCount);
    
    // LOG: Multiple write request received
    LoggerManager::info("MODBUS_WRITE", 
        "Write multiple registers - Start: " + String(address) + 
        ", Count: " + String(words) + 
        ", Bytes: " + String(bytesCount) + 
        ", Client ID: " + String(request.getServerID()));
    
    // Check if word count is valid
    if (words > 0 && words <= 123 && bytesCount == words * 2) {  // Max 123 registers per request
        bool allWritten = true;
        String writeValues = "";
        
        // Write each register value
        for (uint16_t i = 0; i < words; i++) {
            uint16_t value;
            request.get(7 + i * 2, value);
            
            if (registerMapPtr->writeHoldingRegister(address + i, value)) {
                if (i < 5) { // Log first 5 values to avoid spam
                    writeValues += String(address + i) + ":" + String(value) + " ";
                } else if (i == 5) {
                    writeValues += "...";
                }
            } else {
                allWritten = false;
                LoggerManager::error("MODBUS_WRITE", 
                    "Failed to write register " + String(address + i) + " = " + String(value));
                break;
            }
        }
        
        if (allWritten) {
            // Success - create response with address and word count
            response.add(request.getServerID(), request.getFunctionCode());
            response.add(address);
            response.add(words);
            LoggerManager::info("MODBUS_WRITE", 
                "Multiple write successful - " + String(words) + " registers: " + writeValues);
        } else {
            // Failed to write at least one register
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            LoggerManager::error("MODBUS_WRITE", 
                "Multiple write failed - Error writing to register range: " + 
                String(address) + "-" + String(address + words - 1));
        }
    } else {
        // Invalid word count or byte count
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        LoggerManager::error("MODBUS_WRITE", 
            "Multiple write failed - Invalid parameters: Words=" + String(words) + 
            ", Bytes=" + String(bytesCount) + " (expected " + String(words * 2) + ")");
    }
    
    return response;
}
