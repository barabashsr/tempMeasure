# Brownfield Product Requirements Document (PRD)
# Temperature Measurement System - MQTT Integration

## Document Information
- **Version**: 1.0
- **Date**: October 18, 2025
- **Status**: DRAFT
- **Project Phase**: Brownfield Enhancement

## Executive Summary

### Project Overview
This PRD defines the requirements for adding MQTT client capabilities to an existing ESP32-based industrial temperature monitoring system. The system currently monitors 60 measurement points with comprehensive alarm management, web interface, and Modbus RTU integration.

### Business Objective
Enable remote monitoring, control, and integration with modern IoT platforms while maintaining all existing functionality and ensuring zero regression in current operations.

### Key Deliverable
A fully-integrated MQTT client that provides real-time telemetry, remote command execution, alarm notifications, and system state updates without disrupting existing Modbus RTU, web interface, or local operations.

## Current System State

### Existing Capabilities
1. **Temperature Monitoring**
   - 60 measurement points (50 DS18B20 + 10 PT1000 capable)
   - Sensor binding to measurement points
   - Min/max tracking per point
   - CSV-based temperature logging

2. **Alarm System**
   - Three alarm types per point: HIGH_TEMPERATURE, LOW_TEMPERATURE, SENSOR_ERROR
   - Four priority levels: CRITICAL, HIGH, MEDIUM, LOW
   - State machine: NEW → ACTIVE → ACKNOWLEDGED → CLEARED → RESOLVED
   - Hysteresis support (configurable per point)
   - Relay control based on priority and state

3. **User Interfaces**
   - Web interface with dashboard, configuration pages
   - OLED display with circular scrolling
   - Single button interface (short/long press)
   - LED indicators (RED, YELLOW, BLUE, GREEN)
   - Three relay outputs (Siren, Beacon, Spare)

4. **Communication Protocols**
   - Modbus RTU server (RS-485)
   - HTTP/WebSocket for web interface
   - ConfigAssist for configuration management

5. **Data Management**
   - Temperature logging to CSV files
   - Event logging system
   - Alarm history tracking
   - Configuration persistence (YAML/INI files)

### Technical Constraints
- **Hardware**: ESP32-WROVER with fixed pin assignments
- **Memory**: Limited RAM/Flash, PSRAM available
- **Network**: WiFi connectivity required
- **Power**: 24V industrial power supply
- **Environment**: Industrial deployment conditions

### Known Limitations
- Single WiFi connection only
- No current remote monitoring capability
- Limited integration with modern IoT platforms
- No push notifications for alarms

## Business Requirements

### BR-1: Remote Monitoring
**Priority**: HIGH
- Enable real-time temperature monitoring from remote locations
- Support for multiple concurrent monitoring clients
- Maintain data integrity and timeliness

### BR-2: Cloud Integration
**Priority**: HIGH
- Compatible with major IoT platforms (AWS IoT, Azure IoT Hub, Google Cloud IoT)
- Support for generic MQTT brokers
- Enable data analytics and long-term storage

### BR-3: Remote Control
**Priority**: MEDIUM
- Allow authorized remote commands
- Maintain safety and security
- Audit trail for all remote actions

### BR-4: Alarm Notifications
**Priority**: HIGH
- Real-time alarm notifications to remote systems
- Support for alarm acknowledgment via MQTT
- Integration with notification systems

### BR-5: System Integration
**Priority**: MEDIUM
- Enable integration with SCADA systems
- Support for automation platforms (n8n, Node-RED)
- Maintain compatibility with existing Modbus systems

## Functional Requirements

### FR-1: MQTT Client Core
**Priority**: CRITICAL

#### FR-1.1: Connection Management
- Support MQTT 3.1.1 protocol (minimum)
- Automatic reconnection with exponential backoff
- Connection status reporting
- Configurable broker URL, port, credentials
- TLS/SSL support for secure connections

#### FR-1.2: Topic Structure
- Hierarchical topic organization: `<device_name>/<topic_type>/<subtopic>`
- Configurable device name
- Support for these topic types:
  - `/telemetry/` - Periodic sensor data
  - `/command/` - Bidirectional commands
  - `/alarm/` - Alarm notifications
  - `/state/` - System state changes
  - `/event/` - Discrete events

#### FR-1.3: Quality of Service
- QoS 0 for high-frequency telemetry
- QoS 1 for commands and alarms
- QoS 2 for critical operations
- Configurable QoS per topic type

### FR-2: Telemetry Publishing
**Priority**: HIGH

