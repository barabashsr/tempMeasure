# Temperature Controller Technical Documentation

## Table of Contents
1. [System Architecture](#system-architecture)
2. [Class Reference](#class-reference)
3. [API Reference](#api-reference)
4. [Configuration Files](#configuration-files)
5. [Communication Protocols](#communication-protocols)
6. [Hardware Interfaces](#hardware-interfaces)
7. [Build and Deployment](#build-and-deployment)

## System Architecture

### Overview
The Temperature Controller system follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                        Web Interface                         │
│                    (HTML/CSS/JavaScript)                     │
└─────────────────────┬───────────────────────────────────────┘
                      │ HTTP/WebSocket
┌─────────────────────┴───────────────────────────────────────┐
│                      ConfigManager                           │
│              (Web Server + Configuration)                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
┌─────────────────────┴───────────────────────────────────────┐
│                  TemperatureController                       │
│            (Core Business Logic + Orchestration)             │
└──┬──────────┬──────────┬──────────┬──────────┬─────────────┘
   │          │          │          │          │
┌──┴───┐  ┌──┴───┐  ┌──┴───┐  ┌──┴───┐  ┌───┴──────┐
│Sensor│  │Point │  │Alarm │  │RegMap│  │Indicator │
└──────┘  └──────┘  └──────┘  └──────┘  └──────────┘
```

### Core Components

#### 1. TemperatureController
Central orchestrator managing all system operations:
- Sensor discovery and management
- Measurement point handling
- Alarm processing
- Register map updates
- Hardware interface coordination

#### 2. MeasurementPoint
Represents a logical measurement location:
- Holds current, min, max temperature values
- Manages alarm thresholds
- Links to physical sensors
- Maintains error states

#### 3. Sensor
Abstracts physical temperature sensors:
- DS18B20 digital sensors
- PT1000/PT100 RTD sensors
- Handles communication protocols
- Error detection and recovery

#### 4. Alarm System
Multi-level alarm management:
- Priority-based processing
- State machine implementation
- Hysteresis support
- Acknowledged delay handling

#### 5. RegisterMap
Modbus register abstraction:
- Maps internal data to Modbus registers
- Handles read/write operations
- Data type conversions
- Access control

#### 6. IndicatorInterface
Hardware UI management:
- OLED display control
- LED management
- Button input handling
- PCF8575 I/O expander interface

## Class Reference

### TemperatureController

```cpp
class TemperatureController {
public:
    // Constructor
    TemperatureController(uint8_t oneWirePins[4], uint8_t csPins[4], 
                         IndicatorInterface& indicator);
    
    // Initialization
    bool begin();
    
    // Main update loop
    void update();
    
    // Sensor management
    bool addSensor(Sensor* sensor);
    bool removeSensorByRom(const String& romString);
    Sensor* findSensorByRom(const String& romString);
    bool discoverDS18B20Sensors();
    bool discoverPTSensors();
    
    // Point management
    MeasurementPoint* getMeasurementPoint(uint8_t address);
    bool bindSensorToPoint(const String& romString, uint8_t pointAddress);
    bool unbindSensorFromPoint(uint8_t pointAddress);
    
    // Alarm management
    void updateAlarms();
    std::vector<Alarm*> getActiveAlarms() const;
    void acknowledgeHighestPriorityAlarm();
    void acknowledgeAllAlarms();
    
    // Configuration
    void setDeviceId(uint16_t id);
    void setMeasurementPeriod(uint16_t seconds);
    void setAcknowledgedDelay(AlarmPriority priority, unsigned long delay);
    
    // Data access
    String getSensorsJson();
    String getPointsJson();
    String getAlarmsJson();
    RegisterMap& getRegisterMap();
};
```

### MeasurementPoint

```cpp
class MeasurementPoint {
public:
    // Constructor
    MeasurementPoint(uint8_t address);
    
    // Temperature management
    void updateTemperature(int16_t temp);
    int16_t getCurrentTemp() const;
    int16_t getMinTemp() const;
    int16_t getMaxTemp() const;
    void resetMinMax();
    
    // Threshold management
    void setLowThreshold(int16_t threshold);
    void setHighThreshold(int16_t threshold);
    int16_t getLowThreshold() const;
    int16_t getHighThreshold() const;
    
    // Sensor binding
    void bindSensor(Sensor* sensor);
    void unbindSensor();
    Sensor* getBoundSensor() const;
    bool hasSensor() const;
    
    // Status
    uint8_t getAddress() const;
    String getName() const;
    void setName(const String& name);
    uint16_t getAlarmStatus() const;
    uint16_t getErrorStatus() const;
};
```

### Sensor

```cpp
class Sensor {
public:
    // Sensor types
    enum class SensorType {
        DS18B20,
        PT1000,
        PT100,
        UNKNOWN
    };
    
    // Constructor
    Sensor(SensorType type, const String& id);
    
    // Temperature reading
    virtual bool readTemperature(int16_t& temperature) = 0;
    
    // Identification
    String getId() const;
    SensorType getType() const;
    String getTypeString() const;
    
    // Status
    bool isConnected() const;
    uint32_t getErrorCount() const;
    unsigned long getLastReadTime() const;
};
```

### Alarm

```cpp
class Alarm {
public:
    // Constructor
    Alarm(AlarmType type, MeasurementPoint* source, 
          AlarmPriority priority = AlarmPriority::PRIORITY_MEDIUM);
    
    // State management
    void acknowledge();
    void clear();
    void resolve();
    void reactivate();
    
    // Properties
    AlarmType getType() const;
    AlarmStage getStage() const;
    AlarmPriority getPriority() const;
    MeasurementPoint* getSource() const;
    
    // Timing
    unsigned long getTimestamp() const;
    unsigned long getAcknowledgedTime() const;
    bool isAcknowledgedDelayElapsed() const;
    
    // Display
    String getDisplayText() const;
    String getStatusText() const;
    
    // Configuration
    void setHysteresis(int16_t hysteresis);
    void setAcknowledgedDelay(unsigned long delay);
};
```

## API Reference

### RESTful API Endpoints

#### System Status
```
GET /api/status
Response: {
    "deviceId": 1000,
    "firmware": "2.0",
    "uptime": 12345,
    "time": "2024-12-30T14:23:45",
    "network": {
        "wifi": true,
        "ip": "192.168.1.100",
        "rssi": -45
    }
}
```

#### Points Management
```
GET /api/points
Response: [{
    "address": 0,
    "name": "Room A",
    "temperature": 23.5,
    "min": 20.1,
    "max": 25.3,
    "sensor": "28:AA:BB:CC:DD:EE:FF:00",
    "status": "normal"
}]

PUT /api/points/{address}
Body: {
    "name": "New Name",
    "lowThreshold": -10.0,
    "highThreshold": 30.0
}

POST /api/points/{address}/bind
Body: {
    "sensorId": "28:AA:BB:CC:DD:EE:FF:00"
}
```

#### Sensors Management
```
GET /api/sensors
Response: [{
    "id": "28:AA:BB:CC:DD:EE:FF:00",
    "type": "DS18B20",
    "temperature": 23.5,
    "bound": true,
    "pointAddress": 0,
    "errors": 0
}]

POST /api/sensors/discover
Response: {
    "discovered": 5,
    "sensors": [...]
}
```

#### Alarms Management
```
GET /api/alarms
Response: [{
    "id": "alarm_0_high",
    "type": "HIGH_TEMPERATURE",
    "point": 0,
    "priority": "CRITICAL",
    "stage": "ACTIVE",
    "timestamp": "2024-12-30T14:23:45",
    "value": 45.2,
    "threshold": 40.0
}]

POST /api/alarms/acknowledge
Body: {
    "alarmId": "alarm_0_high"
}

GET /api/alarms/config
Response: [{
    "point": 0,
    "lowPriority": "MEDIUM",
    "highPriority": "HIGH",
    "errorPriority": "CRITICAL",
    "hysteresis": 2.0
}]

PUT /api/alarms/config
Body: [{
    "point": 0,
    "lowPriority": "HIGH",
    "highPriority": "CRITICAL",
    "hysteresis": 3.0
}]
```

#### Logs Access
```
GET /api/logs/temperature?date=2024-12-30
Response: CSV data

GET /api/logs/events?date=2024-12-30&category=ALARM
Response: [{
    "timestamp": "2024-12-30T14:23:45",
    "category": "ALARM",
    "message": "High temperature alarm on Point 5"
}]

GET /api/logs/list
Response: [{
    "type": "temperature",
    "date": "2024-12-30",
    "size": 102400,
    "url": "/api/logs/download/temperature_2024-12-30.csv"
}]
```

### WebSocket API

#### Connection
```javascript
const ws = new WebSocket('ws://device-ip/ws');

ws.onopen = () => {
    // Subscribe to updates
    ws.send(JSON.stringify({
        action: 'subscribe',
        topics: ['alarms', 'temperatures']
    }));
};
```

#### Real-time Updates
```javascript
// Temperature update
{
    "type": "temperature",
    "data": {
        "point": 0,
        "value": 23.5,
        "timestamp": "2024-12-30T14:23:45"
    }
}

// Alarm notification
{
    "type": "alarm",
    "data": {
        "id": "alarm_0_high",
        "action": "triggered",
        "details": {...}
    }
}
```

## Configuration Files

### Main Configuration (`/config.yaml`)
```yaml
Wifi settings:
  - st_ssid:
      label: WiFi SSID
      default: ''
  - st_pass:
      label: WiFi Password
      default: ''
      
Device settings:
  - device_id:
      label: Device ID
      type: number
      min: 1
      max: 9999
      default: 1000
  - measurement_period:
      label: Measurement Period (seconds)
      type: number
      min: 1
      max: 3600
      default: 10
      
Modbus settings:
  - modbus_enabled:
      label: Enable Modbus RTU
      checked: true
  - modbus_address:
      label: Modbus Device Address
      type: number
      min: 1
      max: 247
      default: 1
```

### Sensor Configuration (`/sensors.csv`)
```csv
rom,type,point,name
28:AA:BB:CC:DD:EE:FF:00,DS18B20,0,Room A
28:11:22:33:44:55:66:77,DS18B20,1,Room B
PT:1,PT1000,50,Boiler Input
```

### Alarm Configuration (`/alarms.csv`)
```csv
point,low_threshold,low_priority,high_threshold,high_priority,error_priority,hysteresis
0,-10.0,MEDIUM,30.0,HIGH,CRITICAL,2.0
1,-5.0,LOW,35.0,MEDIUM,HIGH,1.5
```

## Communication Protocols

### OneWire Protocol (DS18B20)
- Bus speed: Standard (15.4 kbps)
- Power: Parasitic or external (3.3V)
- Pull-up resistor: 4.7kΩ
- Maximum devices per bus: 15 (recommended)
- Maximum cable length: 100m

### SPI Protocol (PT1000)
- Clock speed: 1 MHz
- Mode: 3 (CPOL=1, CPHA=1)
- Chip select: Active low
- Data format: MSB first
- MAX31865 configuration: 3-wire or 4-wire RTD

### Modbus RTU Protocol
- Physical layer: RS-485
- Baud rates: 4800, 9600, 19200, 38400, 57600, 115200
- Data format: 8 data bits, no parity, 1 stop bit
- Maximum nodes: 32 (without repeater)
- Maximum cable length: 1200m

### I2C Protocol
- Speed: 100 kHz (standard mode)
- Addresses used:
  - 0x20: PCF8575 I/O expander
  - 0x3C: OLED display
  - 0x68: RTC (if present)

## Hardware Interfaces

### Pin Assignments (ESP32-WROVER)

#### OneWire Buses
```
BUS1_PIN: GPIO 4
BUS2_PIN: GPIO 5
BUS3_PIN: GPIO 18
BUS4_PIN: GPIO 19
```

#### SPI Interface
```
SCK_PIN:  GPIO 14
MISO_PIN: GPIO 12
MOSI_PIN: GPIO 13
CS1_PIN:  GPIO 32
CS2_PIN:  GPIO 33
CS3_PIN:  GPIO 26
CS4_PIN:  GPIO 27
CS5_PIN:  GPIO 0 (SD Card)
```

#### RS-485 Interface
```
RX_PIN: GPIO 22
TX_PIN: GPIO 23
DE_PIN: Not used (auto-direction)
```

#### I2C Interface
```
SDA_PIN: GPIO 21
SCL_PIN: GPIO 25
PCF_INT: GPIO 34
```

### PCF8575 I/O Mapping

| Port | Function | Direction | Active |
|------|----------|-----------|--------|
| P0 | Button Input | Input | Low |
| P1 | Relay 1 (Siren) | Output | High |
| P2 | Relay 2 (Beacon) | Output | High |
| P3 | Relay 3 (Spare) | Output | High |
| P4 | LED Red | Output | Low |
| P5 | LED Yellow | Output | Low |
| P6 | LED Blue | Output | Low |
| P7 | LED Green | Output | Low |
| P8-P15 | Reserved | - | - |

## Build and Deployment

### Development Environment
- PlatformIO Core 6.1.0+
- ESP-IDF 4.4+
- Arduino framework for ESP32

### Dependencies
```ini
lib_deps = 
    adafruit/Adafruit MAX31865 library
    Wire
    WiFi
    OneWire
    ESPAsyncWebServer
    AsyncTCP
    ArduinoJson
    DallasTemperature
    eModbus
    ConfigAssist
    PCF8575
    U8g2
    CSV-Parser-for-Arduino
    RTClib
    NTPClient
```

### Build Configuration
```ini
[env:esp-wrover-kit]
platform = espressif32
board = esp-wrover-kit
framework = arduino
monitor_speed = 115200
build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -DCA_USE_LITTLEFS
    -DLOGGER_LOG_LEVEL=3
board_build.partitions = huge_app.csv
board_build.filesystem = littlefs
```

### Partition Table
```
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x300000
app1,     app,  ota_1,   0x310000,0x300000
littlefs, data, littlefs,0x610000,0x1F0000
```

### Build Commands
```bash
# Clean build
pio run --target clean

# Build firmware
pio run

# Upload firmware
pio run --target upload

# Upload filesystem
pio run --target uploadfs

# Monitor serial output
pio device monitor

# Full deployment
pio run --target upload && pio run --target uploadfs
```

### OTA Update Process
1. Build firmware: `pio run`
2. Access web interface: `http://device-ip/update`
3. Select firmware.bin from `.pio/build/esp-wrover-kit/`
4. Click Upload and wait for completion
5. Device will reboot automatically

### Debugging

#### Serial Debug Levels
```cpp
#define LOGGER_LOG_LEVEL_NONE 0
#define LOGGER_LOG_LEVEL_ERROR 1
#define LOGGER_LOG_LEVEL_WARN 2
#define LOGGER_LOG_LEVEL_INFO 3
#define LOGGER_LOG_LEVEL_DEBUG 4
#define LOGGER_LOG_LEVEL_VERBOSE 5
```

#### Common Debug Commands
```cpp
// In setup()
Serial.setDebugOutput(true);
esp_log_level_set("*", ESP_LOG_VERBOSE);

// Memory debugging
Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

// Task debugging
Serial.printf("Task watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
```

### Performance Optimization

#### Memory Management
- Use PSRAM for large buffers
- Minimize String usage in loops
- Prefer stack allocation over heap
- Use F() macro for constant strings

#### Task Priorities
```cpp
#define SENSOR_TASK_PRIORITY 5
#define ALARM_TASK_PRIORITY 4
#define WEB_TASK_PRIORITY 3
#define MODBUS_TASK_PRIORITY 3
#define LOG_TASK_PRIORITY 2
```

#### Timing Constraints
- Sensor reading: 750ms max (DS18B20)
- Web response: 500ms target
- Modbus response: 100ms max
- Display update: 100ms (10 FPS)

## Error Handling

### Error Codes

| Code | Category | Description |
|------|----------|-------------|
| 1xx | Sensor | Sensor-related errors |
| 2xx | Network | Network and communication errors |
| 3xx | Storage | SD card and filesystem errors |
| 4xx | Config | Configuration errors |
| 5xx | System | System and hardware errors |

### Recovery Strategies

#### Sensor Errors
1. Retry reading 3 times with 100ms delay
2. Mark sensor as disconnected after 3 failures
3. Disable temperature alarms for affected point
4. Log error with timestamp and sensor ID

#### Network Errors
1. Attempt reconnection every 30 seconds
2. Fall back to AP mode after 5 failures
3. Continue normal operation without network
4. Buffer logs locally until connection restored

#### Storage Errors
1. Retry write operation once
2. Skip logging if SD card fails
3. Use circular buffer in RAM (last 100 entries)
4. Alert user via LED and display

## Testing

### Unit Tests
Located in `/test` directory:
- test_alarm_logic.cpp
- test_sensor_reading.cpp
- test_modbus_registers.cpp
- test_web_api.cpp

### Integration Tests
- Full system startup sequence
- Sensor discovery and binding
- Alarm trigger and acknowledge cycle
- Modbus master simulation
- Web interface automation

### Performance Tests
- Maximum sensor count stress test
- Concurrent web requests handling
- Modbus polling rate limits
- Memory leak detection
- Long-term stability (72 hours)

### Hardware-in-the-Loop Tests
- Temperature chamber cycling
- Power cycling resilience
- EMI/RFI susceptibility
- Cable length limits
- Relay lifecycle testing

---
*Version: 2.0*  
*Last Updated: December 2024*
