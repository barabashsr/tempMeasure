# Industrial ESP32 Temperature Monitoring System

## ⚠️ MANDATORY: AI Assistant Requirements

**ALL AI assistants working on this project MUST:**
1. ✅ Read and follow `/CLAUDE.md` instructions EXACTLY
2. ✅ Create baseline commits before any changes
3. ✅ Use the documented session workflow
4. ✅ Generate documentation before and after changes
5. ✅ Follow documentation-first approach

**Failure to follow CLAUDE.md will result in rejected contributions.**

**🔴 CRITICAL**: Before making ANY changes:
- Read the complete CLAUDE.md file 
- Create baseline commit: `git add -A && git commit -m "baseline: pre-session $(date +%Y%m%d_%H%M%S)"`
- Start session: `.claude/scripts/start_session.sh`
- Generate docs: `scripts/generate_docs.sh`

---

## Device Description

This industrial-grade ESP32-based device is designed for precision temperature monitoring in industrial environments. It collects temperature data from multiple sensors, including DS18B20 digital temperature sensors and PT1000/PT100 RTD sensors connected via MAX31865 modules. The system supports up to 50 DS18B20 sensors and 10 PT1000/PT100 sensors, with expansion capability for future sensor types.

### Key Features:
- Multi-sensor support (DS18B20 and PT1000/PT100)
- MODBUS-RTU communication over RS485
- Web interface for configuration and monitoring
- Configurable alarm thresholds for each sensor
- Error detection and reporting
- Temperature range: -40°C to +200°C (integer Celsius values)
- Industrial-grade reliability and accuracy

### Communication Interfaces:
- RS485 MODBUS-RTU for industrial system integration
- Web interface for configuration and monitoring
- Temperature data accessible via both interfaces

## MODBUS Register Map

### Device Information Registers (0-99)
| Register | Description | Data Type | Access |
|----------|-------------|-----------|--------|
| 0 | Device ID/Model Number | UINT16 | R |
| 1 | Firmware Version | UINT16 | R |
| 2 | Number of Active DS18B20 Sensors | UINT16 | R |
| 3 | Number of Active PT1000/PT100 Sensors | UINT16 | R |
| 4-10 | Device Status and Diagnostics | UINT16 | R |
| 11 | Relay 1 Status (bit0: commanded, bit1: actual) | UINT16 | R |
| 12 | Relay 2 Status (bit0: commanded, bit1: actual) | UINT16 | R |
| 13 | Relay 3 Status (bit0: commanded, bit1: actual) | UINT16 | R |
| 14-99 | Reserved for Future Use | - | - |

### Temperature Data Registers (100-299)
| Register Range | Description | Data Type | Access |
|----------------|-------------|-----------|--------|
| 100-149 | Current Temperature Readings - DS18B20 (addresses 0-49) | INT16 | R |
| 150-159 | Current Temperature Readings - PT1000/PT100 (addresses 50-59) | INT16 | R |
| 160-199 | Reserved for Future Sensor Types | - | - |
| 200-249 | Min Temperature Readings - DS18B20 (addresses 0-49) | INT16 | R |
| 250-259 | Min Temperature Readings - PT1000/PT100 (addresses 50-59) | INT16 | R |
| 260-299 | Reserved for Future Sensor Types | - | - |

### Max Temperature Registers (300-399)
| Register Range | Description | Data Type | Access |
|----------------|-------------|-----------|--------|
| 300-349 | Max Temperature Readings - DS18B20 (addresses 0-49) | INT16 | R |
| 350-359 | Max Temperature Readings - PT1000/PT100 (addresses 50-59) | INT16 | R |
| 360-399 | Reserved for Future Sensor Types | - | - |

### Alarm and Error Registers (400-599)
| Register Range | Description | Data Type | Access |
|----------------|-------------|-----------|--------|
| 400-449 | Alarm Status - DS18B20 (addresses 0-49) | UINT16 | R |
| 450-459 | Alarm Status - PT1000/PT100 (addresses 50-59) | UINT16 | R |
| 460-499 | Reserved for Future Sensor Types | - | - |
| 500-549 | Error Status - DS18B20 (addresses 0-49) | UINT16 | R |
| 550-559 | Error Status - PT1000/PT100 (addresses 50-59) | UINT16 | R |
| 560-599 | Reserved for Future Sensor Types | - | - |

### Configuration Registers (600-799)
| Register Range | Description | Data Type | Access |
|----------------|-------------|-----------|--------|
| 600-649 | Low Temperature Alarm Thresholds - DS18B20 (addresses 0-49) | INT16 | R/W |
| 650-659 | Low Temperature Alarm Thresholds - PT1000/PT100 (addresses 50-59) | INT16 | R/W |
| 660-699 | Reserved for Future Sensor Types | - | - |
| 700-749 | High Temperature Alarm Thresholds - DS18B20 (addresses 0-49) | INT16 | R/W |
| 750-759 | High Temperature Alarm Thresholds - PT1000/PT100 (addresses 50-59) | INT16 | R/W |
| 760-799 | Reserved for Future Sensor Types | - | - |

### Alarm Control Registers (800-899)
| Register Range | Description | Data Type | Access |
|----------------|-------------|-----------|--------|
| 800-849 | Alarm Configuration - DS18B20 (addresses 0-49) | UINT16 | R/W |
| 850-859 | Alarm Configuration - PT1000/PT100 (addresses 50-59) | UINT16 | R/W |
| 860-862 | Relay Control (Relay 1-3) | UINT16 | R/W |
| 863-865 | Relay Status (Relay 1-3) | UINT16 | R |
| 870-889 | Hysteresis Configuration | UINT16 | R/W |
| 899 | Command Execution Register | UINT16 | W |

## Alarm Configuration Bit Definitions (Registers 800-859)
Each alarm configuration register contains:
- Bit 0: Low Temperature Alarm Enable
- Bit 1: High Temperature Alarm Enable
- Bit 2: Sensor Error Alarm Enable
- Bits 3-4: Low Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 5-6: High Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 7-8: Sensor Error Priority (0=Low, 1=Medium, 2=High, 3=Critical)

## Relay Control Values (Registers 860-862)
- 0: Auto (follows alarm logic)
- 1: Force Off
- 2: Force On

## Command Register Values (Register 899)
- 0x0001: Apply alarm configuration

## Alarm Status Bit Definitions
Each alarm status register contains the following bit flags:
- Bit 0: Low Temperature Alarm
- Bit 1: High Temperature Alarm
- Bits 2-15: Reserved for future alarm types

## Error Status Bit Definitions
Each error status register contains the following bit flags:
- Bit 0: Sensor Communication Error
- Bit 1: Sensor Out of Range
- Bit 2: Sensor Disconnected
- Bits 3-15: Reserved for future error types

## Notes
- All temperature values are in integer degrees Celsius
- Valid temperature range: -40°C to +200°C
- MODBUS function code 0x03 (Read Holding Registers) for reading values
- MODBUS function code 0x06 (Write Single Register) for writing configuration
- MODBUS function code 0x10 (Write Multiple Registers) for writing multiple configuration values
