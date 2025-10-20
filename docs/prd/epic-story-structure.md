# Epic & Story Structure

## Epic 1: MQTT Integration
**Goal**: Add complete MQTT functionality with telemetry, commands, and alarms

### Story 1.1: Complete existing MQTTManager implementation
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

### Story 1.2: Implement MQTT history logging framework
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

### Story 1.3: Implement temperature telemetry publishing  
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

### Story 1.4: Add system status publishing
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

### Story 1.5: Implement alarm MQTT notifications
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

### Story 1.6: Add command subscription and parser
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

### Story 1.7: Implement essential read commands
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

### Story 1.8: Implement control commands
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

### Story 1.9: Add MQTT history viewer and optimization
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

## Epic 2: System Fine-tuning and Additional Features
**Goal**: Complete Russian translation, QR code display, and Modbus safety

### Story 2.1: Implement Russian translation framework
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

### Story 2.2: Translate all web interface pages
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

### Story 2.3: Add QR code display functionality
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

### Story 2.4: Implement WiFi setup QR for AP mode
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

### Story 2.5: Complete Modbus register 899 implementation
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

### Story 2.6: Add safety mechanisms and logging
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

### Story 2.7: Refactor event logs viewer to prevent controller suspension
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

### Story 2.8: Final User Manual Review and Alignment
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
