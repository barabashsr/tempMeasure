# MQTT Architecture Guide for Temperature Measurement System

## Overview

This guide outlines the MQTT communication architecture and command structure for the industrial temperature measurement system, supporting 60 measurement points with comprehensive alarm management and remote control capabilities.

## MQTT Topic Hierarchy

### Primary Topic Structure
```
<prefix>/<level1>/<level2>/<level3>/<device_name>/<topic_type>/<subtopic>
```

### Topic Levels
- **Level 1-3**: Configurable organizational hierarchy (e.g., plant/area/line)
- **Device Name**: Unique device identifier (e.g., tempcontroller01)
- **Topic Type**: Category of data or command
- **Subtopic**: Specific data within the category

### Topic Types

#### 1. Telemetry (`/telemetry/`)
Real-time sensor data and system metrics
- `/telemetry/temperature` - Temperature readings from all points
- `/telemetry/sensors` - Sensor health status
- `/telemetry/system` - System metrics (CPU, memory, uptime)

#### 2. Command (`/command/`)
Bidirectional command interface
- `/command/request` - Incoming commands from broker
- `/command/response` - Command execution results

#### 3. Alarm (`/alarm/`)
Alarm state notifications
- `/alarm/state` - Alarm state changes
- `/alarm/active` - New alarm activations
- `/alarm/acknowledged` - Alarm acknowledgments
- `/alarm/resolved` - Alarm resolutions

#### 4. State (`/state/`)
System state changes
- `/state/config` - Configuration changes
- `/state/network` - Network status updates
- `/state/device` - Device lifecycle events

#### 5. Event (`/event/`)
Discrete system events
- `/event/sensor` - Sensor connect/disconnect
- `/event/relay` - Relay state changes
- `/event/user` - User interactions

#### 6. Schedule (`/schedule/`)
Scheduled task management
- `/schedule/status` - Schedule execution status

## Command Architecture

### Command Request/Response Pattern

All commands follow a standardized request/response pattern with unique correlation IDs for tracking.

#### Request Structure
```json
{
  "cmd_id": "unique-identifier",
  "timestamp": "ISO8601-timestamp",
  "source": "origin-system",
  "command": "command_name",
  "parameters": {
    // Command-specific parameters
  }
}
```

#### Response Structure
```json
{
  "cmd_id": "matching-request-id",
  "timestamp": "ISO8601-timestamp",
  "command": "command_name",
  "status": "success|error",
  "execution_time": 45,
  "data": {
    // Command-specific response data
  },
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable description"
  }
}
```

### Command Categories

#### 1. System Interrogation
Commands for retrieving system information and status
- `get_system_info` - Hardware/firmware capabilities
- `get_all_points` - All measurement points with current values
- `get_sensors` - Discovered sensor information
- `get_alarm_config` - Alarm configuration details
- `get_modbus_map` - Modbus register mapping
- `get_network_config` - Network configuration
- `get_time_config` - RTC and time sync status
- `get_display_status` - Display configuration

#### 2. Configuration
Commands for modifying system parameters
- `set_point_config` - Configure measurement point
- `set_alarm_thresholds` - Update alarm thresholds
- `set_measurement_period` - Set measurement interval
- `set_modbus_config` - Configure Modbus parameters
- `set_network_config` - Update network settings
- `set_relay_mode` - Configure relay behavior
- `set_logging_config` - Data logging parameters

#### 3. Operational
Commands for system operations
- `force_measurement` - Trigger immediate measurement
- `reset_min_max` - Reset min/max values
- `clear_alarm_history` - Clear historical alarm data
- `export_config` - Export system configuration
- `import_config` - Import configuration
- `self_test` - Run system diagnostics
- `restart_system` - System restart
- `factory_reset` - Reset to defaults

#### 4. Data Query
Commands for retrieving historical and analytical data
- `get_historical_data` - Retrieve temperature history
- `get_alarm_history` - Query alarm events
- `get_statistics` - System operation statistics
- `get_trend_analysis` - Temperature trend analysis
- `bulk_export` - Export large datasets

#### 5. Alarm Management
Commands for alarm control
- `acknowledge_alarm` - Acknowledge specific alarm
- `acknowledge_all_alarms` - Acknowledge all active alarms
- `test_alarm` - Trigger test alarm
- `set_alarm_priority` - Change alarm priority
- `mute_alarms` - Temporarily mute outputs

#### 6. Relay Control
Commands for relay management
- `get_relay_status` - Current relay status
- `control_relay` - Direct relay control
- `set_relay_behavior` - Configure relay responses

#### 7. Schedule Management
Commands for scheduled operations
- `add_schedule` - Create scheduled task
- `list_schedules` - List all schedules
- `update_schedule` - Modify schedule
- `delete_schedule` - Remove schedule
- `run_schedule_now` - Execute immediately

