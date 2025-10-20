# Source Tree Structure

## Project Organization

The Temperature Controller system follows a modular architecture with clear separation between interface definitions (headers) and implementations.

## Directory Structure

```
tempMeasure/
├── src/                        # Source implementations
│   ├── main.cpp               # Application entry point and setup
│   ├── TemperatureController.cpp # Main orchestrator
│   ├── ConfigManager.cpp      # Web UI and configuration management
│   ├── MQTTManager.cpp        # MQTT client implementation
│   ├── Alarm.cpp              # Alarm state machine
│   ├── MeasurementPoint.cpp   # Temperature point management
│   ├── Sensor.cpp             # DS18B20/PT1000 sensor interfaces
│   ├── LoggerManager.cpp      # SD card logging system
│   ├── IndicatorInterface.cpp # OLED/LED/button interface
│   ├── TempModbusServer.cpp   # Modbus RTU server
│   ├── RegisterMap.cpp        # Modbus register mapping
│   ├── CSVConfigManager.cpp   # CSV import/export
│   ├── SettingsCSVManager.cpp # Settings persistence
│   └── TimeManager.cpp        # RTC and time management
│
├── include/                    # Header files
│   ├── TemperatureController.h
│   ├── ConfigManager.h
│   ├── MQTTManager.h
│   ├── Alarm.h
│   ├── MeasurementPoint.h
│   ├── Sensor.h
│   ├── LoggerManager.h
│   ├── IndicatorInterface.h
│   ├── TempModbusServer.h
│   ├── RegisterMap.h
│   ├── CSVConfigManager.h
│   ├── SettingsCSVManager.h
│   └── TimeManager.h
│
├── data/                       # Web interface files
│   ├── index.html             # Main dashboard
│   ├── settings.html          # Configuration UI
│   ├── alarms.html            # Alarm configuration
│   ├── logs.html              # Log viewer
│   ├── styles.css             # Styling
│   ├── app.js                 # Main JavaScript
│   └── language.js            # Translation support
│
├── docs/                       # Documentation
│   ├── architecture.md        # Technical architecture
│   ├── prd.md                 # Product requirements
│   ├── architecture/          # Sharded architecture docs
│   │   ├── coding-standards.md
│   │   ├── tech-stack.md
│   │   └── source-tree.md     # This file
│   └── prd/                   # Sharded PRD docs
│
├── test/                       # Unit tests
│   └── test_main.cpp          # Test suite entry
│
├── lib/                        # Local libraries
│   └── README
│
├── scripts/                    # Utility scripts
│   └── generate_docs.sh       # Doxygen documentation
│
├── .claude/                    # Claude AI assistant files
│   ├── sessions/              # Session history
│   └── scripts/               # Assistant scripts
│
├── platformio.ini             # Build configuration
├── CLAUDE.md                  # AI assistant instructions
└── README.md                  # Project documentation
```

## Core Components

### Main Application (main.cpp)
- Entry point for ESP32 firmware
- Initializes all subsystems in correct order
- Manages main execution loop
- Handles WiFi connectivity

### TemperatureController
**Files**: `TemperatureController.cpp`, `TemperatureController.h`
- Central orchestrator for all system operations
- Manages 60 measurement points
- Coordinates between all subsystems
- Implements main control loop timing

### ConfigManager
**Files**: `ConfigManager.cpp`, `ConfigManager.h`
- Web server implementation (port 80)
- REST API endpoints
- ConfigAssist integration
- Settings persistence to LittleFS
- CSV import/export coordination

### MQTTManager
**Files**: `MQTTManager.cpp`, `MQTTManager.h`
- Static class design (singleton pattern)
- 256dpi/MQTT library client
- TLS/SSL support
- ISA-95 topic hierarchy
- Command processing
- Telemetry publishing

### Alarm System
**Files**: `Alarm.cpp`, `Alarm.h`
- State machine implementation
- Hysteresis management
- Multi-stage alarm progression
- Acknowledgment handling
- Per-point alarm instances

### Measurement Points
**Files**: `MeasurementPoint.cpp`, `MeasurementPoint.h`
- Individual temperature point management
- Min/max tracking
- Alarm threshold configuration
- Sensor association
- Statistics maintenance

### Sensor Management
**Files**: `Sensor.cpp`, `Sensor.h`
- DS18B20 OneWire interface
- PT1000 ADC interface
- Error detection and recovery
- Calibration support
- Temperature conversion

### Logger System
**Files**: `LoggerManager.cpp`, `LoggerManager.h`
- Three logging subsystems:
  - Measurement logs (temperature data)
  - Event logs (system events)
  - Alarm logs (state changes)
- SD card file management
- Daily log rotation
- CSV format output

### Indicator Interface
**Files**: `IndicatorInterface.cpp`, `IndicatorInterface.h`
- OLED display (128x64)
- LED indicators (status, alarm)
- Button input handling
- Display page rotation
- QR code generation (planned)

### Modbus Server
**Files**: `TempModbusServer.cpp`, `TempModbusServer.h`
- Modbus RTU implementation
- RS-485 interface
- Register mapping to system variables
- Command processing

### Register Mapping
**Files**: `RegisterMap.cpp`, `RegisterMap.h`
- Modbus register definitions
- Read/write handlers
- Value validation
- Register 899 command processing

