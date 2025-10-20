# Product Requirements Document (PRD)
## Temperature Controller System Enhancement

**Version**: 1.1  
**Date**: 2024-10-18 (Updated: 2024-10-20)  
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
- Configurable message history logging
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

#### FR1.7: MQTT History and Logging
- Optional MQTT message logging to SD card
- User-configurable enable/disable via web interface
- CSV format for browser-side parsing: timestamp,direction,topic,size,status,priority,message_preview
- Automatic log rotation with configurable retention (1-30 days, default 7)
- Web interface for viewing and filtering MQTT history
- No server-side JSON generation (browser parses CSV)
- Performance consideration: disabled by default

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
- MQTT logging when enabled: Additional <2% CPU for SD writes
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
6. MQTT history logging framework

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
- [ ] MQTT history viewer functional with <2% additional overhead
- [ ] 100% Russian translation coverage
- [ ] QR code scannable in <2 seconds
- [ ] Zero Modbus configuration corruptions
- [ ] User documentation updated throughout development

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

## Epic & Story Structure

### Testing Documentation Requirements

**IMPORTANT**: Each story MUST have an associated manual testing instructions file that provides detailed, step-by-step testing procedures for developers and QA engineers.

#### Manual Testing File Requirements:
1. **File Location**: `/docs/testing/manual/[story-number]-[story-name]-test.md`
2. **File Naming Convention**: Use story number and brief descriptive name (e.g., `1.1-mqttmanager-test.md`)
3. **Content Structure**:
   - Test Environment Setup
   - Required Tools and Software
   - Pre-test Configuration
   - Step-by-step Test Procedures
   - Expected Results for Each Step
   - Error Scenarios to Test
   - Performance Benchmarks
   - Rollback Verification
   - Test Data Cleanup

#### Developer Responsibilities:
1. **Create Initial Test File**: Developer MUST create the test file when starting work on a story
2. **Update During Development**: As code changes, the test procedures MUST be updated to reflect:
   - New functionality added
   - Changed behavior
   - Additional edge cases discovered
   - Performance metrics observed
   - Any deviations from original requirements
3. **Maintain Accuracy**: Test instructions must always match the current implementation
4. **Document Test Results**: Include actual test results and observations in a "Test Execution Log" section
5. **Version Control**: Test files must be committed alongside code changes

#### Manual Testing File Template:
```markdown
# Manual Testing Instructions: [Story Number] [Story Title]

## Test Environment Setup
- Hardware requirements
- Software tools needed
- Network configuration

## Pre-Test Checklist
- [ ] Configuration steps
- [ ] Required dependencies

## Test Procedures

### Test Case 1: [Description]
**Objective**: What this test validates

**Steps**:
1. Step with specific action
2. Expected observation
3. Verification method

**Expected Result**: Clear success criteria

**Failure Scenarios**: What could go wrong

### Test Case 2: [Description]
...

## Performance Tests
- Metrics to monitor
- Acceptable thresholds

## Rollback Testing
- Steps to verify rollback works

## Post-Test Cleanup
- Reset procedures
- Log collection

## Test Execution Log
| Date | Tester | Test Case | Result | Notes |
|------|--------|-----------|--------|-------|
| | | | | |
```

## Epic & Story Structure

### Epic 1: MQTT Integration
**Goal**: Add complete MQTT functionality with telemetry, commands, and alarms

#### Story 1.1: Complete existing MQTTManager implementation
- Finish the partially implemented MQTTManager class
- Ensure MQTTManager is singleton or static class for global access
- Hook publishTemperatureData() into main loop
- Enable the existing test counter publishing
- **User Manual Update**: Add MQTT overview section, broker connection basics
- **Manual Testing**:
  - Connect to HiveMQ Cloud console
  - Verify test counter messages appear
  - Monitor Serial output for connection status
  - Check heap usage via Serial (ESP.getFreeHeap())
  - Test disconnect/reconnect by unplugging router
- **Rollback**: Set MQTT enabled = false in config

#### Story 1.2: Implement MQTT history logging framework
- Add MQTT logging configuration to MQTTConfig structure (enableHistory flag, retentionDays)
- Implement logMQTTMessage() in LoggerManager
- Define CSV format: timestamp,direction,topic,size,status,priority,message_preview
- Create mqtt_log_YYYY-MM-DD.csv files on SD card
- Implement log rotation based on date with configurable retention
- Add logging calls to publish() and messageReceived() methods
- Implement priority levels (NORMAL, HIGH for alarms, AUDIT for commands)
- Add "MQTT History" section to existing settings-mqtt.html:
  - Checkbox for "Enable MQTT Command History"
  - Number input for retention days (1-30, default 7)
  - Button to view MQTT history logs
  - Help text about performance impact
- **User Manual Update**: Document MQTT logging feature and performance implications
- **Manual Testing**:
  - Enable/disable logging via web interface checkbox
  - Verify CSV files created only when enabled
  - Check file format and headers
  - Test log rotation at midnight
  - Verify retention period works (change system date to test)
  - Verify no performance impact when disabled
  - Monitor SD card writes with Serial output
