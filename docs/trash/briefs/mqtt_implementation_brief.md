# MQTT Implementation Brief for Industrial Temperature Monitoring System

## Project Context
This brief outlines the implementation of an MQTT client for an ESP32-based industrial temperature monitoring system with 60 measurement points, comprehensive alarm management, and existing Modbus RTU integration.

## Executive Summary
The MQTT client will provide real-time telemetry, remote command execution, alarm notifications, and system state updates. The implementation must be robust, secure, and suitable for industrial deployment while maintaining compatibility with existing system architecture.

## Architecture Overview

### Topic Hierarchy
```
<device_name>/
├── telemetry/          # Periodic sensor data
│   ├── temperature     # Temperature readings from all points
│   ├── sensors         # Sensor health status
│   └── system          # System metrics (CPU, memory, uptime)
├── command/            # Incoming commands
│   ├── request         # Command requests from broker
│   └── response        # Command execution results
├── alarm/              # Alarm state changes
│   ├── active          # New alarm activations
│   ├── acknowledged    # Alarm acknowledgments
│   └── resolved        # Alarm resolutions
├── state/              # System state changes
│   ├── config          # Configuration changes
│   ├── network         # Network status updates
│   └── device          # Device lifecycle events
├── event/              # Discrete events
│   ├── sensor          # Sensor connect/disconnect
│   ├── relay           # Relay state changes
│   └── user            # User interactions (button press)
└── notification/       # External notifications
    └── display         # Messages to show on OLED
```

### Alternative Flat Structure (if preferred)
```
<device_name>/data          # Telemetry data
<device_name>/command       # Commands
<device_name>/alarm         # Alarm events
<device_name>/state         # State changes
<device_name>/event         # General events
<device_name>/notification  # Notifications
```

## Message Payload Structures

### 1. Telemetry Data (`<device_name>/telemetry/temperature`)
```json
{
  "timestamp": "2025-01-27T10:30:00Z",
  "device_id": "temp_controller_01",
  "sequence": 12345,
  "points": [
    {
      "id": 1,
      "name": "Reactor Core",
      "temperature": 75.3,
      "unit": "C",
      "sensor_id": "28FF1234567890AB",
      "quality": "GOOD"
    }
  ],
  "summary": {
    "total_points": 60,
    "active_points": 45,
    "min_temp": 22.1,
    "max_temp": 89.5,
    "avg_temp": 55.3
  }
}
```

### 2. Command Request (`<device_name>/command/request`)
```json
{
  "command_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:30:00Z",
  "command": "set_relay",
  "parameters": {
    "relay_id": 1,
    "state": "ON",
    "duration_seconds": 300
  }
}
```

### 3. Command Response (`<device_name>/command/response`)
```json
{
  "command_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:30:01Z",
  "status": "SUCCESS",
  "message": "Relay 1 activated for 300 seconds",
  "data": {
    "relay_id": 1,
    "previous_state": "OFF",
    "new_state": "ON"
  }
}
```

### 4. Alarm Event (`<device_name>/alarm/active`)
```json
{
  "timestamp": "2025-01-27T10:30:00Z",
  "alarm_id": "alarm_123",
  "point_id": 5,
  "point_name": "Heat Exchanger Input",
  "type": "HIGH_TEMPERATURE",
  "priority": "CRITICAL",
  "value": 95.5,
  "threshold": 90.0,
  "unit": "C",
  "message": "Temperature exceeded critical threshold"
}
```

### 5. State Change (`<device_name>/state/config`)
```json
{
  "timestamp": "2025-01-27T10:30:00Z",
  "category": "alarm_config",
  "changes": [
    {
      "point_id": 10,
      "parameter": "high_threshold",
      "old_value": 80.0,
      "new_value": 85.0
    }
  ],
  "source": "web_interface",
  "user": "operator"
}
```

### 6. Notification (`<device_name>/notification/display`)
```json
{
  "timestamp": "2025-01-27T10:30:00Z",
  "notification_id": "notif_456",
  "priority": "HIGH",
  "display_text": "Maintenance Required - Filter #3",
  "display_duration_seconds": 0,
  "require_acknowledgment": true,
  "relay_action": {
    "relay_id": 1,
    "action": "ON_UNTIL_ACK"
  }
}
```

## Supported Commands

### System Commands
- `get_status` - Return overall system status
- `get_config` - Return current configuration
- `restart` - Restart the device
- `get_telemetry` - Force immediate telemetry update

### Alarm Commands
- `acknowledge_alarm` - Acknowledge specific alarm
- `acknowledge_all` - Acknowledge all active alarms
- `get_active_alarms` - List all active alarms
- `test_alarm` - Trigger test alarm

