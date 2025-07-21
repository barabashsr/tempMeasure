# Modbus RTU Register Map Documentation

## Overview
This document describes the complete Modbus RTU register map for the Temperature Controller system. All registers use Modbus function codes 03 (Read Holding Registers), 06 (Write Single Register), and 16 (Write Multiple Registers).

## Connection Parameters
- **Protocol**: Modbus RTU
- **Interface**: RS-485
- **Default Address**: 1 (configurable 1-247)
- **Baud Rate**: 9600 (configurable: 4800, 9600, 19200, 38400, 57600, 115200)
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1

## Register Map

### Device Information (Registers 0-99) - Read Only

| Register | Description | Data Type | Range | Notes |
|----------|-------------|-----------|-------|-------|
| 0 | Device ID/Model | UINT16 | 0-65535 | User configurable |
| 1 | Firmware Version | UINT16 | 0-9999 | Format: XXYY = XX.YY |
| 2 | Active DS18B20 Count | UINT16 | 0-50 | Currently bound sensors |
| 3 | Active PT1000 Count | UINT16 | 0-10 | Currently bound sensors |
| 4 | System Status | UINT16 | Bitmap | See status bits below |
| 5 | Total Alarm Count | UINT16 | 0-65535 | All active alarms |
| 6 | Critical Alarm Count | UINT16 | 0-65535 | Active critical alarms |
| 7 | High Priority Alarm Count | UINT16 | 0-65535 | Active high alarms |
| 8 | Medium Priority Alarm Count | UINT16 | 0-65535 | Active medium alarms |
| 9 | Low Priority Alarm Count | UINT16 | 0-65535 | Active low alarms |
| 10 | System Uptime (Hours) | UINT16 | 0-65535 | Hours since boot |
| 11-99 | Reserved | - | - | Future use |

#### System Status Bits (Register 4)
- Bit 0: WiFi Connected (1=connected)
- Bit 1: SD Card Present (1=present)
- Bit 2: RTC Valid (1=valid time)
- Bit 3: Any Alarm Active (1=active)
- Bit 4: Configuration Error (1=error)
- Bit 5-15: Reserved

### Temperature Data (Registers 100-399)

#### Current Temperature (100-199)
| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 100-149 | DS18B20 Points 0-49 | INT16 | -400 to 2000 | 0.1°C |
| 150-159 | PT1000 Points 50-59 | INT16 | -400 to 2000 | 0.1°C |
| 160-199 | Reserved | - | - | - |

#### Minimum Temperature (200-299)
| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 200-249 | DS18B20 Points 0-49 | INT16 | -400 to 2000 | 0.1°C |
| 250-259 | PT1000 Points 50-59 | INT16 | -400 to 2000 | 0.1°C |
| 260-299 | Reserved | - | - | - |

#### Maximum Temperature (300-399)
| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 300-349 | DS18B20 Points 0-49 | INT16 | -400 to 2000 | 0.1°C |
| 350-359 | PT1000 Points 50-59 | INT16 | -400 to 2000 | 0.1°C |
| 360-399 | Reserved | - | - | - |

### Alarm Status (Registers 400-499) - Read Only

| Register Range | Description | Data Type | Bitmap Definition |
|----------------|-------------|-----------|-------------------|
| 400-449 | DS18B20 Alarm Status | UINT16 | See alarm bits below |
| 450-459 | PT1000 Alarm Status | UINT16 | See alarm bits below |
| 460-499 | Reserved | - | - |

#### Alarm Status Bits
- Bit 0: Low Temperature Alarm Active
- Bit 1: High Temperature Alarm Active
- Bit 2: Low Temperature Alarm Acknowledged
- Bit 3: High Temperature Alarm Acknowledged
- Bit 4: Sensor Error
- Bit 5: Sensor Disconnected
- Bit 6-7: Alarm Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bit 8-15: Reserved

### Error Status (Registers 500-599) - Read Only

| Register Range | Description | Data Type | Bitmap Definition |
|----------------|-------------|-----------|-------------------|
| 500-549 | DS18B20 Error Status | UINT16 | See error bits below |
| 550-559 | PT1000 Error Status | UINT16 | See error bits below |
| 560-599 | Reserved | - | - |

