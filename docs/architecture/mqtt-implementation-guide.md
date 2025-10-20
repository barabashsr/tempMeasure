# MQTT Implementation Guide

## Overview
The MQTT implementation uses the **static class pattern** (singleton-like) to provide global access to MQTT functionality throughout the codebase. The `MQTTManager` class is implemented entirely with static members and methods, making it accessible from any component without passing instances.

## Static Class Design
```cpp
class MQTTManager {
private:
    static MQTTConfig config;
    static MQTTClient mqttClient;
    // All members are static
    MQTTManager() = delete;  // Prevent instantiation

public:
    // All methods are static - accessible anywhere
    static bool begin();
    static void update(TemperatureController& controller);
    static bool publish(const char* topic, const char* payload, bool retain, int qos);
    static bool isEnabled() { return configLoaded && config.enabled; }
    static bool connected() { return mqttClient.connected(); }
};
```

## Integration Points

### 1. Main Application (main.cpp)
```cpp
void setup() {
    // Initialize MQTT after WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        MQTTManager::begin();
    }
}

void loop() {
    // Update MQTT in main loop
    if (MQTTManager::isEnabled()) {
        MQTTManager::update(temperatureController);
    }
}
```

### 2. Alarm State Changes (Alarm.cpp)
Add MQTT notification when alarm state changes:
```cpp
void Alarm::setState(AlarmStage newStage) {
    if (stage != newStage) {
        AlarmStage oldStage = stage;
        stage = newStage;
        
        // Log state change
        Serial.printf("Alarm state changed: Point %d from %s to %s\n", 
                     pointIndex, getStageString(oldStage), getStageString(newStage));
        
        // MQTT notification - ADD THIS
        if (MQTTManager::isEnabled() && MQTTManager::connected()) {
            publishAlarmStateChange(oldStage, newStage);
        }
        
        // Update timestamps
        updateTimestamp(newStage);
    }
}

void Alarm::publishAlarmStateChange(AlarmStage oldStage, AlarmStage newStage) {
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["point_index"] = pointIndex;
    doc["point_name"] = measurementPoint->getName();
    doc["alarm_type"] = (alarmType == ALARM_LOW) ? "LOW" : "HIGH";
    doc["old_state"] = getStageString(oldStage);
    doc["new_state"] = getStageString(newStage);
    doc["current_temp"] = measurementPoint->getCurrentTemp();
    doc["threshold"] = (alarmType == ALARM_LOW) ? 
                       measurementPoint->getLowAlarmThreshold() : 
                       measurementPoint->getHighAlarmThreshold();
    
    String topic = MQTTManager::buildTopic("alarms", "state_change");
    String payload;
    serializeJson(doc, payload);
    
    MQTTManager::publish(topic, payload, true, 1);  // Retain=true, QoS=1
}
```

### 3. Temperature Updates (TemperatureController.cpp)
Enable periodic temperature publishing in the update method:
```cpp
void MQTTManager::update(TemperatureController& controller) {
    if (!config.enabled) return;
    
    // Handle connection maintenance
    loop();
    
    // Publish temperature data periodically - ENABLE THIS
    if (connected()) {
        unsigned long now = millis();
        if (now - lastTelemetryPublish >= publishIntervalMs) {
            publishTemperatureData(controller);
            publishSystemStatus(controller);
            lastTelemetryPublish = now;
        }
    }
}
```

### 4. Command Processing (MQTTManager.cpp)
Implement command handling in the message callback:
```cpp
void MQTTManager::messageReceived(String &topic, String &payload) {
    Serial.printf("[MQTT] Received: Topic=%s, Payload=%s\n", 
                  topic.c_str(), payload.c_str());
    
    // Check if this is a command request
    if (topic.endsWith("/command/request")) {
        processCommand(topic, payload);
    }
}

void MQTTManager::processCommand(const String& topic, const String& payload) {
    JsonDocument request;
    DeserializationError error = deserializeJson(request, payload);
    
    if (error) {
        sendCommandError("Invalid JSON", "");
        return;
    }
    
    const char* cmd = request["command"];
    const char* cmdId = request["cmd_id"] | "";
    
    if (!cmd) {
        sendCommandError("Missing command field", cmdId);
        return;
    }
    
    // Process commands
    if (strcmp(cmd, "get_status") == 0) {
        handleGetStatus(cmdId);
    } else if (strcmp(cmd, "get_points_data") == 0) {
        handleGetPointsData(cmdId, request["params"]);
    } else if (strcmp(cmd, "acknowledge_alarm") == 0) {
        handleAcknowledgeAlarm(cmdId, request["params"]);
    } else if (strcmp(cmd, "reset_min_max") == 0) {
        handleResetMinMax(cmdId, request["params"]);
    } else if (strcmp(cmd, "send_message") == 0) {
        handleSendMessage(cmdId, request["params"]);
    } else {
        sendCommandError("Unknown command", cmdId);
    }
}
```

