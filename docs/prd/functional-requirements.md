# Functional Requirements

## FR1: MQTT Integration

### FR1.1: MQTT Client Core
- Connect to configurable MQTT broker with authentication
- Support TLS/SSL encryption (optional)
- Auto-reconnect with exponential backoff
- Configurable client ID and credentials
- Configurable message history logging
- Maximum 30KB additional RAM usage

### FR1.2: Topic Structure
Default structure: `tempcontroller/<type>/<subtopic>`

Types:
- `telemetry/temperature` - Bulk temperature data
- `telemetry/changes` - Changed values only
- `telemetry/system` - System status
- `command/request` - Incoming commands
- `command/response` - Command responses
- `alarm/active` - New alarm notifications
- `alarm/state` - State changes

### FR1.3: Temperature Telemetry
Publish every 60 seconds (configurable 10-3600s) - only configured points with bound sensors:
```json
{
  "timestamp": "2024-10-18T10:00:00Z",
  "device_id": "tempcontroller01",
  "points": {
    "0": {"name": "Reactor Core", "temp": 75.3, "status": "OK"},
    "5": {"name": "Heat Exchanger", "temp": 85.2, "status": "OK"},
    "12": {"name": "Cooling Tower", "temp": 45.1, "status": "OK"}
    // ... only configured points, not all 60
  },
  "summary": {
    "configured_points": 15,
    "active_points": 12,
    "sensor_errors": 2,
    "active_alarms": 3
  }
}
```

### FR1.4: Essential Commands
1. **get_status** - System health check
2. **get_points_data** - Query specific points with current temp and alarms
3. **acknowledge_alarm** - Remote alarm acknowledgment
4. **send_message** - Display message on OLED with buzzer

### FR1.5: Control Commands
1. **get_all_points** - Full system state
2. **set_alarm_threshold** - Update alarm limits
3. **control_relay** - Manual relay control
4. **get_historical_data** - Query logged data

### FR1.6: Alarm Integration
- Publish on any alarm state change
- Include priority, temperature, threshold
- QoS 2 for critical alarms
- Support bulk acknowledgment

### FR1.7: MQTT History and Logging
- Optional MQTT message logging to SD card
- User-configurable enable/disable via web interface
- CSV format for browser-side parsing: timestamp,direction,topic,size,status,priority,message_preview
- Automatic log rotation with configurable retention (1-30 days, default 7)
- Web interface for viewing and filtering MQTT history
- No server-side JSON generation (browser parses CSV)
- Performance consideration: disabled by default

## FR2: Russian Translation

### FR2.1: Web Interface Pages
Translate all 8 HTML pages:
- index.html - Dashboard
- alarms.html - Alarm configuration
- settings.html - General settings
- settings-mqtt.html - MQTT settings
- settings-network.html - Network configuration
- settings-modbus.html - Modbus settings
- logs.html - Event viewer
- about.html - System information

### FR2.2: JavaScript Strings
- All alert() and confirm() dialogs
- Dynamic status messages
- Table headers and labels
- Button texts
- Validation messages

### FR2.3: System Messages
- Alarm descriptions
- Error messages
- Configuration tooltips
- Help text

## FR3: QR Code Display

### FR3.1: QR Code Generation
- Generate QR code containing device access URL
- Update when IP changes
- Include device name for identification
- Maximum 29x29 pixels for 128x64 OLED

### FR3.2: Display Integration
- Show QR code in system status mode
- Accessible via long button press (>3s)
- New status page in rotation
- Auto-timeout after 30s

### FR3.3: QR Code Content
**Normal Mode**:
```
http://192.168.1.100
```

**AP Mode**:
```
WIFI:T:WPA;S:TempController-AP;P:12345678;H:false;;
URL:http://192.168.4.1
```

### FR3.4: User Flow
1. Long press button → Enter status mode
2. Navigate to QR code page (page 6)
3. Scan with mobile device
4. Auto-connect to WiFi (if AP mode)
5. Auto-open web interface

## FR4: Modbus RTU Completion

### FR4.1: Register 899 Command Processing
- Implement write handler for register 899
- Define command codes:
  - 0x01: Apply alarm configuration
  - 0x02: Reset min/max values
  - 0x03: Clear alarm history
  - 0xFF: Factory reset

### FR4.2: Configuration Validation
- Verify threshold ranges (-50 to 150°C)
- Ensure low < high thresholds
- Validate priority values (0-3)
- Check hysteresis (0.1 to 10.0)

### FR4.3: Safety Mechanisms
- Require confirmation sequence
- 5-second timeout for multi-step commands
- Log all changes with timestamp
- Reject invalid configurations
