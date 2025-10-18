# Product Requirements Document (PRD)
## Temperature Controller System Enhancement

**Version**: 1.0  
**Date**: 2024-10-18  
**Author**: John (Product Manager)  
**Project Type**: Brownfield Enhancement

---

## Executive Summary

This PRD outlines the enhancement of an existing ESP32-based industrial temperature monitoring system with three major features:

1. **MQTT Integration** - Enable remote monitoring and control via MQTT protocol
2. **Russian Translation** - Complete UI localization for Russian-speaking operators
3. **QR Code Access** - Quick web interface access via QR code on OLED display
4. **Modbus RTU Completion** - Finalize register 899 command processing

The system currently monitors 60 temperature measurement points with comprehensive alarm management, and these enhancements will extend its capabilities for remote operations, international deployment, and improved user experience.

## Current System State

### Existing Capabilities
- **Hardware**: ESP32-based controller with 60 measurement points (50 DS18B20 + 10 PT1000)
- **Interfaces**: Web UI, OLED display, 3 relays, 4 LED indicators, Modbus RTU
- **Alarm System**: Three alarm types per point with state machine, priorities, and hysteresis
- **Data Logging**: Temperature and event logging to SD card
- **Network**: WiFi connectivity with web server

### Technical Stack
- Platform: PlatformIO with ESP32
- Core Libraries: ConfigAssist, OneWire, DallasTemperature, ModbusMaster
- Web: HTML/CSS/JS with Chart.js for trending
- Configuration: YAML-based with persistent storage

## Business Requirements

### BR1: Remote Monitoring and Control
**Problem**: Operators need to monitor temperature data and respond to alarms without physical presence at the facility.

**Solution**: Implement MQTT protocol for:
- Real-time temperature telemetry for all 60 points
- Instant alarm notifications with priority levels
- Remote acknowledgment and control capabilities
- Integration with n8n for Telegram notifications

**Value**: Reduce response time to critical alarms by 80%, enable 24/7 monitoring without on-site staff.

### BR2: Multi-language Support
**Problem**: Russian-speaking operators struggle with English-only interface, leading to operational errors.

**Solution**: Complete Russian translation of:
- All web interface pages and elements
- Error messages and notifications
- Configuration parameters and help text

**Value**: Improve operator efficiency and reduce configuration errors by 60%.

### BR3: Quick Access to Web Interface
**Problem**: Operators waste time manually entering IP addresses, especially during commissioning or AP mode setup.

**Solution**: Display QR code on OLED containing:
- Current device IP address
- Direct URL to web interface
- AP mode connection details when applicable
- Auto-connection parameters for mobile devices

**Value**: Reduce setup time by 90%, improve field technician productivity.

### BR4: Reliable Modbus Configuration
**Problem**: Accidental configuration changes from Modbus masters during startup can corrupt alarm settings.

**Solution**: Implement explicit command trigger for register 899:
- Validate all configuration values before applying
- Require specific command sequence
- Log all Modbus configuration changes

**Value**: Prevent 100% of accidental configuration corruption incidents.

## Functional Requirements

### FR1: MQTT Integration

#### FR1.1: MQTT Client Core
- Connect to configurable MQTT broker with authentication
- Support TLS/SSL encryption (optional)
- Auto-reconnect with exponential backoff
- Configurable client ID and credentials
- Maximum 30KB additional RAM usage

#### FR1.2: Topic Structure
Default structure: `tempcontroller/<type>/<subtopic>`

Types:
- `telemetry/temperature` - Bulk temperature data
- `telemetry/changes` - Changed values only
- `telemetry/system` - System status
- `command/request` - Incoming commands
- `command/response` - Command responses
- `alarm/active` - New alarm notifications
- `alarm/state` - State changes

#### FR1.3: Temperature Telemetry
Publish every 60 seconds (configurable 10-3600s):
```json
{
  "timestamp": "2024-10-18T10:00:00Z",
  "device_id": "tempcontroller01",
  "points": {
    "0": {"name": "Reactor Core", "temp": 75.3, "status": "OK"},
    // ... all 60 points
  },
  "summary": {
    "active_points": 45,
    "sensor_errors": 2,
    "active_alarms": 3
  }
}
```

#### FR1.4: Essential Commands
1. **get_status** - System health check
2. **get_points_data** - Query specific points with current temp and alarms
3. **acknowledge_alarm** - Remote alarm acknowledgment
4. **send_message** - Display message on OLED with buzzer

#### FR1.5: Control Commands
1. **get_all_points** - Full system state
2. **set_alarm_threshold** - Update alarm limits
3. **control_relay** - Manual relay control
4. **get_historical_data** - Query logged data

#### FR1.6: Alarm Integration
- Publish on any alarm state change
- Include priority, temperature, threshold
- QoS 2 for critical alarms
- Support bulk acknowledgment

### FR2: Russian Translation

#### FR2.1: Web Interface Pages
Translate all 8 HTML pages:
- index.html - Dashboard
- alarms.html - Alarm configuration
- settings.html - General settings
- settings-mqtt.html - MQTT settings
- settings-network.html - Network configuration
- settings-modbus.html - Modbus settings
- logs.html - Event viewer
- about.html - System information

#### FR2.2: JavaScript Strings
- All alert() and confirm() dialogs
- Dynamic status messages
- Table headers and labels
- Button texts
- Validation messages

#### FR2.3: System Messages
- Alarm descriptions
- Error messages
- Configuration tooltips
- Help text

### FR3: QR Code Display

