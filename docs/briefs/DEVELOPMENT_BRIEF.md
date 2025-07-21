# Temperature Controller Development Brief

## Table of Contents
1. [Overview](#overview)
2. [Alarm System Redesign](#alarm-system-redesign)
3. [Web Interface Redesign](#web-interface-redesign)
4. [OLED Display & LED Indicators](#oled-display--led-indicators)
5. [Modbus RTU Implementation](#modbus-rtu-implementation)
6. [System Architecture](#system-architecture)
7. [Documentation Requirements](#documentation-requirements)

## Overview

This document provides comprehensive specifications for rewriting the Temperature Controller system while maintaining hardware compatibility and core functionality. The main goals are:

- Improved alarm handling with configurable priorities and hysteresis
- Consistent and modern web interface with enhanced features
- Better OLED display and LED indication logic
- Enhanced Modbus RTU implementation with explicit configuration triggers
- Clear separation between sensors and measurement points
- Comprehensive documentation in English and Russian

### Key Constraints
- **Pin definitions must remain unchanged** (hardware is already assembled)
- **Libraries and basic class structure should remain the same**
- **ConfigAssist library's file handling must be preserved**
- **PCF8575 logic must remain as proven**
- **Logger system works well - minimize changes**

## Alarm System Redesign

### 1. Alarm Structure
All alarms should exist from initialization but can be disabled through configuration:

```cpp
class AlarmConfig {
    AlarmType type;           // HIGH_TEMP, LOW_TEMP, SENSOR_ERROR, SENSOR_DISCONNECTED
    AlarmPriority priority;   // CRITICAL, HIGH, MEDIUM, LOW, DISABLED
    bool enabled;             // true/false
    int16_t threshold;        // Temperature threshold
    int16_t hysteresis;       // Hysteresis value (e.g., 2°C)
    uint32_t acknowledgedDelay; // Time before re-activation after acknowledgment
};
```

### 2. Alarm Priority Configuration

#### Via Web Interface
- Single comprehensive table showing all measurement points
- Columns: Point Name, Current Temp, Low Threshold, High Threshold, Low Priority, High Priority, Sensor Error Priority
- Dropdown menus for priority selection: Critical, High, Medium, Low, Disabled
- Apply changes button with confirmation

#### Via Config File
Already implemented in CSV format - maintain current structure

#### Via Modbus RTU
- Use register 800-899 for alarm configuration
- 8-bit register per point: 
  - Bits 0-2: Low temp alarm priority (0-4, where 4=disabled)
  - Bits 3-5: High temp alarm priority
  - Bits 6-7: Sensor error priority
- **Important**: Configuration should only apply when master writes to specific trigger register (e.g., 899)

### 3. Alarm State Management

```cpp
enum class AlarmStage {
    NEW,          // Just triggered
    ACTIVE,       // Confirmed active (after delay)
    ACKNOWLEDGED, // Operator acknowledged
    CLEARED,      // Condition cleared but in hysteresis
    RESOLVED      // Fully resolved
};
```

### 4. Relay Control Logic

#### Hardware Configuration
- Relay 1: Siren
- Relay 2: Flash beacon
- Relay 3: Spare

#### Notification Scenarios

| Priority | Active State | Acknowledged State |
|----------|-------------|-------------------|
| CRITICAL | Siren ON + Beacon ON (constant) | Beacon ON (constant) |
| HIGH | Beacon ON (constant) | Beacon ON (blink 2s on/30s off) |
| MEDIUM | Beacon ON (blink 2s on/30s off) | Beacon OFF |
| LOW | No relay action | No relay action |

### 5. Hysteresis Implementation

```cpp
// Example for high temperature alarm
if (temperature > (highThreshold + hysteresis)) {
    // Trigger alarm
} else if (temperature < highThreshold && alarmActive) {
    // Clear alarm
}
```

### 6. Acknowledged Alarm Re-activation
- Configurable delays per priority level
- After acknowledgment, alarm moves to ACKNOWLEDGED state
- Timer starts, and after delay expires, alarm returns to ACTIVE if condition still exists

## Web Interface Redesign

### 1. Navigation Structure
```
- Dashboard (Main page with all points overview)
- Alarms
  - Active Alarms
  - Alarm Configuration
  - Alarm History
- Points
  - Point Configuration
  - Sensor Binding
- System
  - General Settings
  - Network Settings
  - Time Settings
  - Modbus Settings
- Logs
  - Event Logs
  - Temperature Logs
  - Download Logs
```

### 2. Dashboard Page
- Table with all measurement points
- Columns: Point #, Name, Current Temp, Min, Max, Status, Trend Icon
- Color coding: Red (alarm), Yellow (warning), Green (normal), Gray (no sensor)
- Click trend icon to open modal with temperature chart
- Auto-refresh every 5 seconds

### 3. Alarm Configuration Page
Single table showing all configurable parameters:
```html
<table>
  <tr>
    <th>Point</th>
    <th>Name</th>
    <th>Low Threshold</th>
    <th>Low Priority</th>
    <th>High Threshold</th>
    <th>High Priority</th>
    <th>Error Priority</th>
    <th>Hysteresis</th>
  </tr>
  <!-- Dynamic rows for each point -->
</table>
```

### 4. Temperature Trend Modal
- Chart.js implementation showing last 24 hours
- Markers for alarm events with tooltips
- Data from log files
- Y-axis: Temperature, X-axis: Time
- Show threshold lines

### 5. Sensor Binding Interface
- Replace point number input with dropdown showing point names
- Add autocomplete functionality if possible
- Show current binding status clearly
- Validate point type matches sensor type

### 6. Russian Translation
All pages must support Russian language with toggle in header:
```javascript
const translations = {
  en: { 
    "dashboard": "Dashboard",
    "temperature": "Temperature",
    // ... 
  },
  ru: { 
    "dashboard": "Панель управления",
    "temperature": "Температура",
    // ...
  }
};
```

### 7. Consistent Styling
- Use modern CSS framework (Bootstrap 5 or similar)
- Dark mode support
- Consistent color scheme:
  - Primary: #2196F3 (blue)
  - Success: #4CAF50 (green)
  - Warning: #FF9800 (orange)
  - Danger: #F44336 (red)
- Responsive design for mobile access

## OLED Display & LED Indicators

### 1. Display States

#### Sleep Mode
- Display off after configurable timeout (default 60s)
- Wake on:
  - Button press
  - New alarm
  - System status request

#### Normal Operation Mode
- Cycle through points with bound sensors only
- Format:
  ```
  Point 01: Room A
  Temp: 23.5°C
  Min: 20.1  Max: 25.3
  Status: Normal
  ```
- 10-second display per point

#### Alarm Display Mode
- Show highest priority unacknowledged alarm first
- Format:
  ```
  ALARM: HIGH TEMP
  Point 05: Server Room
  Temp: 45.2°C (>40°C)
  Priority: CRITICAL
  Time: 14:23:15
  ```
- No cycling until acknowledged
- After all acknowledged, cycle through acknowledged alarms

#### System Status Mode
- Enter: Long button press (>3s)
- Exit: Another long press or timeout (30s)
- Pages (cycle with short press):
  1. Network info (IP, WiFi status)
  2. System stats (Points configured, Active sensors)
  3. Alarm summary (by priority)
  4. Modbus status

### 2. LED Indicator Logic

| LED Color | Purpose | Behavior |
|-----------|---------|----------|
| RED | Critical alarm | Solid when active, blink when acknowledged |
| YELLOW | High priority alarm | Solid when active, slow blink when acknowledged |
| BLUE | Medium priority alarm | Slow blink when active, off when acknowledged |
| GREEN | System OK | Solid when no alarms, off otherwise |

### 3. Button Handling
- **Short press (<1s)**:
  - Acknowledge current alarm
  - Navigate in system status mode
  - Wake display
- **Long press (>3s)**:
  - Enter/exit system status mode

### 4. Scrolling Improvement
Current scrolling should be circular:
```
"This is a long text that needs scrolling     This is a long text..."
```
Not jumping back to start.

## Modbus RTU Implementation

### 1. Extended Register Map

| Register Range | Description | Type | Access |
|----------------|-------------|------|--------|
| 0-799 | Existing registers | Various | R/R+W |
| 800-859 | Alarm configuration | UINT8 | R+W |
| 860-889 | Relay control | UINT16 | R+W |
| 890-899 | System commands | UINT16 | W |
| 900-959 | Point names (2 chars/register) | UINT16 | R |
| 960-999 | Extended status | UINT16 | R |

### 2. Alarm Configuration Registers (800-859)
One register per measurement point, 8-bit configuration:
```
Bit 7-6: Sensor error priority (0-3)
Bit 5-3: High temp alarm priority (0-4, 4=disabled)
Bit 2-0: Low temp alarm priority (0-4, 4=disabled)
```

### 3. Relay Control Registers (860-862)
- 860: Relay 1 control (0=Auto, 1=Force Off, 2=Force On)
- 861: Relay 2 control
- 862: Relay 3 control

### 4. System Command Register (899)
Write-only trigger register:
- 0x0001: Apply alarm configuration
- 0x0002: Reset min/max values
- 0x0003: Acknowledge all alarms
- 0x0004: Reboot system

### 5. Configuration Safety
- Configuration changes are buffered
- Only applied when command register triggered
- Validate all values before applying
- Log all configuration changes

## System Architecture

### 1. Class Hierarchy
```
TemperatureController
├── MeasurementPoint (60 instances)
│   ├── Sensor* (bound sensor reference)
│   └── AlarmConfig[] (configured alarms)
├── Sensor[] (discovered sensors)
├── Alarm[] (active alarms)
├── RegisterMap
└── AlarmManager (new class)
    ├── Relay control
    ├── LED control
    └── Priority handling
```

### 2. Data Flow
1. Sensors read temperature → Update MeasurementPoint
2. MeasurementPoint checks thresholds → Create/update Alarms
3. AlarmManager processes alarms → Control outputs
4. RegisterMap reflects current state
5. Web/Modbus interfaces read/write through controller

### 3. Key Principles
- **Separation of Concerns**: Sensors measure, Points aggregate, Alarms notify
- **Single Source of Truth**: MeasurementPoint holds authoritative data
- **Event-Driven**: Changes trigger updates, not polling
- **Fail-Safe**: Sensor errors disable temperature alarms for that point

## Documentation Requirements

### 1. Technical Documentation (English)
- **System Architecture**: Class diagrams, data flow
- **API Reference**: All public methods and interfaces
- **Modbus Register Map**: Complete table with examples
- **Configuration Guide**: All configuration options
- **Troubleshooting Guide**: Common issues and solutions

### 2. User Manual (Russian)
- **Руководство по установке**: Hardware connections, initial setup
- **Веб-интерфейс**: Screenshots and descriptions
- **Настройка сигнализации**: Alarm configuration guide
- **Работа с Modbus**: Integration examples
- **Обслуживание**: Maintenance procedures

### 3. Code Documentation
- All classes with Doxygen-style comments
- Complex algorithms explained inline
- Configuration file examples
- README files in each major directory

### 4. Log File Documentation
Current structure (maintain):
- `/data/YYYY-MM-DD_temps.csv`: Temperature readings
- `/alarms/YYYY-MM-DD_alarms.csv`: Alarm state changes
- `/events/YYYY-MM-DD_events.log`: System events

## Implementation Notes

### 1. ConfigAssist Integration
- Study existing implementation in ConfigManager.cpp
- Maintain YAML configuration structure
- Use ConfigAssist's file handling for all config files

### 2. Event Log Optimization
Current issue: "20 alarms created" generates 20 log entries
Solution: Batch similar events within 1-second window:
```cpp
logger.info("ALARM", "Multiple alarms created: HIGH_TEMP(5), LOW_TEMP(3), SENSOR_ERROR(2)");
```

### 3. Display Sleep/Wake
- Use hardware timer for reliable timeout
- Store display state to handle wake events properly
- Smooth transitions (fade in/out if OLED supports)

### 4. Web Interface Modularity
- Separate HTML, CSS, JS files
- Use template system for consistent headers/footers
- Implement API endpoints for all data access
- Cache static resources

### 5. Testing Considerations
- Unit tests for alarm logic
- Integration tests for Modbus commands
- Web interface testing on multiple browsers
- Stress testing with maximum sensors/alarms

## Version Control and Migration

### 1. Database Migration
- Preserve existing sensor bindings
- Convert alarm settings to new format
- Backup before migration

### 2. Configuration Compatibility
- Support old config format temporarily
- Automatic conversion on first boot
- Log all conversions

### 3. Firmware Update Process
- OTA update support
- Rollback capability
- Version checking in web interface

## Performance Requirements

- Web interface response: <500ms
- Sensor reading cycle: <5s for all 60 points
- Alarm detection latency: <1s
- Modbus response time: <100ms
- Display update rate: 10Hz minimum
- Log write buffering: Batch writes every 10s

## Security Considerations

- Web interface authentication (basic auth minimum)
- Modbus write protection (optional password)
- Configuration backup encryption
- Secure OTA updates
- Rate limiting on API endpoints