### Relay Commands
- `set_relay` - Control relay state
- `get_relay_status` - Get current relay states

### Configuration Commands
- `set_point_config` - Update measurement point configuration
- `set_alarm_thresholds` - Update alarm thresholds
- `set_telemetry_interval` - Change reporting interval

## Quality of Service (QoS) Recommendations

### QoS Level 0 (Fire and Forget)
- Periodic telemetry data (high frequency)
- System metrics
- Non-critical state updates

### QoS Level 1 (At Least Once)
- Alarm notifications
- Command responses
- Configuration changes
- Important state updates

### QoS Level 2 (Exactly Once)
- Critical alarm events
- Safety-related commands
- Configuration updates that must not be duplicated

## Security Considerations

### Authentication
- Username/password authentication (minimum requirement)
- Client certificate authentication (recommended)
- Consider OAuth2 for cloud brokers

### TLS/SSL Encryption
- Mandatory for production deployment
- TLS 1.2 minimum
- Certificate validation required
- Consider certificate pinning for added security

### Access Control Lists (ACL)
```
# Read permissions
<device_name>/+/# - Allow broker to read all device topics

# Write permissions
<device_name>/command/# - Restrict to authorized control systems
<device_name>/notification/# - Restrict to authorized notification systems
```

### Security Best Practices
1. Use unique device IDs as client identifiers
2. Implement command validation and sanitization
3. Rate limiting for command processing
4. Audit logging for all commands
5. Regular security key rotation
6. Disable unused MQTT features (if any)

## Error Handling and Resilience

### Connection Management
```c
// Reconnection strategy with exponential backoff
uint32_t reconnect_delay = 1000;  // Start with 1 second
const uint32_t MAX_RECONNECT_DELAY = 300000;  // Max 5 minutes

while (!mqtt_client.connected()) {
    if (mqtt_client.connect(device_id, username, password)) {
        reconnect_delay = 1000;  // Reset on success
        resubscribe_to_topics();
        send_online_notification();
    } else {
        delay(reconnect_delay);
        reconnect_delay = min(reconnect_delay * 2, MAX_RECONNECT_DELAY);
    }
}
```

### Message Queue
- Implement local queue for critical messages during disconnection
- Maximum queue size: 100 messages
- Queue persistence: RAM only (consider SPIFFS for critical data)
- FIFO processing with priority for alarms

### Last Will and Testament (LWT)
```json
{
  "timestamp": "2025-01-27T10:30:00Z",
  "device_id": "temp_controller_01",
  "status": "OFFLINE",
  "last_seen": "2025-01-27T10:29:55Z",
  "reason": "unexpected_disconnect"
}
```
- Topic: `<device_name>/state/device`
- QoS: 1
- Retain: true

## Integration Points

### Existing Alarm System
- Hook into `Alarm::setState()` for state change notifications
- Use `AlarmPriority` enum for priority mapping
- Respect acknowledgment delays and re-activation logic

### Relay Control
- Integrate with existing relay control logic
- Maintain relay mode states (Auto/Manual)
- Ensure MQTT commands respect Modbus relay overrides

### Display Integration
- Notification display on OLED (3 lines max)
- Priority-based display ordering
- Button press for acknowledgment

### Web Interface
- MQTT configuration page in settings.html
- Real-time MQTT connection status
- Message traffic statistics

## Configuration Parameters

### settings.html Configuration
```html
<!-- MQTT Configuration Section -->
<h2>MQTT Settings</h2>
<form id="mqtt-config">
  <label>Enable MQTT: <input type="checkbox" name="mqtt_enabled"></label>
  <label>Broker URL: <input type="text" name="mqtt_broker" placeholder="mqtt://broker.example.com:1883"></label>
  <label>Username: <input type="text" name="mqtt_username"></label>
  <label>Password: <input type="password" name="mqtt_password"></label>
  <label>Device Name: <input type="text" name="mqtt_device_name" placeholder="temp_controller_01"></label>
  <label>Telemetry Interval (s): <input type="number" name="mqtt_telemetry_interval" value="60"></label>
  <label>Use TLS: <input type="checkbox" name="mqtt_use_tls"></label>
  <label>Retain Messages: <input type="checkbox" name="mqtt_retain"></label>
</form>
```