#### FR3.1: QR Code Generation
- Generate QR code containing device access URL
- Update when IP changes
- Include device name for identification
- Maximum 29x29 pixels for 128x64 OLED

#### FR3.2: Display Integration
- Show QR code in system status mode
- Accessible via long button press (>3s)
- New status page in rotation
- Auto-timeout after 30s

#### FR3.3: QR Code Content
**Normal Mode**:
```
http://192.168.1.100
```

**AP Mode**:
```
WIFI:T:WPA;S:TempController-AP;P:12345678;H:false;;
URL:http://192.168.4.1
```

#### FR3.4: User Flow
1. Long press button → Enter status mode
2. Navigate to QR code page (page 6)
3. Scan with mobile device
4. Auto-connect to WiFi (if AP mode)
5. Auto-open web interface

### FR4: Modbus RTU Completion

#### FR4.1: Register 899 Command Processing
- Implement write handler for register 899
- Define command codes:
  - 0x01: Apply alarm configuration
  - 0x02: Reset min/max values
  - 0x03: Clear alarm history
  - 0xFF: Factory reset

#### FR4.2: Configuration Validation
- Verify threshold ranges (-50 to 150°C)
- Ensure low < high thresholds
- Validate priority values (0-3)
- Check hysteresis (0.1 to 10.0)

#### FR4.3: Safety Mechanisms
- Require confirmation sequence
- 5-second timeout for multi-step commands
- Log all changes with timestamp
- Reject invalid configurations

## Non-Functional Requirements

### NFR1: Performance
- MQTT operations: <5% CPU overhead
- Command response: <500ms
- Memory usage: <30KB additional for MQTT
- Telemetry processing: <100ms
- Message delivery: 99.9% reliability (QoS 1)

### NFR2: Compatibility
- Maintain all existing functionality
- No changes to hardware pin definitions
- Preserve Modbus register map
- Keep web API endpoints

### NFR3: Reliability
- System uptime: 99.9% minimum
- Automatic recovery from network failures
- Graceful degradation without MQTT
- Configuration persistence across restarts

### NFR4: Security
- MQTT authentication required
- Optional TLS/SSL support
- Rate limiting for commands
- Audit logging for configuration changes

### NFR5: Usability
- QR code readable from 30cm distance
- Russian translations professionally reviewed
- Intuitive command structure
- Clear error messages

## Implementation Approach

### Phase 1: MQTT Core & Essential Features (Week 1)
1. Implement MQTTManager class
2. Basic broker connection
3. Temperature telemetry publishing
4. Essential commands (status, acknowledge)
5. Alarm notifications

### Phase 2: Russian Translation (Week 2)
1. Extract all strings to language files
2. Professional translation
3. Implement language switching
4. Test all UI elements
5. Update documentation

### Phase 3: Advanced MQTT & QR Code (Week 3)
1. Control commands implementation
2. QR code generation library
3. OLED display integration
4. Status page addition
5. Historical data queries

### Phase 4: Modbus Completion & Testing (Week 4)
1. Register 899 command handler
2. Validation logic
3. Integration testing
4. Performance optimization
5. Documentation updates

## Success Metrics

### Technical Metrics
- [ ] All 60 points publishing via MQTT
- [ ] <5% CPU overhead confirmed
- [ ] 100% Russian translation coverage
- [ ] QR code scannable in <2 seconds
- [ ] Zero Modbus configuration corruptions

### Operational Metrics
- [ ] 80% reduction in alarm response time
- [ ] 90% reduction in setup time via QR
- [ ] 60% fewer configuration errors
- [ ] 100% uptime over 30 days

### Business Metrics
- [ ] Remote monitoring deployed at 5+ sites
- [ ] Positive feedback from Russian operators
- [ ] Integration with 2+ SCADA systems
- [ ] ROI achieved within 6 months

## Risk Assessment

### Technical Risks
1. **Memory constraints** - Mitigation: Optimize message sizes, use heap monitoring
2. **Network stability** - Mitigation: Implement robust reconnection logic
3. **QR code readability** - Mitigation: Test multiple QR libraries, optimize size

### Implementation Risks
1. **Translation quality** - Mitigation: Use professional service, operator review
2. **Breaking changes** - Mitigation: Comprehensive regression testing
3. **Integration conflicts** - Mitigation: Careful event system design

## Dependencies

### External Dependencies
- MQTT broker availability for testing
- Professional translation service
- QR code generation library selection
- Test devices for validation

### Internal Dependencies
- No changes to core sensor reading logic
- Maintain existing alarm state machine
- Preserve configuration structure
- Keep current web server implementation

## Migration Strategy

### Pre-deployment
1. Full backup of existing configuration
2. Document current Modbus register values
3. Test MQTT broker connectivity
4. Verify translation completeness

### Deployment
1. Deploy to test device first
2. Validate all existing features work
3. Enable MQTT in stages
4. Monitor system resources

### Post-deployment
1. Monitor error logs for 48 hours
2. Gather operator feedback
3. Fine-tune performance parameters
4. Document lessons learned

## Appendix

### A. MQTT Command Reference
See `/docs/mqtt_simplified_architecture.md` for complete command specifications.

### B. Russian Translation Glossary
Key terms requiring consistent translation:
- Temperature → Температура
- Alarm → Тревога
- Threshold → Порог
- Sensor → Датчик
- Error → Ошибка

### C. QR Code Specifications
- Library: QRCode by Richard Moore
- Error correction: Medium (15%)
- Module size: 1 pixel
- Quiet zone: 2 pixels

---

*End of PRD*