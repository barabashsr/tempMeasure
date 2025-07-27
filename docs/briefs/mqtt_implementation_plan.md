# MQTT Implementation Plan for Temperature Controller

## Executive Summary
This document provides a comprehensive implementation plan for integrating MQTT into the Temperature Controller system. The implementation will be phased, focusing on telemetry first, then commands, alarms, and advanced features.

## System Context
- **Device**: ESP32-based industrial temperature monitoring system
- **Points**: 60 measurement points (50 DS18B20, 10 PT1000/PT100)
- **Features**: Web interface, Modbus RTU, comprehensive alarm system, OLED display, relay control
- **Integration**: Works with n8n + AI for flexible automation

## Topic Structure Design

### Configurable Topic Hierarchy (ISA-95 Based)
The topic structure will be configurable via web interface with the following levels:

```
Level 1: [Custom/Predefined] - Enterprise/Plant/Site
Level 2: [Custom/Predefined] - Area/Department/Zone  
Level 3: [Custom/Predefined] - Line/Cell/Unit
Level 4: [Device Name] - Always the device identifier
Level 5: [Topic Type] - telemetry/command/alarm/state/event/notification/schedule
Level 6: [Subtopic] - Specific data type
```

### Default Configuration Example:
```
plant/area1/line2/tempcontroller01/telemetry/temperature
plant/area1/line2/tempcontroller01/command/request
plant/area1/line2/tempcontroller01/alarm/active
```

### Web Interface Configuration:
```html
<div class="mqtt-topic-config">
  <h3>MQTT Topic Configuration</h3>
  <div class="topic-level">
    <label>Level 1:</label>
    <select name="topic_level1_type">
      <option value="custom">Custom</option>
      <option value="plant">Plant</option>
      <option value="enterprise">Enterprise</option>
      <option value="skip">Skip Level</option>
    </select>
    <input type="text" name="topic_level1_value" placeholder="Enter custom value">
  </div>
  <!-- Repeat for levels 2-3 -->
  <div class="topic-preview">
    Preview: <code id="topic-preview">plant/area1/line2/device/telemetry/temperature</code>
  </div>
</div>
```

## Message Structures (Custom JSON)

### 1. Telemetry Messages

#### Temperature Data (Bulk) - `<prefix>/<device>/telemetry/temperature`
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "device_id": "tempcontroller01",
  "sequence": 12345,
  "measurement_period": 5,
  "points": {
    "0": {
      "name": "Reactor Core",
      "temp": 75.3,
      "min": 74.8,
      "max": 75.9,
      "sensor_id": "28FF1234567890AB",
      "sensor_type": "DS18B20",
      "status": "OK"
    },
    // ... up to 59
  },
  "summary": {
    "total_points": 60,
    "active_points": 45,
    "sensor_errors": 2,
    "active_alarms": 3,
    "min_temp": 22.1,
    "max_temp": 89.5,
    "avg_temp": 55.3
  }
}
```

#### Temperature Changes Only - `<prefix>/<device>/telemetry/changes`
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "device_id": "tempcontroller01",
  "changes": {
    "5": {
      "name": "Heat Exchanger",
      "temp": 95.5,
      "prev_temp": 85.2,
      "delta": 10.3,
      "rate": 2.06,  // °C/min
      "alarm_triggered": "HIGH_TEMP"
    }
  }
}
```