### CSV Configuration
**Files**: `CSVConfigManager.cpp`, `CSVConfigManager.h`
- CSV parsing for configuration
- Alarm settings import/export
- Measurement point configuration
- Validation logic

### Settings Management
**Files**: `SettingsCSVManager.cpp`, `SettingsCSVManager.h`
- Additional CSV operations
- Settings persistence
- Backup/restore functionality

### Time Management
**Files**: `TimeManager.cpp`, `TimeManager.h`
- RTC integration
- NTP synchronization
- Timezone handling
- Timestamp generation

## Data Files (Web Interface)

### HTML Files
- **index.html**: Main dashboard with temperature display
- **settings.html**: System configuration interface
- **alarms.html**: Alarm threshold configuration
- **logs.html**: Log file viewer and download

### JavaScript Files
- **app.js**: Main application logic, API calls
- **language.js**: Translation system for Russian/English

### CSS Files
- **styles.css**: UI styling and responsive design

## Configuration Files

### platformio.ini
Build configuration including:
- Board: ESP-WROVER-KIT
- Framework: Arduino
- Library dependencies
- Build flags and options
- Upload settings

### CLAUDE.md
Instructions for AI assistant:
- Development workflow
- Code standards
- Documentation requirements
- Testing procedures

## Library Dependencies

Key external libraries (managed via PlatformIO):
- **OneWire**: DS18B20 sensor communication
- **DallasTemperature**: DS18B20 temperature conversion
- **ModbusClient**: Modbus RTU server
- **256dpi/MQTT**: MQTT client with SSL support
- **ConfigAssist**: Configuration management
- **ArduinoJson**: JSON parsing
- **SSD1306**: OLED display driver
- **QRCode**: QR code generation
- **ESPAsyncWebServer**: Async web server
- **SD**: SD card file system

## Build System

### Compilation Units
Each `.cpp` file is compiled as a separate unit:
1. Source files in `src/` directory
2. Headers in `include/` directory provide interfaces
3. Libraries linked from `.pio/libdeps/`

### Memory Layout
- **Flash**: ~1.4MB (application + web files)
- **RAM**: ~140KB runtime usage
- **SPIFFS/LittleFS**: Configuration storage
- **SD Card**: Log file storage

## Development Workflow

### File Organization Rules
1. **Headers** (`include/`): Public interfaces, class definitions
2. **Source** (`src/`): Implementation details, private methods
3. **Data** (`data/`): Web resources uploaded to SPIFFS
4. **Docs** (`docs/`): Markdown documentation

### Naming Conventions
- **Classes**: PascalCase (e.g., `TemperatureController`)
- **Methods**: camelCase (e.g., `getMeasurementPoint`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_POINTS`)
- **Files**: Match class names

### Module Dependencies
```
main.cpp
    └── TemperatureController
        ├── ConfigManager
        ├── MQTTManager
        ├── LoggerManager
        ├── IndicatorInterface
        ├── TempModbusServer
        │   └── RegisterMap
        ├── MeasurementPoint[]
        │   ├── Sensor
        │   └── Alarm
        └── CSVConfigManager
```

## Testing Structure

### Unit Tests
Located in `test/` directory:
- Component-level testing
- Mock implementations for hardware
- Automated via PlatformIO

### Integration Points
Key integration test areas:
- MQTT + Alarm notifications
- Modbus + Register mapping
- Web API + Configuration
- Sensor + Measurement points

## Documentation

### Code Documentation
- Doxygen comments in headers
- Generated via `scripts/generate_docs.sh`
- HTML output in `docs/doxygen/`

### Architecture Documentation
- Main document: `docs/architecture.md`
- Sharded into topic-specific files
- Maintained in Markdown format

### Requirements Documentation
- PRD: `docs/prd.md`
- User stories: `docs/stories/`
- Epic definitions: `docs/epic-*.md`

## Version Control

### Branch Structure
- **main**: Stable releases
- **claude-branch**: AI assistant working branch
- **feature/***: Feature development
- **bugfix/***: Bug fixes

### Ignored Files
Key patterns in `.gitignore`:
- `.pio/` - Build artifacts
- `.vscode/` - IDE settings
- `*.bak` - Backup files
- Log files and temporary data

## Security Considerations

### Sensitive Files
- WiFi credentials: Stored in ConfigAssist
- MQTT credentials: Encrypted in config
- API keys: Not committed to repository

### Access Control
- Web interface: Optional authentication
- MQTT: Username/password + TLS
- Modbus: Physical access required

## Performance Optimization

### Critical Paths
1. Sensor reading: 1-second intervals
2. Display update: 100ms refresh
3. MQTT publish: Configurable interval
4. Modbus response: <100ms target

### Memory Management
- Static allocation preferred
- Shared buffers for large operations
- Careful String usage to avoid fragmentation

## Future Enhancements

### Planned Additions
1. QR code display module
2. Russian language files
3. Enhanced MQTT commands
4. Modbus register 899 implementation

### Extension Points
- New sensor types via Sensor interface
- Additional display pages
- Custom MQTT topics
- Extended API endpoints

---

*This document provides a comprehensive overview of the Temperature Controller source tree structure, serving as a reference for developers working with the codebase.*