### 5. Web Interface Integration (ConfigManager.cpp)
Update MQTT settings when changed via web:
```cpp
void ConfigManager::onConfigChanged(String key) {
    if (key == "mqtt_enabled") {
        bool enabled = conf(key).toInt() == 1;
        if (enabled && !MQTTManager::isEnabled()) {
            MQTTManager::begin();
        } else if (!enabled && MQTTManager::isEnabled()) {
            MQTTManager::disconnect();
        }
    }
}
```

## MQTT Message Formats

### 1. Temperature Telemetry
**Topic**: `{prefix}/{device_name}/telemetry/temperature`  
**Publish Interval**: Configurable (default 60 seconds)  
**Retain**: false  
**QoS**: 0  

**Message Example**:
```json
{
  "timestamp": 1706543210123,
  "device_name": "temp_controller_01",
  "measurement_points": [
    {
      "address": "28:FF:12:34:56:78:90:AB",
      "name": "Tank 1 Bottom",
      "type": "DS18B20",
      "value": 65.5,
      "min": 64.2,
      "max": 67.8,
      "alarm_status": "NORMAL",
      "error_status": false
    },
    {
      "address": "28:FF:AB:CD:EF:12:34:56",
      "name": "Tank 1 Top",
      "type": "DS18B20",
      "value": 66.2,
      "min": 65.0,
      "max": 68.1,
      "alarm_status": "HIGH_WARNING",
      "error_status": false
    },
    {
      "address": "PT1000_0",
      "name": "Reactor Core",
      "type": "PT1000",
      "value": 125.3,
      "min": 120.1,
      "max": 128.5,
      "alarm_status": "NORMAL",
      "error_status": false
    }
  ]
}
```

### 2. System Status
**Topic**: `{prefix}/{device_name}/telemetry/status`  
**Publish Interval**: Same as temperature  
**Retain**: true  
**QoS**: 0  

**Message Example**:
```json
{
  "timestamp": 1706543210123,
  "device_name": "temp_controller_01",
  "device_id": 1001,
  "firmware_version": "2.1.0",
  "uptime": 3600,
  "alarms": {
    "active_count": 2,
    "acknowledged_count": 1,
    "total_count": 5
  },
  "network": {
    "wifi_connected": true,
    "wifi_ssid": "PlantNetwork",
    "wifi_rssi": -65,
    "ip_address": "192.168.1.100"
  },
  "memory": {
    "free_heap": 145632,
    "min_free_heap": 125000,
    "heap_size": 327680
  }
}
```

### 3. Alarm State Change
**Topic**: `{prefix}/{device_name}/alarms/state_change`  
**Publish**: On state change  
**Retain**: true  
**QoS**: 1  

**Message Example**:
```json
{
  "timestamp": 1706543215456,
  "point_index": 5,
  "point_name": "Tank 2 Middle",
  "alarm_type": "HIGH",
  "old_state": "NORMAL",
  "new_state": "HIGH_WARNING",
  "current_temp": 68.5,
  "threshold": 68.0
}
```

### 4. Command Request (Subscribe)
**Topic**: `{prefix}/{device_name}/command/request`  
**Direction**: Device subscribes  
**QoS**: 2  

**Command Examples**:

**4.1 Get Status Command**:
```json
{
  "command": "get_status",
  "cmd_id": "cmd_123456",
  "timestamp": 1706543220000
}
```

**4.2 Get Points Data Command**:
```json
{
  "command": "get_points_data",
  "cmd_id": "cmd_123457",
  "params": {
    "points": [0, 5, 10, 15],  // Optional: specific points
    "include_history": false
  }
}
```

**4.3 Acknowledge Alarm Command**:
```json
{
  "command": "acknowledge_alarm",
  "cmd_id": "cmd_123458",
  "params": {
    "point_index": 5,
    "alarm_type": "HIGH",
    "acknowledgment_message": "Operator aware, cooling initiated"
  }
}
```

**4.4 Reset Min/Max Command**:
```json
{
  "command": "reset_min_max",
  "cmd_id": "cmd_123459",
  "params": {
    "points": "all"  // or array of point indices
  }
}
```

