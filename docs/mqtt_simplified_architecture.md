# Simplified MQTT Architecture for Temperature Controller

## Overview
This document provides a streamlined implementation guide for MQTT in the Temperature Controller system. All features from the comprehensive architecture are retained but organized for practical implementation.

## Quick Start Topics

### Default Topic Structure
```
tempcontroller/<type>/<subtopic>
```

Examples:
- `tempcontroller/telemetry/temperature` - Temperature data
- `tempcontroller/command/request` - Send commands
- `tempcontroller/alarm/active` - Alarm notifications

### Enterprise Topic Structure (Optional)
For SCADA/Codesys integration, use configurable hierarchy:
```
<level1>/<level2>/<level3>/<device_name>/<type>/<subtopic>
```
Example: `plant/area1/line2/tempcontroller01/telemetry/temperature`

## Core Implementation Phases

### Phase 1: Essential Operations (Week 1)
**Goal**: Basic monitoring and alarm notifications for n8n/Telegram

#### 1.1 Temperature Telemetry
**Topic**: `tempcontroller/telemetry/temperature`
**Interval**: 60 seconds (configurable 10-3600s)
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "device_id": "tempcontroller01",
  "points": {
    "0": {"name": "Reactor Core", "temp": 75.3, "status": "OK"},
    "1": {"name": "Heat Exchanger", "temp": 85.2, "status": "OK"},
    // ... all 60 points
  },
  "summary": {
    "active_points": 45,
    "sensor_errors": 2,
    "active_alarms": 3
  }
}
```

#### 1.2 Alarm Notifications
**Topic**: `tempcontroller/alarm/active`
**Trigger**: On any alarm state change
```json
{
  "timestamp": "2025-01-27T10:00:00Z",
  "alarm_id": "ALM_2025_0127_001",
  "point": 5,
  "point_name": "Heat Exchanger",
  "type": "HIGH_TEMPERATURE",
  "priority": "CRITICAL",
  "temperature": 95.5,
  "threshold": 90.0,
  "state": "ACTIVE"
}
```

#### 1.3 Essential Commands

**Command Topic**: `tempcontroller/command/request`
**Response Topic**: `tempcontroller/command/response`

##### 1.3.1 Get Status
```json
// Request
{
  "cmd_id": "unique-id",
  "command": "get_status"
}

// Response
{
  "cmd_id": "unique-id",
  "status": "success",
  "data": {
    "sensors_active": 45,
    "alarms_active": 3,
    "uptime": 432000,
    "wifi_rssi": -65
  }
}
```

##### 1.3.2 Acknowledge Alarm
```json
// Request
{
  "cmd_id": "unique-id",
  "command": "acknowledge_alarm",
  "parameters": {
    "point_address": 5,
    "note": "Operator notified"
  }
}
```

##### 1.3.3 Send Message (OLED Display)
```json
// Request
{
  "cmd_id": "unique-id",
  "command": "send_message",
  "parameters": {
    "message": "Check cooling pump",
    "priority": "HIGH"
  }
}
```

### Phase 2: Remote Control (Week 2)
**Goal**: Full device control via MQTT alongside Modbus

#### 2.1 Configuration Commands

##### 2.1.1 Set Alarm Threshold
```json
{
  "command": "set_alarm_threshold",
  "parameters": {
    "point_address": 5,
    "high_threshold": 95.0,
    "low_threshold": 20.0
  }
}
```

##### 2.1.2 Control Relay
```json
{
  "command": "control_relay",
  "parameters": {
    "relay_number": 1,  // 1-3
    "mode": "AUTO",     // AUTO, FORCE_ON, FORCE_OFF
    "state": null       // true/false for FORCE modes
  }
}
```

#### 2.2 Data Queries

##### 2.2.1 Get All Points
```json
{
  "command": "get_all_points",
  "parameters": {
    "include_config": true
  }
}
```

##### 2.2.2 Get Points Data (Current Temperature & Alarms)
```json
// Request
{
  "command": "get_points_data",
  "parameters": {
    "points": [0, 5, 10, 15],  // List of point addresses
    "include_alarms": true
  }
}