#### FR-2.1: Temperature Data
- Publish all 60 measurement points
- Configurable interval (10-3600 seconds, default 60s)
- Include point name, value, status, sensor ID
- JSON format with timestamp

#### FR-2.2: System Metrics
- Device uptime, firmware version
- Network statistics (RSSI, IP)
- Memory usage, CPU load
- Active alarm count by priority

#### FR-2.3: Sensor Status
- Connected sensor count
- Sensor errors per point
- Unbound sensor list
- Last read timestamps

### FR-3: Command Processing
**Priority**: HIGH

#### FR-3.1: Command Structure
- JSON-based command format
- Unique command ID for correlation
- Timestamp for command tracking
- Parameter validation

#### FR-3.2: Supported Commands
1. **System Commands**
   - `get_status` - System overview
   - `get_all_points` - All measurement data
   - `restart_system` - Controlled restart

2. **Alarm Commands**
   - `acknowledge_alarm` - Acknowledge specific alarm
   - `acknowledge_all_alarms` - Acknowledge all
   - `get_active_alarms` - List active alarms
   - `set_alarm_thresholds` - Update thresholds

3. **Configuration Commands**
   - `get_point_config` - Point configuration
   - `set_point_config` - Update point settings
   - `set_measurement_period` - Change intervals

4. **User Interaction Commands**
   - `send_message` - Display on OLED with acknowledgment
   - `help` - Get available commands (AI agent ready)

#### FR-3.3: Command Response
- Response within 500ms
- Success/error status
- Execution time tracking
- Detailed error messages

### FR-4: Alarm Integration
**Priority**: HIGH

#### FR-4.1: State Change Notifications
- Immediate publication on alarm state changes
- Include all alarm details (type, priority, value, threshold)
- Timestamp with millisecond precision

#### FR-4.2: Remote Acknowledgment
- Accept acknowledgment via MQTT
- Update local alarm state
- Notify other connected clients
- Log acknowledgment source

#### FR-4.3: Alarm History
- Query historical alarm data
- Filter by date, type, priority
- Export alarm statistics

### FR-5: Security
**Priority**: CRITICAL

#### FR-5.1: Authentication
- Username/password support
- Client certificate option
- Secure credential storage

#### FR-5.2: Encryption
- TLS 1.2 minimum
- Certificate validation
- Configurable cipher suites

#### FR-5.3: Access Control
- Command authorization
- Rate limiting (10 commands/second)
- Audit logging

### FR-6: Web Interface Integration
**Priority**: MEDIUM

#### FR-6.1: MQTT Configuration Page
- Enable/disable MQTT
- Broker configuration
- Authentication settings
- Topic prefix configuration
- TLS settings

#### FR-6.2: Status Display
- Connection status
- Message statistics
- Last error information
- Active subscriptions

### FR-7: Error Handling
**Priority**: HIGH

#### FR-7.1: Connection Failures
- Exponential backoff retry
- Maximum retry limit
- Offline message queuing
- Connection event logging

#### FR-7.2: Message Failures
- Failed message queue (100 messages)
- Retry logic for QoS > 0
- Error notification

#### FR-7.3: Resource Management
- Memory usage limits
- CPU usage throttling
- Bandwidth management

## Non-Functional Requirements

### NFR-1: Performance
- CPU overhead < 5% for MQTT operations
- Memory usage < 30KB additional RAM
- Message latency < 500ms to broker
- Support 60-point telemetry at 10s intervals

### NFR-2: Reliability
- 99.9% message delivery for QoS 1
- Reconnection within 30s of network recovery
- No impact on existing Modbus/Web operations
- Graceful degradation on MQTT failure

### NFR-3: Scalability
- Support up to 10 concurrent MQTT subscriptions
- Handle burst of 20 alarms simultaneously
- Queue up to 100 offline messages

### NFR-4: Maintainability
- Modular code architecture
- Comprehensive logging
- Diagnostic commands
- Version compatibility checking

### NFR-5: Usability
- Zero-configuration option (use defaults)
- Clear error messages
- Intuitive web configuration
- Self-documenting command system

## Implementation Constraints

### IC-1: Backward Compatibility
- **MUST NOT** change existing functionality
- **MUST NOT** modify hardware pin assignments
- **MUST** preserve all current APIs
- **MUST** maintain configuration file formats

### IC-2: Architecture Guidelines
- Use separate MQTTManager class
- Minimal modifications to existing classes
- Event-driven integration pattern
- Non-blocking operations only