#### Error Status Bits
- Bit 0: Communication Error
- Bit 1: Out of Range
- Bit 2: Sensor Disconnected
- Bit 3: CRC Error
- Bit 4: Configuration Error
- Bit 5-15: Reserved

### Alarm Thresholds (Registers 600-799) - Read/Write

#### Low Temperature Thresholds (600-699)
| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 600-649 | DS18B20 Low Thresholds | INT16 | -400 to 2000 | 0.1°C |
| 650-659 | PT1000 Low Thresholds | INT16 | -400 to 2000 | 0.1°C |
| 660-699 | Reserved | - | - | - |

#### High Temperature Thresholds (700-799)
| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 700-749 | DS18B20 High Thresholds | INT16 | -400 to 2000 | 0.1°C |
| 750-759 | PT1000 High Thresholds | INT16 | -400 to 2000 | 0.1°C |
| 760-799 | Reserved | - | - | - |

### Alarm Configuration (Registers 800-859) - Read/Write

| Register Range | Description | Data Type | Bit Definition |
|----------------|-------------|-----------|----------------|
| 800-849 | DS18B20 Alarm Config | UINT16 | See config bits |
| 850-859 | PT1000 Alarm Config | UINT16 | See config bits |

#### Alarm Configuration Bits
- Bit 0: Low Temperature Alarm Enable (0=Disabled, 1=Enabled)
- Bit 1: High Temperature Alarm Enable (0=Disabled, 1=Enabled)
- Bit 2: Sensor Error Alarm Enable (0=Disabled, 1=Enabled)
- Bits 3-4: Low Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 5-6: High Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 7-8: Sensor Error Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 9-15: Reserved

Priority Values:
- 0: Low
- 1: Medium
- 2: High
- 3: Critical

### Relay Control (Registers 860-869) - Read/Write

| Register | Description | Values | Notes |
|----------|-------------|--------|-------|
| 860 | Relay 1 (Siren) Control | 0=Auto, 1=Force Off, 2=Force On | Auto follows alarm logic |
| 861 | Relay 2 (Beacon) Control | 0=Auto, 1=Force Off, 2=Force On | Auto follows alarm logic |
| 862 | Relay 3 (Spare) Control | 0=Auto, 1=Force Off, 2=Force On | Auto follows alarm logic |
| 863 | Relay 1 Current State | 0=Off, 1=On | Read only |
| 864 | Relay 2 Current State | 0=Off, 1=On | Read only |
| 865 | Relay 3 Current State | 0=Off, 1=On | Read only |
| 866-869 | Reserved | - | - |

### Hysteresis Configuration (Registers 870-889) - Read/Write

| Register Range | Description | Data Type | Range | Units |
|----------------|-------------|-----------|-------|-------|
| 870-879 | Temperature Hysteresis | UINT16 | 0-100 | 0.1°C |
| 880-889 | Reserved | - | - | - |

### System Commands (Registers 890-899) - Write Only

| Register | Command | Value | Description |
|----------|---------|-------|-------------|
| 899 | Execute Command | 0x0001 | Apply alarm configuration |
| | | 0x0002 | Reset all min/max values |
| | | 0x0003 | Acknowledge all alarms |
| | | 0x0004 | System reboot |
| | | 0x0005 | Save configuration |
| | | 0x0006 | Restore defaults |
| | | 0x0007 | Clear alarm history |
| | | 0x0008 | Force sensor discovery |

### Point Names (Registers 900-959) - Read Only

Each measurement point name uses 10 registers (20 characters):
- Registers 900-909: Point 0 name (DS18B20)
- Registers 910-919: Point 1 name (DS18B20)
- ...continues for first 6 points

Format: 2 ASCII characters per register, high byte first

### Extended Configuration (Registers 960-999) - Read/Write