- **Rollback**: Set enableHistory = false in config

#### Story 1.3: Implement temperature telemetry publishing  
- Call publishTemperatureData() every 60 seconds
- Publish ONLY configured points (with bound sensors, not all 60)
- Skip points without sensors or disabled points
- Add publishChangedValues() for delta updates
- Integrate with MQTT logging (if enabled)
- **User Manual Update**: Document telemetry data format and publishing intervals
- **Manual Testing**:
  - Configure only 10 points with sensors
  - Use MQTT Explorer to subscribe to telemetry topic
  - Verify JSON contains only configured points
  - Test with 10-second intervals temporarily
  - Monitor Serial for timing accuracy
  - Verify telemetry messages in mqtt_log when logging enabled
  - Check message size is reduced with fewer points
- **Rollback**: Comment out publish calls in main loop

#### Story 1.4: Add system status publishing
- Implement publishSystemStatus() method
- Include uptime, memory, WiFi RSSI, sensor counts
- Publish every 5 minutes to status topic
- Add device info (firmware version, MAC address)
- Log status publishes to mqtt_log file
- **User Manual Update**: Add system status monitoring section
- **Manual Testing**:
  - Subscribe to status topic in MQTT Explorer
  - Verify 5-minute publishing interval
  - Check all status fields populated correctly
  - Force WiFi reconnect and verify RSSI updates
  - Verify status messages in mqtt_log file
- **Rollback**: Disable status publishing flag

#### Story 1.5: Implement alarm MQTT notifications
- Add MQTT notification calls in Alarm::updateState()
- Publish to alarm topic on state transitions
- Use QoS 1 for alarm messages
- Include all alarm details (type, priority, value, threshold)
- Log alarm notifications with priority flag
- **User Manual Update**: Document alarm notification format and priorities
- **Manual Testing**:
  - Heat/cool sensors to trigger alarms
  - Verify alarm messages in MQTT Explorer
  - Test all alarm types (LOW/HIGH/CRITICAL)
  - Verify state transition messages
  - Check mqtt_log shows alarm messages with HIGH priority
- **Rollback**: Remove MQTT calls from Alarm class

#### Story 1.6: Add command subscription and parser
- Implement processIncomingMessage() in MQTTManager
- Subscribe to command/request topic
- Parse JSON command structure
- Add command validation and error responses
- Log all received commands to mqtt_log
- **User Manual Update**: Add MQTT commands overview section
- **Manual Testing**:
  - Send test commands via MQTT Explorer
  - Verify Serial logs show received commands
  - Test malformed JSON handling
  - Verify commands logged with timestamp
  - Test rate limiting (flood with commands)
- **Rollback**: Unsubscribe from command topic

#### Story 1.7: Implement essential read commands
- Add handlers for get_status, get_points_data
- Create command response framework
- Publish responses to command/response topic
- Add rate limiting (10 commands/second)
- Log command responses
- **User Manual Update**: Document read commands with examples
- **Manual Testing**:
  - Send get_status command, verify response
  - Test get_points_data with various point lists
  - Verify response times < 500ms (Serial timing)
  - Check mqtt_log shows request/response pairs
  - Test with invalid point numbers
- **Rollback**: Return "not implemented" for all commands

#### Story 1.8: Implement control commands
- Add acknowledge_alarm command with validation
- Implement send_message for OLED display
- Add set_alarm_threshold with range validation
- Log all control commands for audit with AUDIT flag
- **User Manual Update**: Document control commands, safety considerations
- **Manual Testing**:
  - Trigger alarm, then acknowledge via MQTT
  - Verify OLED displays sent messages
  - Test threshold changes via MQTT
  - Verify AUDIT entries in mqtt_log
  - Test acknowledgment of non-existent alarms
- **Rollback**: Disable control commands via config flag

#### Story 1.9: Add MQTT history viewer and optimization
- Create new mqtt-history.html page for viewing logs
- Add link to MQTT History page from settings-mqtt.html
- Implement JavaScript to:
  - Fetch list of mqtt_log_*.csv files via API
  - Parse CSV content in browser (no server-side JSON generation)
  - Display in sortable/filterable table
  - Filter by direction (IN/OUT/ALL), topic pattern, time range
  - Show message preview with expandable full content
  - Download individual log files
- Add API endpoints in ConfigManager:
  - GET /api/mqtt/logs - list available log files
  - GET /api/mqtt/logs/{filename} - get log file content
  - DELETE /api/mqtt/logs/{filename} - delete old logs
- Implement offline message queuing (100 messages)
- Optimize message sizes and frequencies
- **User Manual Update**: Document MQTT history viewer usage and navigation
- **Manual Testing**:
  - Navigate to MQTT History page from settings
  - Verify log file list appears (or empty message if disabled)
  - Test CSV parsing with various message sizes
  - Test all filters (direction, topic, date range)
  - Verify sorting by timestamp, topic, size
  - Test message preview expansion
  - Download log files and verify content
  - Test with history disabled - should show appropriate message
  - Test performance with large log files (1000+ entries)
  - Perform 24-hour stability test