### ConfigAssist Integration
```cpp
// MQTT configuration structure
struct MQTTConfig {
    bool enabled = false;
    String broker_url = "";
    uint16_t port = 1883;
    String username = "";
    String password = "";
    String device_name = "temp_controller_01";
    uint32_t telemetry_interval = 60;  // seconds
    bool use_tls = false;
    bool retain_messages = false;
    uint8_t qos_telemetry = 0;
    uint8_t qos_alarms = 1;
    uint8_t qos_commands = 1;
};
```

## Performance Considerations

### Message Frequency
- Telemetry: Configurable (default 60s, minimum 10s)
- Alarms: Immediate on state change
- State: On change only
- Events: Real-time as they occur

### Bandwidth Optimization
- Compress large telemetry payloads (60 points = ~5KB uncompressed)
- Batch multiple events when possible
- Use binary payloads for high-frequency data (optional)
- Implement delta reporting for temperature values

### Resource Constraints
- Maximum MQTT packet size: 16KB
- Concurrent connections: 1 (single broker)
- Message buffer: 4KB
- Subscribe topic limit: 10

## Implementation Priorities

### Phase 1: Core MQTT Client (Week 1)
1. Basic MQTT connection with authentication
2. Telemetry publishing (temperature data)
3. Command subscription and basic command handling
4. Connection resilience (auto-reconnect)

### Phase 2: Alarm Integration (Week 2)
1. Alarm state change notifications
2. Alarm acknowledgment via MQTT
3. Alarm history queries
4. Priority-based message QoS

### Phase 3: Advanced Features (Week 3)
1. TLS/SSL implementation
2. Notification display system
3. Bulk data transfer (logs, historical data)
4. Performance optimization

### Phase 4: Production Hardening (Week 4)
1. Security audit and penetration testing
2. Load testing with 60 points
3. Documentation and deployment guide
4. Monitoring and diagnostics

## Testing Strategy

### Unit Tests
- Message serialization/deserialization
- Command parsing and validation
- Queue management
- Error handling scenarios

### Integration Tests
- End-to-end message flow
- Alarm trigger to notification
- Command execution verification
- Failover and recovery

### Performance Tests
- 60-point telemetry at 10s intervals
- Concurrent alarm flooding (10+ alarms)
- Network interruption recovery
- Memory leak detection

## Success Metrics

1. **Reliability**: 99.9% message delivery for QoS 1 messages
2. **Latency**: <500ms from event to broker delivery
3. **Recovery**: <30s reconnection after network restoration
4. **Efficiency**: <5% CPU overhead for MQTT operations
5. **Security**: Zero unauthorized command executions

## Open Questions for Stakeholder Review

1. **Topic Hierarchy**: Should we use hierarchical topics (`telemetry/temperature`) or flat structure (`data`)?
2. **Command Scope**: Which system parameters should be configurable via MQTT?
3. **Data Frequency**: What's the optimal telemetry interval for 60 points?
4. **Historical Data**: Should we implement bulk historical data transfer via MQTT?
5. **Retained Messages**: Which message types should be retained on the broker?
6. **Offline Behavior**: How long should we queue messages during disconnection?
7. **Multi-Broker**: Is there a need to support multiple MQTT brokers simultaneously?
8. **Bandwidth Limits**: Are there specific bandwidth constraints to consider?
9. **Protocol Version**: MQTT 3.1.1 or MQTT 5.0?
10. **Cloud Integration**: Specific cloud IoT platform requirements (AWS IoT, Azure IoT Hub)?

## Appendix: Example Implementation Snippet

```cpp
class MQTTManager {
private:
    WiFiClient* wifiClient;
    WiFiClientSecure* wifiClientSecure;
    PubSubClient* mqttClient;
    MQTTConfig config;
    QueueHandle_t messageQueue;
    
public:
    void publishTelemetry() {
        DynamicJsonDocument doc(4096);
        doc["timestamp"] = TimeManager::getISO8601Time();
        doc["device_id"] = config.device_name;
        
        JsonArray points = doc.createNestedArray("points");
        for (int i = 0; i < 60; i++) {
            if (measurementPoints[i].isSensorBound()) {
                JsonObject point = points.createNestedObject();
                point["id"] = i;
                point["name"] = measurementPoints[i].getName();
                point["temperature"] = measurementPoints[i].getTemperature();
                point["unit"] = "C";
            }
        }
        
        String topic = String(config.device_name) + "/telemetry/temperature";
        String payload;
        serializeJson(doc, payload);
        
        mqttClient->publish(topic.c_str(), payload.c_str(), config.retain_messages);
    }
};
```

---

*Document Version: 1.0*  
*Date: 2025-01-27*  
*Author: Project Management Team*  
*Status: DRAFT - Pending Stakeholder Review*