// Response
{
  "cmd_id": "unique-id",
  "status": "success",
  "data": {
    "points": {
      "0": {
        "name": "Reactor Core",
        "temperature": 75.3,
        "status": "OK",
        "alarm_state": null
      },
      "5": {
        "name": "Heat Exchanger", 
        "temperature": 95.5,
        "status": "ALARM",
        "alarm_state": {
          "type": "HIGH_TEMPERATURE",
          "priority": "CRITICAL",
          "active_since": "2025-01-27T08:30:15Z",
          "acknowledged": false
        }
      },
      "10": {
        "name": "Cooling Tower",
        "temperature": null,
        "status": "SENSOR_ERROR",
        "alarm_state": {
          "type": "SENSOR_ERROR",
          "priority": "HIGH",
          "active_since": "2025-01-27T07:15:00Z",
          "acknowledged": true
        }
      },
      "15": {
        "name": "Storage Tank",
        "temperature": 23.5,
        "status": "OK",
        "alarm_state": null
      }
    },
    "timestamp": "2025-01-27T10:00:00Z"
  }
}
```

##### 2.2.3 Get Historical Data
```json
{
  "command": "get_historical_data",
  "parameters": {
    "points": [0, 5, 10],
    "hours": 24
  }
}
```

### Phase 3: Advanced Features (Week 3)
**Goal**: Analytics and automation

#### 3.1 Scheduled Commands
```json
{
  "command": "add_schedule",
  "parameters": {
    "schedule_id": "daily_report",
    "cron": "0 8 * * *",
    "command": "get_statistics",
    "publish_to": "tempcontroller/reports/daily"
  }
}
```

#### 3.2 Predictive Analytics
```json
{
  "command": "get_trend_analysis",
  "parameters": {
    "points": [5],
    "time_window": "24h"
  }
}
```

## n8n Integration Examples

### Telegram Alarm Notification
```javascript
// n8n Function Node
if (msg.topic.includes('/alarm/active')) {
  const alarm = JSON.parse(msg.payload);
  if (alarm.priority === 'CRITICAL' || alarm.priority === 'HIGH') {
    return {
      message: `🚨 ${alarm.priority} ALARM!\nPoint: ${alarm.point_name}\nTemp: ${alarm.temperature}°C\nThreshold: ${alarm.threshold}°C`,
      chatId: 'your-telegram-chat-id'
    };
  }
}
```

### Auto-Acknowledge Low Priority
```javascript
// n8n MQTT Out Node
if (alarm.priority === 'LOW' && isNightShift()) {
  return {
    topic: 'tempcontroller/command/request',
    payload: {
      cmd_id: generateId(),
      command: 'acknowledge_alarm',
      parameters: {
        point_address: alarm.point,
        note: 'Auto-acknowledged (night shift)'
      }
    }
  };
}
```

## SCADA/Codesys Integration

### Topic Mapping
```
SCADA Point                 → MQTT Topic
TempController.Point[0]     → tempcontroller/telemetry/temperature → points.0.temp
TempController.Alarm[0]     → tempcontroller/alarm/active → filter by point=0
TempController.Relay[1]     → tempcontroller/command/request → control_relay
```

### Modbus/MQTT Coexistence
- Modbus remains primary for local HMI/PLC
- MQTT provides remote monitoring and control
- Relay control respects Modbus overrides
- Both protocols share same data structures

## Configuration

### Web Interface Settings
```
MQTT Configuration Page (/settings-mqtt.html):
- Broker: [mqtt.broker.com]
- Port: [1883]
- Username: [tempcontroller01]
- Password: [****]
- Client ID: [tempcontroller01]
- Topic Prefix: [tempcontroller] or [plant/area1/line2/tempcontroller01]
- Telemetry Interval: [60] seconds
- QoS Telemetry: [0]
- QoS Alarms: [2]
- QoS Commands: [1]
```

## Command Quick Reference

### Essential Commands (Phase 1)
| Command | Purpose | Parameters |
|---------|---------|------------|
| get_status | System health check | None |
| acknowledge_alarm | Acknowledge active alarm | point_address, note |
| send_message | Display message on OLED | message, priority |
| help | List available commands | None |

### Control Commands (Phase 2)
| Command | Purpose | Parameters |
|---------|---------|------------|
| get_all_points | Get all 60 points data | include_config |
| get_points_data | Get specific points with alarms | points[], include_alarms |
| set_alarm_threshold | Update alarm limits | point_address, high/low_threshold |
| control_relay | Manual relay control | relay_number, mode, state |
| get_historical_data | Query past data | points[], hours |

### Advanced Commands (Phase 3)
| Command | Purpose | Parameters |
|---------|---------|------------|
| add_schedule | Create scheduled task | schedule_id, cron, command |
| get_trend_analysis | Temperature predictions | points[], time_window |
| get_statistics | System statistics | period, categories[] |
| bulk_export | Export large datasets | data_type, time_range |

## Testing Checklist

### Phase 1 Testing
- [ ] Subscribe to telemetry, verify 60-point updates
- [ ] Trigger alarm, verify notification
- [ ] Send acknowledge command via MQTT
- [ ] Test message display on OLED
- [ ] Verify n8n receives alarm notifications

### Phase 2 Testing
- [ ] Change threshold via MQTT
- [ ] Control relay via MQTT
- [ ] Query historical data
- [ ] Test Modbus/MQTT relay control priority

### Phase 3 Testing
- [ ] Create scheduled report
- [ ] Verify trend analysis accuracy
- [ ] Test bulk data export
- [ ] Load test with multiple clients

## Performance Targets

- Telemetry publishing: <100ms processing
- Command response: <500ms
- Memory overhead: <30KB
- Concurrent clients: 10
- Message queue: 20 deep

## Security Notes

### Minimum Requirements
- Username/password authentication
- Unique client IDs
- Read-only topics for telemetry
- Command authorization

### Production Requirements
- TLS/SSL encryption
- Certificate validation
- Rate limiting
- Audit logging

## Common Issues and Solutions

| Issue | Solution |
|-------|----------|
| Messages not received | Check topic subscription with wildcards |
| Commands timeout | Verify QoS settings (use QoS 1) |
| Relay conflicts | Modbus has priority, check register 860-862 |
| Memory issues | Reduce telemetry frequency or point count |

## Summary

This simplified architecture provides:
1. **Complete device control** via MQTT
2. **Easy n8n integration** for Telegram/automation
3. **SCADA compatibility** with configurable topics
4. **Phased implementation** from simple to advanced
5. **Clear command reference** for all operations

Start with Phase 1 for immediate monitoring/alerting, then add control features as needed. All commands from the comprehensive architecture remain available but are organized by practical use cases.