### IC-3: Library Constraints
- Use PubSubClient or equivalent lightweight library
- Maximum library size: 50KB
- ESP32 Arduino framework compatible
- No external dependencies beyond broker

### IC-4: Testing Requirements
- Unit tests for all MQTT functions
- Integration tests with test broker
- 24-hour stability test
- Performance benchmarks

## User Stories

### US-1: Remote Monitoring Engineer
"As a remote monitoring engineer, I want to receive real-time temperature data via MQTT so that I can monitor the system from our central control room without requiring VPN access."

**Acceptance Criteria:**
- Temperature data published every 60 seconds
- All 60 points included in telemetry
- Data includes point names and status
- Timestamp accuracy within 1 second

### US-2: Maintenance Technician
"As a maintenance technician, I want to receive alarm notifications via MQTT so that I can respond quickly to critical issues even when away from the local HMI."

**Acceptance Criteria:**
- Alarms published within 1 second of trigger
- Include all relevant alarm information
- Support remote acknowledgment
- Maintain alarm history

### US-3: System Integrator
"As a system integrator, I want to send commands via MQTT so that I can integrate the temperature system with our facility automation platform."

**Acceptance Criteria:**
- Support for all critical commands
- Response within 500ms
- Clear error messages
- Command help system

### US-4: Operations Manager
"As an operations manager, I want the system to display important messages on the local OLED so that on-site operators are notified of remote requests requiring attention."

**Acceptance Criteria:**
- Messages displayed within 2 seconds
- RELAY1 blinks until acknowledged
- Button press acknowledges message
- Acknowledgment reported back via MQTT

### US-5: Security Administrator
"As a security administrator, I want all MQTT communications to be encrypted and authenticated so that the system meets our cybersecurity requirements."

**Acceptance Criteria:**
- TLS encryption for all traffic
- Username/password authentication
- Command authorization
- Audit trail for all commands

## Success Metrics

### SM-1: Technical Metrics
- Zero regression in existing functionality
- MQTT message delivery rate > 99.9% (QoS 1)
- Command response time < 500ms (95th percentile)
- Reconnection time < 30 seconds
- Memory overhead < 30KB

### SM-2: Operational Metrics
- Remote monitoring availability > 99%
- Alarm notification latency < 2 seconds
- Successful command execution rate > 95%
- Mean time to acknowledge alarms reduced by 50%

### SM-3: Business Metrics
- Integration with at least 2 cloud platforms
- Support for at least 3 automation platforms
- Reduced on-site maintenance visits by 30%
- Improved alarm response time by 40%

## Migration Strategy

### Phase 1: Core Implementation (Week 1-2)
1. Implement MQTTManager class
2. Basic connection and telemetry
3. Web configuration interface
4. Initial testing

### Phase 2: Command System (Week 2-3)
1. Command parser implementation
2. Core command set
3. Response handling
4. Error management

### Phase 3: Alarm Integration (Week 3)
1. Alarm state notifications
2. Remote acknowledgment
3. History queries
4. Priority handling

### Phase 4: Production Hardening (Week 4)
1. TLS implementation
2. Security audit
3. Performance optimization
4. Documentation

### Rollback Plan
- MQTT can be disabled via web interface
- No changes to core functionality
- Configuration preserved separately
- Firmware rollback supported

## Risk Assessment

### High Risks
1. **Memory Constraints**
   - Mitigation: Careful memory management, use PSRAM if needed
   
2. **Network Reliability**
   - Mitigation: Robust reconnection logic, offline queuing

3. **Security Vulnerabilities**
   - Mitigation: TLS mandatory, command validation, rate limiting

### Medium Risks
1. **Performance Impact**
   - Mitigation: Optimize message frequency, use QoS 0 for telemetry

2. **Integration Complexity**
   - Mitigation: Clean interfaces, comprehensive documentation

3. **Broker Compatibility**
   - Mitigation: Stick to MQTT 3.1.1 standard, test with multiple brokers

## Appendices

### A. Reference Architecture
See `/docs/briefs/mqtt_architecture_guide.md`

### B. Implementation Plan
See `/docs/briefs/mqtt_implementation_plan.md`

### C. Command Reference
See `/docs/briefs/mqtt_command_reference.md`

### D. Testing Guide
See `/docs/briefs/mqtt_testing_guide.md`

---

**Document Status**: This PRD is pending stakeholder review and approval before implementation begins.

**Next Steps**:
1. Stakeholder review and feedback
2. Technical feasibility confirmation
3. Resource allocation
4. Implementation kickoff

**Contact**: Project Management Team