#### System Status - `<prefix>/<device>/telemetry/system`
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "device_id": "tempcontroller01",
  "uptime": 432000,  // seconds
  "free_heap": 45678,
  "wifi_rssi": -65,
  "ip_address": "192.168.1.100",
  "ssid": "Industrial-WiFi",
  "modbus_requests": 12345,
  "web_requests": 6789,
  "mqtt_messages_sent": 54321,
  "mqtt_reconnects": 2
}
```

### 2. Command Structure

#### Command Request - `<prefix>/<device>/command/request`
```json
{
  "cmd_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:00:00Z",
  "source": "n8n_automation",
  "command": "get_status",
  "parameters": {}
}
```

#### Command Response - `<prefix>/<device>/command/response`
```json
{
  "cmd_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:00:01Z",
  "command": "get_status",
  "status": "success",
  "execution_time": 45,  // ms
  "data": {
    // Command-specific response data
  }
}
```

### 3. Standard Commands

#### System Commands

##### `help` - Get available commands
Response:
```json
{
  "commands": {
    "system": ["help", "list", "status", "restart", "get_config"],
    "telemetry": ["get_all_points", "get_point", "get_changes", "get_summary"],
    "alarms": ["get_active", "get_history", "acknowledge", "test"],
    "config": ["get_thresholds", "set_threshold", "get_alarm_config", "set_alarm_config"],
    "relay": ["get_status", "set_relay", "test_relay"],
    "schedule": ["list_schedules", "add_schedule", "remove_schedule"]
  }
}
```

##### `status` - Get system status
Response:
```json
{
  "system": {
    "device_id": "tempcontroller01",
    "firmware": "2.0.0",
    "uptime": 432000,
    "time": "2025-01-27T10:00:00Z",
    "free_heap": 45678
  },
  "network": {
    "wifi_connected": true,
    "ssid": "Industrial-WiFi",
    "ip": "192.168.1.100",
    "rssi": -65
  },
  "sensors": {
    "total": 60,
    "active": 45,
    "errors": 2
  },
  "alarms": {
    "active": 3,
    "acknowledged": 1,
    "total_today": 15
  }
}
```

#### Telemetry Commands

##### `get_point` - Get single point data
Parameters:
```json
{
  "point_id": 5,
  "include_history": true,
  "history_minutes": 60
}
```

##### `get_summary` - Get system summary
Response:
```json
{
  "points": {
    "configured": 60,
    "active": 45,
    "alarming": 3
  },
  "temperature": {
    "min": 22.1,
    "max": 89.5,
    "avg": 55.3
  },
  "top_5_hottest": [
    {"id": 5, "name": "Reactor", "temp": 89.5},
    // ...
  ],
  "top_5_coldest": [
    {"id": 12, "name": "Storage", "temp": 22.1},
    // ...
  ]
}
```

#### Alarm Commands

##### `acknowledge_alarm` - Acknowledge specific alarm
Parameters:
```json
{
  "point_id": 5,
  "alarm_type": "HIGH_TEMP"
}
```

##### `acknowledge_all` - Acknowledge all active alarms
Response:
```json
{
  "acknowledged_count": 3,
  "alarms": [
    {"point_id": 5, "type": "HIGH_TEMP", "priority": "HIGH"},
    // ...
  ]
}
```

##### `set_threshold` - Update alarm threshold
Parameters:
```json
{
  "point_id": 5,
  "threshold_type": "high",  // "high" or "low"
  "value": 85.0,
  "hysteresis": 2.0
}
```

#### Configuration Commands

##### `get_alarm_config` - Get alarm configuration for a point
Parameters:
```json
{
  "point_id": 5
}
```

Response:
```json
{
  "point_id": 5,
  "name": "Reactor Core",
  "alarms": {
    "high_temp": {
      "enabled": true,
      "threshold": 80.0,
      "hysteresis": 2.0,
      "priority": "HIGH",
      "delay_seconds": 30
    },
    "low_temp": {
      "enabled": true,
      "threshold": 20.0,
      "hysteresis": 2.0,
      "priority": "MEDIUM",
      "delay_seconds": 60
    },
    "sensor_error": {
      "enabled": true,
      "priority": "HIGH"
    }
  }
}
```

##### `set_alarm_config` - Update alarm configuration
Parameters:
```json
{
  "point_id": 5,
  "alarm_type": "high_temp",  // "high_temp", "low_temp", "sensor_error"
  "config": {
    "enabled": true,
    "threshold": 85.0,
    "priority": "CRITICAL"
  }
}
```

### 4. Scheduler Commands

#### `add_schedule` - Add scheduled message
Parameters:
```json
{
  "schedule_id": "daily_report",
  "schedule_type": "interval",  // "interval", "cron", "once"
  "interval_seconds": 3600,     // For interval type
  "cron": "0 8 * * *",         // For cron type
  "command": "get_summary",
  "parameters": {}
}
```

#### `list_schedules` - List all schedules
Response:
```json
{
  "schedules": [
    {
      "id": "daily_report",
      "type": "cron",
      "expression": "0 8 * * *",
      "command": "get_summary",
      "last_run": "2025-01-27T08:00:00Z",
      "next_run": "2025-01-28T08:00:00Z"
    }
  ]
}
```

### 5. Alarm Event Messages

#### Alarm State Change - `<prefix>/<device>/alarm/state`
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "device_id": "tempcontroller01",
  "alarm_id": "ALM_2025_0127_001",
  "point_id": 5,
  "point_name": "Reactor Core",
  "alarm_type": "HIGH_TEMP",
  "priority": "CRITICAL",
  "state_transition": "RESOLVED->ACTIVE",
  "temperature": 95.5,
  "threshold": 90.0,
  "message": "Temperature exceeded critical threshold"
}
```

