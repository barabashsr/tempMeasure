# Technology Stack

## Overview

This document details the technology stack used in the Temperature Controller System Enhancement project. Understanding these technologies is crucial for development and maintenance.

## Hardware Platform

### ESP32 Microcontroller
- **Model**: ESP32-WROOM-32
- **Architecture**: Dual-core Xtensa LX6 @ 240MHz
- **RAM**: 520KB SRAM
- **Flash**: 4MB
- **WiFi**: 802.11 b/g/n
- **Bluetooth**: Classic & BLE (not used in this project)

### Peripheral Hardware
- **Temperature Sensors**:
  - DS18B20: Digital 1-Wire sensors (up to 50 units)
  - PT1000: Analog RTD sensors via ADC (up to 10 units)
- **Display**: SSD1306 OLED 128x64 pixels (I2C)
- **Storage**: SD Card via SPI interface
- **Communication**: RS-485 for Modbus RTU
- **Indicators**: 4 LEDs, 3 Relays
- **Input**: 1 Navigation button

## Development Environment

### PlatformIO
- **Version**: Latest stable
- **Platform**: espressif32
- **Framework**: arduino
- **Board**: esp32dev

### platformio.ini Configuration
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600

; Partition table for OTA support
board_build.partitions = partitions.csv

; Build flags for optimization
build_flags = 
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_ASYNC_TCP_RUNNING_CORE=1
    -O2

; Library dependencies
lib_deps = 
    ; Core libraries
    bblanchon/ArduinoJson@^7.0.0
    https://github.com/gemi254/ConfigAssist.git
    256dpi/MQTT@^2.5.1
    
    ; Sensor libraries
    paulstoffregen/OneWire@^2.3.7
    milesburton/DallasTemperature@^3.11.0
    
    ; Display libraries
    thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays@^4.4.0
    
    ; Modbus library
    4-20ma/ModbusMaster@^2.0.1
    
    ; QR Code generation
    ricmoo/QRCode@^0.0.1