- **Rollback**: Remove history page link, disable advanced features

### Epic 2: System Fine-tuning and Additional Features
**Goal**: Complete Russian translation, QR code display, and Modbus safety

#### Story 2.1: Implement Russian translation framework
- Create language switching mechanism
- Extract all UI strings to translation files
- Add language selection to settings
- Default to English if not configured
- Keep Serial debug logs in English (for development)
- Translate only: web UI, system event logs, alarm descriptions
- **User Manual Update**: Add language switching instructions (RU)
- **Manual Testing**:
  - Switch language via web settings
  - Verify language persists after reboot
  - Test fallback to English
  - Verify Serial logs remain in English
  - Check system event logs use selected language
  - Verify MQTT history page translates
- **Rollback**: Force English language

#### Story 2.2: Translate all web interface pages
- Translate all 8 HTML pages + new MQTT history page to Russian
- Localize JavaScript alert/confirm messages
- Translate form validation messages
- Update dynamic content generation
- **User Manual Update**: Translate existing sections to Russian
- **Manual Testing**:
  - Navigate all pages in Russian
  - Test all form submissions
  - Verify all alerts/confirms in Russian
  - Test MQTT history page in Russian
  - Check special characters display correctly
- **Rollback**: Serve English HTML files

#### Story 2.3: Add QR code display functionality
- Integrate QR code library (test memory impact)
- Generate QR with device IP address
- Add new OLED display page for QR
- Update button navigation logic
- **User Manual Update**: Add QR code usage section with images
- **Manual Testing**:
  - Long press button to enter QR mode
  - Scan QR with multiple phone types
  - Verify URL opens in browser
  - Test QR readability from 30cm
  - Test display timeout after 30s
- **Rollback**: Skip QR page in display rotation

#### Story 2.4: Implement WiFi setup QR for AP mode
- Generate WiFi connection QR in AP mode
- Include SSID and password in QR
- Add setup URL to QR data
- **User Manual Update**: Document AP mode QR setup process
- **Manual Testing**:
  - Enter AP mode
  - Scan QR with iOS/Android devices
  - Verify auto-connection to AP
  - Test password-protected AP
  - Verify web UI accessible after connection
- **Rollback**: Display text-only connection info

#### Story 2.5: Complete Modbus register 899 implementation
- Add command handler for register 899
- Implement configuration validation logic
- Add command codes (apply, reset, clear, factory)
- Require confirmation sequence for safety
- **User Manual Update**: Document Modbus command register usage
- **Manual Testing**:
  - Use Modbus simulator (ModbusPoll)
  - Test all command codes (0x01-0xFF)
  - Verify threshold validation
  - Test timeout on incomplete commands
  - Verify changes logged to Serial
- **Rollback**: Return error for all register 899 writes

#### Story 2.6: Add safety mechanisms and logging
- Implement timeout protection for commands
- Add configuration rollback on error
- Log all Modbus configuration changes
- Integrate Modbus changes with mqtt_log when MQTT enabled
- Validate threshold ranges and hysteresis
- **User Manual Update**: Add troubleshooting section
- **Manual Testing**:
  - Test command timeout (5s)
  - Verify all Modbus changes logged
  - Test invalid configuration rejection
  - Verify rollback on validation failure
  - Check integrated logging when MQTT active
- **Rollback**: Make register 899 read-only

#### Story 2.7: Refactor event logs viewer to prevent controller suspension
- Replace current event-logs.html JSON generation with file download approach
- Remove server-side JSON generation that causes controller to hang
- Implement new workflow:
  - Page requests list of available log files
  - User selects date range or specific files
  - Browser downloads CSV files directly
  - JavaScript parses CSV client-side
  - Display in paginated table (100 entries per page)
- Add virtual scrolling for large files
- Implement same filtering as dashboard trends
- **User Manual Update**: Document new logs viewer interface
- **Manual Testing**:
  - Load page with large log files (10,000+ entries)
  - Verify controller doesn't suspend/hang
  - Test file download progress indication
  - Verify CSV parsing handles all event types
  - Test pagination and filtering
  - Check memory usage in browser
  - Compare performance with old implementation
- **Rollback**: Restore old event-logs.html

#### Story 2.8: Final User Manual Review and Alignment
- Review entire USER_MANUAL_RU.md
- Verify all features documented accurately
- Add missing sections for new features
- Update screenshots
- Create quick reference card
- **Manual Testing**:
  - Follow guide as new user
  - Verify all procedures match actual device behavior
  - Test every documented workflow
  - Have Russian speaker review grammar
  - Verify MQTT examples work
  - Check QR code instructions accurate
  - Validate troubleshooting steps
- **Rollback**: Not applicable

## Migration Strategy

### Rollback Strategy
Each story includes specific rollback steps. General rollback approach:
1. **Level 1**: Disable specific feature via configuration
2. **Level 2**: Comment out feature code
3. **Level 3**: Revert to previous firmware
4. **Level 4**: Factory reset to original firmware

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