#### 8. User Interaction
Commands for direct user interaction with the device
- `send_message` - Display message on OLED screen
  - Triggers RELAY1 to blink until button acknowledgment
  - Response includes acknowledgment timestamp
  - Used for critical notifications requiring operator attention
- `help` - Get list of available commands
  - Returns comprehensive command reference with data structures
  - Designed for n8n AI agent integration
  - Dynamically generated from runtime capabilities
  - Provides parameter schemas and expected responses

## Data Flow Patterns

### 1. Telemetry Flow
```
Device → Periodic Measurement → MQTT Publish → Broker → Subscribers
```
- Default interval: 60 seconds (configurable)
- QoS 0 for regular telemetry
- Payload includes all 60 measurement points

### 2. Command Flow
```
Client → Command Request → Broker → Device → Processing → Response → Broker → Client
```
- QoS 1 for command reliability
- Timeout handling for non-responsive commands
- Correlation via cmd_id

### 3. Alarm Flow
```
Threshold Breach → Alarm Generation → State Change → MQTT Publish → Notification
```
- Immediate publication on state change
- QoS 2 for critical alarms
- Includes priority and acknowledgment status

### 4. Configuration Change Flow
```
Configuration Update → Validation → Apply → State Change Event → MQTT Notification
```
- Atomic configuration updates
- Rollback on validation failure
- Change notifications to all subscribers

## Common Workflows

### 1. Temperature Monitoring
1. Subscribe to telemetry topics
2. Receive periodic temperature updates
3. Process data for display/storage
4. Monitor for anomalies

### 2. Alarm Response
1. Receive alarm notification
2. Evaluate alarm details
3. Send acknowledgment command
4. Monitor resolution status
5. Log incident details

### 3. Remote Configuration
1. Query current configuration
2. Prepare configuration update
3. Send configuration command
4. Verify successful application
5. Monitor system behavior

### 4. Data Collection
1. Schedule periodic data queries
2. Request historical data
3. Aggregate responses
4. Export for analysis
5. Archive processed data

### 5. System Maintenance
1. Run periodic self-tests
2. Monitor system health metrics
3. Schedule maintenance windows
4. Execute diagnostic commands
5. Generate maintenance reports

## Quality of Service Strategy

### QoS Level Assignment
- **QoS 0**: High-frequency telemetry, non-critical updates
- **QoS 1**: Commands, configuration changes, standard alarms
- **QoS 2**: Critical alarms, safety commands, audit events

### Message Persistence
- Telemetry: Not retained (real-time only)
- State: Retained (last known state)
- Alarms: Retained until acknowledged
- Configuration: Always retained

## Performance Considerations

### Message Frequency
- Telemetry: Configurable (10s-3600s)
- Alarms: Real-time on state change
- Events: As they occur
- Commands: On-demand

### Bandwidth Optimization
- Delta reporting for unchanged values
- Batch small events when possible
- Compress large payloads
- Implement local caching

### Resource Constraints
- Maximum payload: 16KB
- Command queue: 20 deep
- Concurrent commands: 10
- Subscribe limit: 10 topics

## Security Architecture

### Authentication Layers
1. MQTT username/password
2. TLS client certificates
3. Command-level authorization
4. Rate limiting per client

### Access Control
- Read-only topics for monitoring
- Write restrictions on commands
- Role-based topic permissions
- Audit logging for all commands

## Integration Patterns

### 1. SCADA Integration
- Map telemetry to SCADA points
- Forward critical alarms
- Implement command proxying
- Maintain state synchronization

### 2. Cloud Platform Integration
- AWS IoT Core topic mapping
- Azure IoT Hub device twins
- Google Cloud IoT telemetry
- Generic MQTT bridge setup

### 3. Analytics Platform Integration
- Stream to time-series databases
- Event-driven data pipelines
- Real-time analytics triggers
- Historical data warehousing

### 4. Automation Platform Integration
- n8n workflow triggers
- Node-RED flow integration
- Home Assistant entities
- Custom webhook endpoints

## Best Practices

### Topic Design
1. Use consistent hierarchical structure
2. Include version in topic path for upgrades
3. Avoid special characters in topic names
4. Keep topic depth reasonable (max 7 levels)

### Payload Design
1. Use JSON for human readability
2. Include timestamps in ISO8601 format
3. Provide context in each message
4. Version payload schemas

### Error Handling
1. Implement retry with exponential backoff
2. Queue critical messages during disconnection
3. Log all errors with context
4. Provide meaningful error messages

### Monitoring
1. Track message delivery rates
2. Monitor command response times
3. Alert on connection failures
4. Maintain command audit logs

---

*This document provides the architectural foundation for MQTT integration with the temperature measurement system. For detailed command parameters and examples, refer to the MQTT Command Reference documentation.*