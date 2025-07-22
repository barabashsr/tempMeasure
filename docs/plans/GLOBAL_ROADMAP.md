# Temperature Controller Global Roadmap

## Project Overview
Temperature Control System - PlatformIO-based embedded system for monitoring and controlling temperature with 60 measurement points, web interface, Modbus RTU, and comprehensive alarm management.

## Current Implementation Status

### ✅ Completed Features
1. **Core Temperature Monitoring**
   - 60 measurement points with DS18B20 and PT1000 sensor support
   - Sensor binding to measurement points
   - Temperature logging to CSV files

2. **Alarm System Foundation**
   - Three alarm types per point: HIGH_TEMPERATURE, LOW_TEMPERATURE, SENSOR_ERROR
   - Alarm enable/disable functionality
   - Priority system (CRITICAL, HIGH, MEDIUM, LOW)
   - Basic alarm state management
   - Web interface for alarm configuration with enable checkboxes
   - Persistent storage in points2.ini
   - Hysteresis configuration implemented
   - Enhanced alarm configuration table

3. **Web Interface Core**
   - Dashboard with measurement points overview (`NEED TO BE IMPROVED`)
   - Alarm configuration page with enable/disable controls
   - Point name editing capability
   - API endpoints for configuration

4. **Modbus RTU Foundation**
   - Basic register map (0-799 existing registers)
   - Extended register map design (800-899 for alarm control) (`NOT TESTED`)
   - Command register (899) implementation started
   - Temperature reading via Modbus

5. **Hardware Integration**
   - PCF8575 I/O expander integration
   - OLED display basic functionality
   - LED indicators
   - Single button interface
   - Three relay outputs

6. **Configuration Management**
   - ConfigAssist library integration
   - YAML configuration files
   - CSV import/export for sensor bindings (`NOT TESTED`)
   - Points configuration persistence

7. **Logging System**
   - Temperature logging to CSV
   - Event logging
   - Alarm state change logging

## 📋 Implementation Roadmap (Based on Briefs)

### Priority 1: Relay Control Logic ✓ COMPLETED
1. **Relay Control Based on Alarm Priority and State** ✓
   - Implemented notification scenarios:
     - Relay1 (Siren): ON for ANY active alarm (any priority), OFF when all acknowledged
     - CRITICAL: Siren ON + Beacon ON (constant) → Acknowledged: Beacon ON (constant)
     - HIGH: Siren ON + Beacon ON (constant) → Acknowledged: Beacon ON (blink 2s on/30s off)
     - MEDIUM: Siren ON + Beacon ON (blink 2s on/30s off) → Acknowledged: Beacon OFF
     - LOW: Siren ON + No beacon action
   - Handles multiple active alarms (uses highest priority for beacon)

2. **Relay Control via Modbus RTU** ✓
   - Implemented registers 860-862 handling
   - Modes: 0=Auto, 1=Force Off, 2=Force On
   - Relay 3 controlled via Modbus only
   - Status feedback in registers 11-13

### Priority 2: Display & LED Improvements ✓ COMPLETED
1. **Circular Text Scrolling** ✓
   - Implemented smooth pixel-based scrolling without jumping
   - Configurable speed via SCROLL_SPEED_PIXELS and SCROLL_UPDATE_DELAY_MS
   - Fixed UTF-8 support for Cyrillic characters

2. **Multiple Unacknowledged Alarms** ✓
   - Implemented 3-line alarm display format
   - Line 1: Priority (C/H/M/L) + alarm type
   - Line 2: Point name + temperature/status (with ACK suffix for acknowledged)
   - Line 3: Alarm counter (e.g., "1/3") + activation time (HH:MM)
   - Manual advancement through alarms with button press

3. **System Status Mode** (Not implemented yet)
   - Enter/Exit: Long button press (>3s)
   - Navigate pages: Short button press
   - Auto-exit: 30s timeout
   - Pages:
     1. Network info (IP, WiFi status)
     2. System stats (Points configured, Active sensors, Unbound sensors count)
     3. Alarm summary by priority
     4. Alarm summary by type
     5. Modbus status

4. **LED Indicators** ✓
   - Fixed LED logic per requirements
   - GREEN: ON when no alarms (system OK)
   - RED: Solid for CRITICAL priority
   - YELLOW: Solid for HIGH priority
   - BLUE: Solid for MEDIUM, Blinking for LOW priority

### Priority 3: Core Functionality Completion
1. **Acknowledged Alarm Re-activation Delays**
   - Configurable per priority via ConfigAssist
   - Timer starts on acknowledgment
   - Re-activate if condition persists after delay

2. **Sensor Disconnection Behavior**
   - Auto-clear all temperature alarms for that point (disable the alarms)
   - Keep sensor error alarm active
   - Prevent false temperature alarms

3. **Alarm State Transitions**
   - Extend current working transitions (don't rewrite)
   - Ensure proper state persistence

### Priority 4: Web Interface Features (Specific from Briefs)
1. **Temperature Trend Charts**
   - Modal implementation (click trend icon in dashboard)
   - 24-hour data from log files
   - Chart.js library (try to use if possiable)
   - Alarm event markers with tooltips
   - Threshold lines display

2. **Real-time Dashboard Updates**
   - Auto-refresh every 5 seconds
   - Efficient data fetching

3. **Russian Translation**
   - Complete translation HTML files
   - All pages and messages

### Priority 5: Modbus RTU Completion
1. **Explicit Trigger for Alarm Configuration**
   - Only apply changes when command register 899 is written
   - Validate all values before applying
   - Prevent accidental zero/undefined values from master startup

2. **Complete Modbus Implementation**
   - Process alarm configuration commands
   - Log all Modbus configuration changes
   - Safety validations

## Performance Requirements
- Web interface response: <500ms
- Sensor reading cycle: <5s for all 60 points
- Alarm detection latency: <1s
- Modbus response time: <100ms
- Display update rate: 10Hz minimum
- Log write buffering: Batch writes every 10s

## Key Constraints & Principles
- **Pin definitions must remain unchanged** (hardware assembled)
- **Libraries and basic class structure preserved**
- **ConfigAssist file handling maintained**
- **PCF8575 logic proven - minimal changes**
- **Logger system works well - minimize changes**
- **Separation of Sensors and MeasurementPoints**
- **Event-driven architecture**
- **Fail-safe design (sensor errors disable temp alarms)**

## Implementation Notes
1. Always maintain backwards compatibility during migration
2. Use Doxygen documentation for all new code
3. Follow existing code conventions and patterns
4. Test on actual hardware before major releases
5. Keep web interface modular with separate HTML/CSS/JS
6. Implement proper error handling and recovery

## Success Metrics
- All 60 measurement points operational
- <1% false alarm rate
- 99.9% uptime
- All alarms acknowledged within configured time
- Successful integration with existing Modbus systems
- Positive user feedback on interface usability

---

*Last Updated: 2025-07-22*
*Version: 1.1*