| Register | Description | Data Type | Range | Units |
|----------|-------------|-----------|-------|-------|
| 960 | Acknowledged Delay Critical | UINT16 | 0-3600 | Seconds |
| 961 | Acknowledged Delay High | UINT16 | 0-3600 | Seconds |
| 962 | Acknowledged Delay Medium | UINT16 | 0-3600 | Seconds |
| 963 | Acknowledged Delay Low | UINT16 | 0-3600 | Seconds |
| 964 | Display Sleep Timeout | UINT16 | 0-3600 | Seconds (0=never) |
| 965 | Alarm Display Cycle Time | UINT16 | 1-60 | Seconds |
| 966 | Beacon Blink On Time | UINT16 | 1-60 | Seconds |
| 967 | Beacon Blink Off Time | UINT16 | 1-300 | Seconds |
| 968-999 | Reserved | - | - | - |

## Usage Examples

### Reading Current Temperature
To read temperature from Point 5 (DS18B20):
```
Request:  01 03 00 69 00 01 [CRC]
Response: 01 03 02 00 EB [CRC]
Result: 235 = 23.5°C
```

### Setting High Temperature Threshold
To set high threshold for Point 0 to 30.0°C:
```
Request:  01 06 02 BC 01 2C [CRC]
Response: 01 06 02 BC 01 2C [CRC]
Result: Threshold set to 300 = 30.0°C
```

### Configuring Alarm Priorities
To set Point 0 with all alarms enabled, High=Critical, Low=Medium, Error=High:
```
Enable bits: 0x07 (all enabled)
Low priority = 1 (Medium) << 3 = 0x08
High priority = 3 (Critical) << 5 = 0x60
Error priority = 2 (High) << 7 = 0x100
Value = 0x07 | 0x08 | 0x60 | 0x100 = 0x16F
Request:  01 06 03 20 01 6F [CRC]
```

### Applying Configuration
After changing alarm configuration, trigger application:
```
Request:  01 06 03 83 00 01 [CRC]
Response: 01 06 03 83 00 01 [CRC]
```

### Reading Multiple Registers
To read all current temperatures for DS18B20 points 0-9:
```
Request:  01 03 00 64 00 0A [CRC]
Response: 01 03 14 00 E1 00 E7 00 EA 00 E5 00 E8 00 E6 00 E9 00 EC 00 E4 00 E3 [CRC]
```

## Error Responses

| Exception Code | Description | Possible Cause |
|----------------|-------------|----------------|
| 01 | Illegal Function | Unsupported function code |
| 02 | Illegal Data Address | Register address out of range |
| 03 | Illegal Data Value | Value outside allowed range |
| 04 | Device Failure | Internal error |

## Best Practices

1. **Configuration Changes**
   - Always read current configuration before modifying
   - Apply changes using command register 899
   - Verify changes by reading back

2. **Alarm Management**
   - Check alarm status before acknowledging
   - Use bulk acknowledge only when necessary
   - Monitor relay states after changes

3. **Performance**
   - Read multiple registers in one request when possible
   - Limit polling frequency to 1-5 seconds
   - Use exception responses to detect issues

4. **Error Handling**
   - Implement timeout handling (recommended 1 second)
   - Retry failed requests up to 3 times
   - Log all communication errors

## Integration Example

```python
import minimalmodbus

# Configure instrument
instrument = minimalmodbus.Instrument('/dev/ttyUSB0', 1)
instrument.serial.baudrate = 9600
instrument.serial.timeout = 1.0

# Read current temperature from point 0
temp_raw = instrument.read_register(100, 0, 3, True)
temperature = temp_raw / 10.0
print(f"Temperature: {temperature}°C")

# Set high alarm threshold to 35°C
instrument.write_register(700, 350, 0, 6, True)

# Configure alarm (all enabled, High=Critical, Low=Medium, Error=High)
# Enable bits: 0x07, Low=Medium(1)<<3, High=Critical(3)<<5, Error=High(2)<<7
alarm_config = 0x07 | (1 << 3) | (3 << 5) | (2 << 7)
instrument.write_register(800, alarm_config, 0, 6, False)

# Apply configuration
instrument.write_register(899, 1, 0, 6, False)
```

## Change Log

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024-01-01 | Initial release |
| 2.0 | 2024-12-30 | Added alarm configuration, relay control, extended status |
| 2.1 | 2025-01-21 | Updated alarm configuration with separate enable bits and priority values |