```

## Core Libraries

### ConfigAssist
- **Purpose**: Configuration management with web interface
- **Features**:
  - YAML-based configuration definition
  - Automatic HTML form generation
  - JSON/INI file persistence
  - Built-in web server integration
- **Documentation**: [GitHub](https://github.com/gemi254/ConfigAssist)

### ArduinoJson v7
- **Purpose**: JSON parsing and generation
- **Usage**:
  - MQTT message formatting
  - API response generation
  - Configuration file handling
- **Memory**: Uses StaticJsonDocument for predictable allocation
- **Best Practices**: 
  ```cpp
  StaticJsonDocument<2048> doc;  // Stack allocation preferred
  ```

### 256dpi/MQTT
- **Purpose**: MQTT client implementation
- **Features**:
  - ESP32 optimized
  - TLS/SSL support
  - QoS 0, 1, 2 support
  - Automatic reconnection
- **Advantages over PubSubClient**:
  - Better memory management
  - Native ESP32 support
  - Larger message buffer support

### OneWire & DallasTemperature
- **Purpose**: DS18B20 sensor communication
- **Features**:
  - Multi-sensor support on single bus
  - Temperature resolution configuration
  - Async temperature reading
- **Limitations**: 
  - Maximum 50 sensors per bus (project requirement)

### ModbusMaster
- **Purpose**: Modbus RTU slave implementation
- **Features**:
  - Full function code support
  - Custom register mapping
  - RS-485 communication
- **Register Map**: 900+ registers for comprehensive system access

### ESP8266 and ESP32 OLED Driver
- **Purpose**: SSD1306 OLED display control
- **Features**:
  - I2C communication
  - Built-in fonts
  - Graphics primitives
  - UI widget support
- **Memory**: ~1KB frame buffer

### QRCode
- **Purpose**: QR code generation for display
- **Features**:
  - Configurable error correction
  - Compact size (29x29 for version 3)
  - Low memory footprint
- **Usage**: WiFi credentials and URL display

## Web Technologies

### Frontend Stack
- **HTML5**: Semantic markup with data attributes for i18n
- **CSS3**: Responsive design, no frameworks (lightweight)
- **Vanilla JavaScript**: ES6+ features, no frameworks
- **Chart.js**: Temperature trend visualization

### Web File Structure
```
data/
├── index.html          # Dashboard
├── alarms.html         # Alarm configuration
├── settings.html       # General settings
├── settings-mqtt.html  # MQTT configuration
├── settings-network.html # Network settings
├── settings-modbus.html  # Modbus settings
├── mqtt-history.html   # MQTT log viewer (new)
├── logs.html           # Event logs
├── about.html          # System information
├── style.css           # Common styles
├── app.js              # Common JavaScript
└── translations.js     # i18n support (new)
```

### AJAX Communication
```javascript
// Standardized API calls
async function apiCall(endpoint, method = 'GET', data = null) {
    const options = {
        method,
        headers: {
            'Content-Type': 'application/json',
        }
    };
    
    if (data) {
        options.body = JSON.stringify(data);
    }
    
    const response = await fetch(endpoint, options);
    if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
    }
    
    return await response.json();
}
```

## File System

### LittleFS
- **Purpose**: Internal flash file system
- **Usage**:
  - Configuration files (/config.ini, /mqtt.json)
  - Web server files (/data/*)
  - Temporary data
- **Size**: ~1.5MB partition
- **Advantages**: Wear leveling, power-loss resilient

### SD Card (FAT32)
- **Purpose**: Data logging and file storage
- **File Types**:
  - Temperature logs: `data_YYYY-MM-DD_NNN.csv`
  - Event logs: `events_YYYY-MM-DD_NNN.csv`
  - Alarm logs: `alarms_YYYY-MM-DD_NNN.csv`
  - MQTT logs: `mqtt_log_YYYY-MM-DD.csv` (new)
- **Organization**: Daily files with sequence numbers
- **Retention**: Configurable (default 30 days)

## Communication Protocols

### MQTT (New Enhancement)
- **Version**: 3.1.1
- **Broker Compatibility**: Any MQTT 3.1.1 compliant broker
- **TLS Support**: TLS 1.2 with certificate validation
- **Topics**: ISA-95 hierarchical structure
- **QoS Levels**:
  - QoS 0: Temperature telemetry
  - QoS 1: Alarm notifications
  - QoS 2: Commands
- **Message Format**: JSON
- **Max Message Size**: 4KB

### Modbus RTU
- **Mode**: Slave/Server
- **Baud Rate**: 9600 (configurable)
- **Data Format**: 8N1
- **Address**: 1-247 (configurable)
- **Function Codes**:
  - 0x03: Read Holding Registers
  - 0x06: Write Single Register
  - 0x10: Write Multiple Registers
- **Register Types**:
  - Configuration registers
  - Status registers
  - Command registers (including 899)

### HTTP REST API
- **Endpoints**:
  ```
  GET  /api/status              # System status
  GET  /api/points              # All measurement points
  GET  /api/points/{id}         # Specific point
  POST /api/alarms/acknowledge  # Acknowledge alarm
  GET  /api/logs/measurement    # Temperature logs
  GET  /api/logs/events         # Event logs
  GET  /api/mqtt/logs           # MQTT logs list (new)
  POST /api/config/import       # Import CSV config
  GET  /api/config/export       # Export CSV config
  ```

## Memory Management

### RAM Usage Breakdown
```
Total ESP32 RAM: 520KB
├── Core System: 80KB
│   ├── FreeRTOS: 20KB
│   ├── WiFi Stack: 40KB
│   └── TCP/IP Stack: 20KB
├── Application: 110KB
│   ├── Measurement Points: 30KB
│   ├── Web Server: 20KB
│   ├── Modbus: 10KB
│   ├── Sensor Management: 15KB
│   ├── Display: 5KB
│   └── Buffers: 30KB
├── MQTT Enhancement: 30KB
│   ├── Client: 15KB
│   ├── Message Buffers: 8KB
│   ├── Command Queue: 3KB
│   └── Misc: 4KB
└── Free Heap: ~300KB
```

### Flash Usage
```
Total Flash: 4MB
├── Application: 1.4MB
├── LittleFS: 1.5MB
├── OTA Partition: 1.4MB
└── NVS: 16KB
```

## Security Considerations

### Network Security
- **WiFi**: WPA2 encryption required
- **Web Interface**: No authentication (local network only)
- **MQTT**: Username/password authentication
- **TLS**: Optional for MQTT, certificate validation

### Input Validation
- All web inputs sanitized
- Modbus register range validation
- Configuration value bounds checking
- CSV import validation

### Secure Coding
```cpp
// Example: Safe string handling
void processInput(const char* input) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", input);  // Prevent overflow
    // Process buffer...
}
```

## Development Tools

### Required Tools
- **VS Code** with PlatformIO extension
- **Git** for version control
- **Doxygen** for documentation generation
- **MQTT Explorer** for MQTT testing
- **ModbusPoll** for Modbus testing
- **Serial Monitor** for debugging

### Testing Tools
- **HiveMQ Cloud** - Free MQTT broker for testing
- **Postman** - API endpoint testing
- **Chrome DevTools** - Web interface debugging

## Performance Targets

### Real-time Constraints
- Temperature reading: Every 1 second
- Display update: 10 FPS (100ms)
- Modbus response: <50ms
- Web API response: <200ms
- MQTT publish: <500ms

### Resource Constraints
- CPU usage: <70% average
- Free heap: >100KB minimum
- Stack high water mark: >1KB per task
- SD card writes: <10/second average

## Future Considerations

### Potential Upgrades
- ESP32-S3 for more RAM/Flash
- Ethernet support via W5500
- LoRaWAN for long-range communication
- Cloud integration (AWS IoT, Azure IoT)

### Scalability
- Current design supports:
  - 60 measurement points (hardware limited)
  - 100 MQTT commands/minute
  - 10 concurrent web sessions
  - 1GB+ SD card storage

This technology stack provides a robust foundation for industrial temperature monitoring with modern connectivity options while maintaining reliability and performance.