**4.5 Send Message Command**:
```json
{
  "command": "send_message",
  "cmd_id": "cmd_123460",
  "params": {
    "message": "Maintenance scheduled for 14:00",
    "display_duration": 30
  }
}
```

### 5. Command Response (Publish)
**Topic**: `{prefix}/{device_name}/command/response`  
**Publish**: In response to commands  
**Retain**: false  
**QoS**: 2  

**Response Examples**:

**5.1 Success Response**:
```json
{
  "cmd_id": "cmd_123456",
  "status": "success",
  "timestamp": 1706543221000,
  "data": {
    // Command-specific response data
  }
}
```

**5.2 Get Status Response**:
```json
{
  "cmd_id": "cmd_123456",
  "status": "success",
  "timestamp": 1706543221000,
  "data": {
    "device_id": 1001,
    "active_alarms": [
      {
        "point_index": 5,
        "point_name": "Tank 2 Middle",
        "alarm_type": "HIGH",
        "state": "HIGH_WARNING",
        "duration": 300
      }
    ],
    "sensor_count": {
      "ds18b20": 45,
      "pt1000": 8,
      "total": 53
    }
  }
}
```

**5.3 Error Response**:
```json
{
  "cmd_id": "cmd_123458",
  "status": "error",
  "timestamp": 1706543222000,
  "error": {
    "code": "INVALID_POINT",
    "message": "Point index 65 does not exist"
  }
}
```

### 6. Last Will and Testament (LWT)
**Topic**: `{prefix}/{device_name}/state/connection`  
**Retain**: true  
**QoS**: 1  

**Online Message** (sent on connect):
```json
{
  "status": "online",
  "timestamp": 1706543200000
}
```

**Offline Message** (LWT):
```json
{
  "status": "offline"
}
```

## Topic Structure

The topic structure follows ISA-95 hierarchy pattern:
```
{level1}/{level2}/{level3}/{device_name}/{category}/{subcategory}
```

Example configurations:
- `plant/area1/line2/temp_controller_01/telemetry/temperature`
- `factory/building_a/zone3/tc_1001/alarms/state_change`
- `site/process/reactor/temp_mon_05/command/request`

Topic building example:
```cpp
// Configure hierarchy in settings
config.topic_level1_type = "plant";
config.topic_level1_value = "chemical_plant_1";
config.topic_level2_type = "area";
config.topic_level2_value = "reactor_area";
config.topic_level3_type = "line";
config.topic_level3_value = "reactor_1";
config.device_name = "temp_ctrl_r1";

// Results in topics like:
// chemical_plant_1/reactor_area/reactor_1/temp_ctrl_r1/telemetry/temperature
```

## Implementation Checklist

To fully integrate MQTT functionality:

1. **In TemperatureController::loop()**:
   ```cpp
   // Add MQTT update call
   if (MQTTManager::isEnabled()) {
       MQTTManager::update(*this);
   }
   ```

2. **In Alarm::setState()**:
   - Add alarm state change publishing

3. **In MQTTManager::update()**:
   - Uncomment temperature publishing calls
   - Remove test counter publishing

4. **In MQTTManager::messageReceived()**:
   - Implement command processing logic

5. **Subscribe to command topic** in connect():
   ```cpp
   String cmdTopic = buildTopic("command", "request");
   mqttClient.subscribe(cmdTopic, config.qos_commands);
   ```

6. **Add command handlers**:
   - Implement each command processing function
   - Send appropriate responses

## Memory Considerations

- Message buffer: 4KB allocated for MQTT client
- JSON documents: Use StaticJsonDocument where possible
- Topic strings: Build dynamically, don't store
- Payload optimization: Only send changed values when possible

## Error Handling

```cpp
// Connection error handling
if (!MQTTManager::connected()) {
    // MQTT operations will fail gracefully
    // Automatic reconnection handled internally
}

// Publish error handling
if (!MQTTManager::publish(topic, payload, retain, qos)) {
    Serial.printf("MQTT publish failed for topic: %s\n", topic);
    // Log error but don't block operation
}
```

## Testing MQTT Integration

1. **Test connection**: Check test topic publishing
2. **Verify telemetry**: Monitor temperature/status topics
3. **Test alarms**: Trigger alarm conditions
4. **Command testing**: Send each command type
5. **Disconnection handling**: Test network interruptions
6. **Load testing**: Verify with all 60 points active
