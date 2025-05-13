#ifndef TEMP_MODBUS_SERVER_H
#define TEMP_MODBUS_SERVER_H

#include <Arduino.h>
#include "ModbusServerRTU.h"
#include "RegisterMap.h"

class TempModbusServer {
private:
    ModbusServerRTU* mbServer;
    RegisterMap& registerMap;
    uint8_t serverID;
    HardwareSerial& serial;
    int rxPin;
    int txPin;
    int baudRate;
    
    // Worker functions for different Modbus function codes
    static ModbusMessage readHoldingRegistersWorker(ModbusMessage request);
    static ModbusMessage writeHoldingRegisterWorker(ModbusMessage request);
    static ModbusMessage writeMultipleRegistersWorker(ModbusMessage request);
    
    // Pointer to the RegisterMap instance for static worker functions
    static RegisterMap* registerMapPtr;

public:
    TempModbusServer(RegisterMap& regMap, uint8_t id, HardwareSerial& serialPort, 
                 int rx, int tx, int baud = 9600);
    ~TempModbusServer();
    
    bool begin();
    void stop();
};



#endif // TEMP_MODBUS_SERVER_H