### 6. Notification Messages

#### Display Notification - `<prefix>/<device>/notification/display`
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "notification_id": "NOTIF_123",
  "source": "external_system",
  "priority": "HIGH",
  "display": {
    "text": "Cooling System Alert",
    "line1": "COOLING ALERT",
    "line2": "Check pump #3",
    "line3": "Press to ACK",
    "scroll": true,
    "duration": 0  // 0 = until acknowledged
  },
  "relay": {
    "relay_id": 1,
    "action": "on",
    "until": "acknowledged"
  },
  "require_ack": true
}
```

## Implementation Phases

### Phase 1: Core MQTT Infrastructure (Week 1)
1. **MQTTManager Class**
   ```cpp
   class MQTTManager {
   private:
       PubSubClient* mqttClient;
       MQTTConfig config;
       QueueHandle_t outgoingQueue;
       QueueHandle_t incomingQueue;
       TaskHandle_t mqttTask;
       SemaphoreHandle_t mqttMutex;
       
   public:
       bool begin();
       void connect();
       void disconnect();
       bool publish(const String& topic, const String& payload, uint8_t qos = 0);
       void subscribe(const String& topic, uint8_t qos = 1);
       void processCommands();
       void publishTelemetry();
   };
   ```

2. **Configuration Structure**
   ```cpp
   struct MQTTConfig {
       bool enabled;
       String broker_host;
       uint16_t broker_port;
       bool use_tls;
       String username;
       String password;
       String device_name;
       
       // Topic configuration
       String topic_level1_type;  // "custom", "plant", "skip"
       String topic_level1_value;
       String topic_level2_type;
       String topic_level2_value;
       String topic_level3_type;
       String topic_level3_value;
       
       // Intervals
       uint32_t telemetry_interval;
       uint32_t change_threshold;  // Minimum change to report
       
       // QoS levels
       uint8_t qos_telemetry;
       uint8_t qos_alarms;
       uint8_t qos_commands;
       
       // Features
       bool publish_changes;
       bool retain_telemetry;
       bool retain_alarms;
       String lwt_message;
   };
   ```

3. **Scheduler Class**
   ```cpp
   class MQTTScheduler {
   private:
       struct ScheduleEntry {
           String id;
           String type;  // "interval", "cron"
           uint32_t interval;
           String cron_expr;
           String command;
           JsonDocument parameters;
           unsigned long last_run;
           unsigned long next_run;
       };
       
       std::vector<ScheduleEntry> schedules;
       
   public:
       void addSchedule(const ScheduleEntry& entry);
       void removeSchedule(const String& id);
       void update();
       std::vector<ScheduleEntry> getSchedules();
   };
   ```

### Phase 2: Telemetry Implementation (Week 1-2)
1. Bulk temperature publishing
2. Change-based publishing
3. System status telemetry
4. Configurable intervals
5. Data compression for 60 points

### Phase 3: Command Processing (Week 2)
1. Command parser and router
2. Standard command implementations
3. Request/response correlation
4. Error handling and validation
5. Command queue management

### Phase 4: Alarm Integration (Week 2-3)
1. Alarm state change events
2. MQTT-based acknowledgment
3. Alarm history queries
4. Priority-based QoS
5. Alarm summary publishing

### Phase 5: Advanced Features (Week 3-4)
1. Scheduler implementation
2. Notification display system
3. Relay control via MQTT
4. Configuration synchronization
5. Bulk data transfers

### Phase 6: Security & Production (Week 4)
1. TLS implementation
2. Certificate management
3. Access control
4. Rate limiting
5. Audit logging

## Resource Requirements

### Memory
- MQTT buffer: 8KB (for 60-point telemetry)
- Command queue: 4KB (10 commands)
- Schedule storage: 2KB
- TLS overhead: 20KB (when enabled)
- Total: ~34KB additional RAM

### Performance
- Telemetry processing: <100ms
- Command response: <200ms
- Network overhead: ~30% with TLS
- CPU usage: <10% average

## Testing Strategy

### Unit Tests
1. Topic builder with all configurations
2. JSON serialization/deserialization
3. Command parsing and validation
4. Schedule calculation
5. Queue management

### Integration Tests
1. End-to-end telemetry flow
2. Command execution verification
3. Alarm notification delivery
4. Network failure recovery
5. Memory leak detection

### System Tests
1. 60-point telemetry at 10s intervals
2. Concurrent command processing
3. Alarm flood scenarios
4. Long-term stability (24h+)
5. Broker disconnection handling

## Configuration UI

### MQTT Settings Page (`/settings-mqtt.html`)
```html
<div class="settings-section">
  <h2>MQTT Configuration</h2>
  
  <div class="form-group">
    <label>
      <input type="checkbox" id="mqtt_enabled"> Enable MQTT
    </label>
  </div>
  
  <div class="form-group">
    <label>Broker Address:</label>
    <input type="text" id="mqtt_broker" placeholder="broker.hivemq.com">
  </div>
  
  <div class="form-group">
    <label>Port:</label>
    <input type="number" id="mqtt_port" value="1883">
  </div>
  
  <div class="form-group">
    <label>Device Name:</label>
    <input type="text" id="mqtt_device_name" value="tempcontroller01">
  </div>
  
  <div class="mqtt-topic-builder">
    <h3>Topic Structure</h3>
    <!-- Topic level configuration as shown above -->
  </div>
  
  <div class="mqtt-features">
    <h3>Features</h3>
    <label><input type="checkbox" id="mqtt_publish_changes"> Publish Changes Only</label>
    <label><input type="checkbox" id="mqtt_retain_telemetry"> Retain Telemetry</label>
    <label><input type="checkbox" id="mqtt_retain_alarms"> Retain Alarms</label>
  </div>
  
  <div class="mqtt-status">
    <h3>Connection Status</h3>
    <div id="mqtt-status-display">
      <span class="status-indicator"></span>
      <span class="status-text">Disconnected</span>
    </div>
    <div id="mqtt-stats">
      Messages Sent: <span id="mqtt-sent">0</span><br>
      Messages Received: <span id="mqtt-received">0</span><br>
      Last Error: <span id="mqtt-error">None</span>
    </div>
  </div>
</div>
```

## Success Criteria

1. **Reliability**: 99.9% message delivery for QoS 1+
2. **Performance**: <5% CPU overhead
3. **Latency**: <500ms command response time
4. **Scalability**: Handle 60 points at 10s intervals
5. **Integration**: Seamless n8n + AI workflow support

## Risk Mitigation

1. **Network Instability**: Implement robust reconnection with exponential backoff
2. **Memory Constraints**: Use streaming JSON serialization
3. **Security**: Mandatory TLS for production
4. **Compatibility**: Support both MQTT 3.1.1 and 5.0
5. **Data Loss**: Local queue during disconnections

## Conclusion

This MQTT implementation will provide a robust, scalable, and secure communication layer for the Temperature Controller system. The phased approach ensures early value delivery while building toward a comprehensive solution suitable for industrial deployment.

The flexible topic structure and comprehensive command set enable seamless integration with n8n and AI systems, while maintaining compatibility with standard MQTT practices and existing system architecture.